
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
  struct GeoArrowGeometry geometry;
};

static GeoArrowErrorCode GeoArrowArrayReaderVisitWKT(
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

static GeoArrowErrorCode GeoArrowArrayReaderVisitWKTArrow(
    const struct ArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowWKTReader* reader, struct GeoArrowVisitor* v) {
  struct ArrowStringView arrow_item;
  struct GeoArrowStringView item;

  for (int64_t i = 0; i < length; i++) {
    if (!ArrowArrayViewIsNull(array_view, offset + i)) {
      arrow_item = ArrowArrayViewGetStringUnsafe(array_view, offset + i);
      item.data = arrow_item.data;
      item.size_bytes = arrow_item.size_bytes;
      NANOARROW_RETURN_NOT_OK(GeoArrowWKTReaderVisit(reader, item, v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->feat_start(v));
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
      NANOARROW_RETURN_NOT_OK(v->feat_end(v));
    }
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayReaderVisitWKB(
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

static GeoArrowErrorCode GeoArrowArrayReaderVisitWKBArrow(
    const struct ArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowWKBReader* reader, struct GeoArrowVisitor* v) {
  struct ArrowBufferView arrow_item;
  struct GeoArrowBufferView item;

  for (int64_t i = 0; i < length; i++) {
    if (!ArrowArrayViewIsNull(array_view, offset + i)) {
      arrow_item = ArrowArrayViewGetBytesUnsafe(array_view, offset + i);
      item.data = arrow_item.data.as_uint8;
      item.size_bytes = arrow_item.size_bytes;
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

  if (private_data->geometry.private_data != NULL) {
    GeoArrowGeometryReset(&private_data->geometry);
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

  switch (private_data->type) {
    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_WKT_VIEW:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKB_VIEW:
      GEOARROW_RETURN_NOT_OK(ArrowArrayViewSetArray(&private_data->src.arrow, array,
                                                    (struct ArrowError*)error));
      break;
    default:
      GEOARROW_RETURN_NOT_OK(
          GeoArrowArrayViewSetArray(&private_data->src.geoarrow, array, error));
      break;
  }

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayReaderVisit(struct GeoArrowArrayReader* reader,
                                           int64_t offset, int64_t length,
                                           struct GeoArrowVisitor* v) {
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)reader->private_data;

  if (length == 0) {
    return GEOARROW_OK;
  }

  switch (private_data->type) {
    case GEOARROW_TYPE_WKT:
      return GeoArrowArrayReaderVisitWKT(&private_data->src.geoarrow, offset, length,
                                         &private_data->wkt_reader, v);
    case GEOARROW_TYPE_WKB:
      return GeoArrowArrayReaderVisitWKB(&private_data->src.geoarrow, offset, length,
                                         &private_data->wkb_reader, v);

    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKB_VIEW:
      return GeoArrowArrayReaderVisitWKBArrow(&private_data->src.arrow, offset, length,
                                              &private_data->wkb_reader, v);

    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_WKT_VIEW:
      return GeoArrowArrayReaderVisitWKTArrow(&private_data->src.arrow, offset, length,
                                              &private_data->wkt_reader, v);

    default:
      return GeoArrowArrayViewVisitNative(&private_data->src.geoarrow, offset, length, v);
  }
}

GeoArrowErrorCode GeoArrowArrayReaderRead(struct GeoArrowArrayReader* reader,
                                          int64_t offset, struct GeoArrowGeometry* out,
                                          struct GeoArrowError* error) {
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)reader->private_data;
  struct GeoArrowBufferView item;
  int is_null;

  switch (private_data->type) {
    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_WKT_VIEW:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKB_VIEW: {
      const struct ArrowArrayView* view = &private_data->src.arrow;
      if (offset < 0 || offset >= view->length) {
        GeoArrowErrorSet(error, "Array reader offset out of bounds");
        return EINVAL;
      }
      is_null = ArrowArrayViewIsNull(view, offset);
      if (!is_null) {
        struct ArrowBufferView bytes = ArrowArrayViewGetBytesUnsafe(view, offset);
        item.data = bytes.data.as_uint8;
        item.size_bytes = bytes.size_bytes;
      }
      break;
    }
    default: {
      const struct GeoArrowArrayView* view = &private_data->src.geoarrow;
      if (offset < 0 || offset >= view->length[0]) {
        GeoArrowErrorSet(error, "Array reader offset out of bounds");
        return EINVAL;
      }
      int64_t index = view->offset[0] + offset;
      is_null = view->validity_bitmap && !ArrowBitGet(view->validity_bitmap, index);
      if (!is_null && (private_data->type == GEOARROW_TYPE_WKT ||
                       private_data->type == GEOARROW_TYPE_WKB)) {
        item.data = view->data + view->offsets[0][index];
        item.size_bytes = view->offsets[0][index + 1] - view->offsets[0][index];
      }
      break;
    }
  }

  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryResizeNodesInline(out, 0));
  if (is_null) {
    struct GeoArrowGeometryNode* node;
    return GeoArrowGeometryAppendNodeInline(out, &node);
  }

  switch (private_data->type) {
    case GEOARROW_TYPE_WKB:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKB_VIEW: {
      struct GeoArrowGeometryView geom;
      GEOARROW_RETURN_NOT_OK(
          GeoArrowWKBReaderRead(&private_data->wkb_reader, item, &geom, error));
      return GeoArrowGeometryShallowCopy(geom, out);
    }
    case GEOARROW_TYPE_WKT:
    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_WKT_VIEW: {
      struct GeoArrowVisitor v;
      GeoArrowGeometryInitVisitor(out, &v);
      v.error = error;
      struct GeoArrowStringView text = {(const char*)item.data, item.size_bytes};
      return GeoArrowWKTReaderVisit(&private_data->wkt_reader, text, &v);
    }
    default:
      return GeoArrowArrayViewGetGeometry(&private_data->src.geoarrow, offset, out);
  }
}

static GeoArrowErrorCode GeoArrowArrayReaderVisitGeometrySerialized(
    struct GeoArrowArrayReaderPrivate* private_data, int64_t offset, int64_t length,
    GeoArrowGeometryCallback callback, void* callback_data, struct GeoArrowError* error,
    int is_wkt, int is_arrow) {
  struct GeoArrowVisitor visitor;
  if (is_wkt) {
    if (private_data->geometry.private_data == NULL) {
      GEOARROW_RETURN_NOT_OK(GeoArrowGeometryInit(&private_data->geometry));
    }
    GeoArrowGeometryInitVisitor(&private_data->geometry, &visitor);
    visitor.error = error;
  }

  const struct GeoArrowGeometryNode null_node = {0};
  const struct GeoArrowGeometryView null_geom = {&null_node, 1};
  for (int64_t i = offset; i < offset + length; i++) {
    struct GeoArrowBufferView item;
    int is_null;
    if (is_arrow) {
      const struct ArrowArrayView* view = &private_data->src.arrow;
      is_null = ArrowArrayViewIsNull(view, i);
      if (!is_null) {
        struct ArrowBufferView bytes = ArrowArrayViewGetBytesUnsafe(view, i);
        item.data = bytes.data.as_uint8;
        item.size_bytes = bytes.size_bytes;
      }
    } else {
      const struct GeoArrowArrayView* view = &private_data->src.geoarrow;
      int64_t index = view->offset[0] + i;
      is_null = view->validity_bitmap && !ArrowBitGet(view->validity_bitmap, index);
      if (!is_null) {
        item.data = view->data + view->offsets[0][index];
        item.size_bytes = view->offsets[0][index + 1] - view->offsets[0][index];
      }
    }

    struct GeoArrowGeometryView geom;
    if (is_null) {
      geom = null_geom;
    } else if (is_wkt) {
      struct GeoArrowStringView text = {(const char*)item.data, item.size_bytes};
      GEOARROW_RETURN_NOT_OK(
          GeoArrowWKTReaderVisit(&private_data->wkt_reader, text, &visitor));
      geom = GeoArrowGeometryAsView(&private_data->geometry);
    } else {
      // The callback can consume the parser's nodes directly: no shallow copy.
      GEOARROW_RETURN_NOT_OK(
          GeoArrowWKBReaderRead(&private_data->wkb_reader, item, &geom, error));
    }
    GEOARROW_RETURN_NOT_OK(callback(geom, callback_data, error));
  }
  return GEOARROW_OK;
}

// Points and lines have one node with constant type, dimensions, and strides.
// Initialize that metadata once for the whole range, including empty sequences.
static GeoArrowErrorCode GeoArrowArrayReaderVisitGeometrySequence(
    const struct GeoArrowArrayView* view, int64_t offset, int64_t length,
    GeoArrowGeometryCallback callback, void* callback_data, struct GeoArrowError* error) {
  struct GeoArrowGeometryNode node = {0};
  node.geometry_type = (uint8_t)view->schema_view.geometry_type;
  node.dimensions = (uint8_t)view->schema_view.dimensions;
  for (int j = 0; j < 4; j++) {
    node.coords[j] = _GeoArrowkEmptyPointCoords;
    node.coord_stride[j] = view->coords.coords_stride * (int32_t)sizeof(double);
  }
  const struct GeoArrowGeometryView geom = {&node, 1};
  const struct GeoArrowGeometryNode null_node = {0};
  const struct GeoArrowGeometryView null_geom = {&null_node, 1};
  int64_t begin = view->offset[0] + offset;
  for (int64_t i = begin; i < begin + length; i++) {
    if (view->validity_bitmap && !ArrowBitGet(view->validity_bitmap, i)) {
      GEOARROW_RETURN_NOT_OK(callback(null_geom, callback_data, error));
      continue;
    }

    int64_t coord_offset = i;
    node.size = 1;
    if (view->n_offsets == 1) {
      coord_offset = view->offsets[0][i] + view->offset[1];
      node.size = (uint32_t)(view->offsets[0][i + 1] - view->offsets[0][i]);
    }
    for (int j = 0; j < view->coords.n_values; j++) {
      node.coords[j] = node.size == 0
                           ? _GeoArrowkEmptyPointCoords
                           : (const uint8_t*)(view->coords.values[j] +
                                              coord_offset * view->coords.coords_stride);
    }
    GEOARROW_RETURN_NOT_OK(callback(geom, callback_data, error));
  }
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayReaderVisitGeometry(struct GeoArrowArrayReader* reader,
                                                   int64_t offset, int64_t length,
                                                   GeoArrowGeometryCallback callback,
                                                   void* callback_data,
                                                   struct GeoArrowError* error) {
  struct GeoArrowArrayReaderPrivate* private_data =
      (struct GeoArrowArrayReaderPrivate*)reader->private_data;
  int is_arrow = private_data->type == GEOARROW_TYPE_LARGE_WKB ||
                 private_data->type == GEOARROW_TYPE_WKB_VIEW ||
                 private_data->type == GEOARROW_TYPE_LARGE_WKT ||
                 private_data->type == GEOARROW_TYPE_WKT_VIEW;
  int64_t array_length =
      is_arrow ? private_data->src.arrow.length : private_data->src.geoarrow.length[0];
  if (offset < 0 || length < 0 || offset > array_length ||
      length > array_length - offset || (length > 0 && callback == NULL)) {
    GeoArrowErrorSet(error, "Invalid geometry callback or array reader range");
    return EINVAL;
  }
  if (length == 0) {
    return GEOARROW_OK;
  }

  switch (private_data->type) {
    case GEOARROW_TYPE_WKB:
      return GeoArrowArrayReaderVisitGeometrySerialized(
          private_data, offset, length, callback, callback_data, error, 0, 0);
    case GEOARROW_TYPE_WKT:
      return GeoArrowArrayReaderVisitGeometrySerialized(
          private_data, offset, length, callback, callback_data, error, 1, 0);
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKB_VIEW:
      return GeoArrowArrayReaderVisitGeometrySerialized(
          private_data, offset, length, callback, callback_data, error, 0, 1);
    case GEOARROW_TYPE_LARGE_WKT:
    case GEOARROW_TYPE_WKT_VIEW:
      return GeoArrowArrayReaderVisitGeometrySerialized(
          private_data, offset, length, callback, callback_data, error, 1, 1);
    default:
      if (private_data->src.geoarrow.schema_view.geometry_type ==
              GEOARROW_GEOMETRY_TYPE_POINT ||
          private_data->src.geoarrow.schema_view.geometry_type ==
              GEOARROW_GEOMETRY_TYPE_LINESTRING) {
        return GeoArrowArrayReaderVisitGeometrySequence(
            &private_data->src.geoarrow, offset, length, callback, callback_data, error);
      }
      if (private_data->geometry.private_data == NULL) {
        GEOARROW_RETURN_NOT_OK(GeoArrowGeometryInit(&private_data->geometry));
      }
      for (int64_t i = offset; i < offset + length; i++) {
        GEOARROW_RETURN_NOT_OK(GeoArrowArrayViewGetGeometry(&private_data->src.geoarrow,
                                                            i, &private_data->geometry));
        GEOARROW_RETURN_NOT_OK(callback(GeoArrowGeometryAsView(&private_data->geometry),
                                        callback_data, error));
      }
      return GEOARROW_OK;
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
