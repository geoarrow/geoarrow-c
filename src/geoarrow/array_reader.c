
#include "geoarrow/geoarrow.h"

#include "nanoarrow/nanoarrow.h"

union GeoArrowArrayReaderSrc {
  struct GeoArrowArrayView geoarrow;
  struct ArrowArrayView arrow;
};

struct GeoArrowArrayReaderPrivate {
  enum GeoArrowType type;
  union GeoArrowArrayReaderSrc src;
  struct GeoArrowWKTReader wkt_reader;
  struct GeoArrowWKBReader wkb_reader;
};

static GeoArrowErrorCode GeoArrowArrayViewVisitWKT(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowWKTReader* reader, struct GeoArrowVisitor* v) {
  struct GeoArrowStringView item;
  const int32_t* offset_begin = array_view->offsets[0] + array_view->offset[0] + offset;

  for (int64_t i = 0; i < length; i++) {
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      item.data = (const char*)(array_view->data + offset_begin[i]);
      item.size_bytes = offset_begin[i + 1] - offset_begin[i];
      NANOARROW_RETURN_NOT_OK(GeoArrowWKTReaderVisit(reader, item, v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->feat_start(v));
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
      NANOARROW_RETURN_NOT_OK(v->feat_end(v));
    }
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitWKB(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowWKBReader* reader, struct GeoArrowVisitor* v) {
  struct GeoArrowBufferView item;
  const int32_t* offset_begin = array_view->offsets[0] + array_view->offset[0] + offset;

  for (int64_t i = 0; i < length; i++) {
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      item.data = array_view->data + offset_begin[i];
      item.size_bytes = offset_begin[i + 1] - offset_begin[i];
      NANOARROW_RETURN_NOT_OK(GeoArrowWKBReaderVisit(reader, item, v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->feat_start(v));
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
      NANOARROW_RETURN_NOT_OK(v->feat_end(v));
    }
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayReaderInitInternal(
    struct GeoArrowArrayReaderPrivate* private_data, enum GeoArrowType type) {
  private_data->type = type;

  switch (type) {
    case GEOARROW_TYPE_LARGE_WKB:
      ArrowArrayViewInitFromType(&private_data->src.arrow, NANOARROW_TYPE_LARGE_BINARY);
      break;
    case GEOARROW_TYPE_WKB_VIEW:
      ArrowArrayViewInitFromType(&private_data->src.arrow, NANOARROW_TYPE_BINARY_VIEW);
      break;
    case GEOARROW_TYPE_LARGE_WKT:
      ArrowArrayViewInitFromType(&private_data->src.arrow, NANOARROW_TYPE_LARGE_STRING);
      break;
    case GEOARROW_TYPE_WKT_VIEW:
      ArrowArrayViewInitFromType(&private_data->src.arrow, NANOARROW_TYPE_STRING_VIEW);
      break;
    default:
      GEOARROW_RETURN_NOT_OK(
          GeoArrowArrayViewInitFromType(&private_data->src.geoarrow, type));
      break;
  }

  // Independent of the source view, we might need a parser
  switch (type) {
    case GEOARROW_TYPE_WKT:
    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_WKT_VIEW:
      return GeoArrowWKTReaderInit(&private_data->wkt_reader);
    case GEOARROW_TYPE_WKB:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKB_VIEW:
      return GeoArrowWKBReaderInit(&private_data->wkb_reader);
    default:
      return GEOARROW_OK;
  }
}

GeoArrowErrorCode GeoArrowArrayReaderInitFromType(struct GeoArrowArrayReader* reader,
                                                  enum GeoArrowType type) {
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowArrayReaderPrivate));

  if (private_data == NULL) {
    return ENOMEM;
  }

  memset(private_data, 0, sizeof(struct GeoArrowArrayReaderPrivate));

  int result = GeoArrowArrayReaderInitInternal(private_data, type);
  if (result != GEOARROW_OK) {
    ArrowFree(private_data);
    return result;
  }

  reader->private_data = private_data;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayReaderInitFromSchema(struct GeoArrowArrayReader* reader,
                                                    const struct ArrowSchema* schema,
                                                    struct GeoArrowError* error) {
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowArrayReaderPrivate));

  if (private_data == NULL) {
    GeoArrowErrorSet(error, "Failed to allocate GeoArrowArrayReaderPrivate");
    return ENOMEM;
  }

  memset(private_data, 0, sizeof(struct GeoArrowArrayReaderPrivate));

  struct GeoArrowSchemaView schema_view;
  int result = GeoArrowSchemaViewInit(&schema_view, schema, error);
  if (result != GEOARROW_OK) {
    ArrowFree(private_data);
    return result;
  }

  result = GeoArrowArrayReaderInitInternal(private_data, schema_view.type);
  if (result != GEOARROW_OK) {
    ArrowFree(private_data);
    GeoArrowErrorSet(error, "GeoArrowArrayReaderInitInternal() failed");
    return result;
  }

  reader->private_data = private_data;
  return GEOARROW_OK;
}

void GeoArrowArrayReaderReset(struct GeoArrowArrayReader* reader) {
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)reader->private_data;

  if (private_data->wkb_reader.private_data != NULL) {
    GeoArrowWKBReaderReset(&private_data->wkb_reader);
  }

  if (private_data->wkt_reader.private_data != NULL) {
    GeoArrowWKTReaderReset(&private_data->wkt_reader);
  }

  ArrowFree(reader->private_data);
  reader->private_data = NULL;
}

GeoArrowErrorCode GeoArrowArrayReaderSetArray(struct GeoArrowArrayReader* reader,
                                              const struct ArrowArray* array,
                                              struct GeoArrowError* error) {
  NANOARROW_DCHECK(reader != NULL);
  NANOARROW_DCHECK(array != NULL);
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)reader->private_data;
  NANOARROW_DCHECK(private_data != NULL);

  GEOARROW_RETURN_NOT_OK(
      GeoArrowArrayViewSetArray(&private_data->src.geoarrow, array, error));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayReaderVisit(struct GeoArrowArrayReader* reader,
                                           int64_t offset, int64_t length,
                                           struct GeoArrowVisitor* v) {
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)reader->private_data;

  switch (private_data->src.geoarrow.schema_view.type) {
    case GEOARROW_TYPE_WKT:
      return GeoArrowArrayViewVisitWKT(&private_data->src.geoarrow, offset, length,
                                       &private_data->wkt_reader, v);
    case GEOARROW_TYPE_WKB:
      return GeoArrowArrayViewVisitWKB(&private_data->src.geoarrow, offset, length,
                                       &private_data->wkb_reader, v);

    default:
      return GeoArrowArrayViewVisitNative(&private_data->src.geoarrow, offset, length, v);
  }
}

GeoArrowErrorCode GeoArrowArrayReaderArrayView(struct GeoArrowArrayReader* reader,
                                               const struct GeoArrowArrayView** out) {
  NANOARROW_DCHECK(reader->private_data != NULL);
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)reader->private_data;
  NANOARROW_DCHECK(private_data != NULL);

  switch (private_data->type) {
    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_WKT_VIEW:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKB_VIEW:
      return ENOTSUP;
    default:
      *out = &private_data->src.geoarrow;
      return GEOARROW_OK;
  }
}
