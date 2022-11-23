
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

struct WKTWriter {
  enum ArrowType storage_type;
  struct ArrowBitmap validity;
  struct ArrowBuffer offsets;
  struct ArrowBuffer values;
  enum GeoArrowGeometryType geometry_type[32];
  int64_t i[32];
  int32_t level;
  int64_t length;
  int64_t null_count;
};

static inline int WKTWriterCheckLevel(struct WKTWriter* builder) {
  if (builder->level >= 0 && builder->level <= 32) {
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static inline int WKTWriterWrite(struct WKTWriter* builder, const char* value) {
  return ArrowBufferAppend(&builder->values, value, strlen(value));
}

static int reserve_feat_wkt(struct GeoArrowVisitor* v, int64_t n) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;

  if (builder->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&builder->validity, n));
  }

  NANOARROW_RETURN_NOT_OK(ArrowBufferReserve(&builder->offsets, n * sizeof(int32_t)));
  return ArrowBufferReserve(&builder->values, n * strlen("POINT ()"));
}

static int feat_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;
  builder->level = 0;
  builder->length++;
  return ArrowBufferAppendInt32(&builder->offsets, builder->values.size_bytes);
}

static int null_feat_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;
  if (builder->validity.buffer.data == NULL && builder->length > 0) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&builder->validity, builder->length));
    ArrowBitmapAppendUnsafe(&builder->validity, 1, builder->length - 1);
  }

  builder->null_count++;
  return ArrowBitmapAppend(&builder->validity, 0, 1);
}

static int geom_start_wkt(struct GeoArrowVisitor* v,
                          enum GeoArrowGeometryType geometry_type,
                          enum GeoArrowDimensions dimensions) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(builder));

  if (builder->level > 0 && builder->i[builder->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(builder, ", "));
    builder->i[builder->level - 1]++;
  }

  if (builder->level == 0 || builder->geometry_type[builder->level] ==
                                 GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION) {
    const char* geometry_type_name = GeoArrowGeometryTypeString(geometry_type);
    if (geometry_type_name == NULL) {
      ArrowErrorSet((struct ArrowError*)v->error,
                    "WKTWriter::geom_start(): Unexpected `geometry_type`");
      return EINVAL;
    }

    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(builder, geometry_type_name));

    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XY:
        break;
      case GEOARROW_DIMENSIONS_XYZ:
        NANOARROW_RETURN_NOT_OK(WKTWriterWrite(builder, " Z"));
        break;
      case GEOARROW_DIMENSIONS_XYM:
        NANOARROW_RETURN_NOT_OK(WKTWriterWrite(builder, " M"));
        break;
      case GEOARROW_DIMENSIONS_XYZM:
        NANOARROW_RETURN_NOT_OK(WKTWriterWrite(builder, " ZM"));
        break;
      default:
        ArrowErrorSet((struct ArrowError*)v->error,
                      "WKTWriter::geom_start(): Unexpected `dimensions`");
        return EINVAL;
    }
  }

  NANOARROW_RETURN_NOT_OK(WKTWriterWrite(builder, " "));

  builder->geometry_type[builder->level] = geometry_type;
  builder->i[builder->level] = 0;
  builder->level++;
  return GEOARROW_OK;
}

static int ring_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(builder));
  if (builder->level > 0 && builder->i[builder->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(builder, ", "));
    builder->i[builder->level - 1]++;
  }

  builder->geometry_type[builder->level] = GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  builder->i[builder->level] = 0;
  builder->level++;
  return GEOARROW_OK;
}

static int coords_wkt(struct GeoArrowVisitor* v, const double** values, int64_t n_coords,
                      int32_t n_dims) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;
  return GEOARROW_OK;
}

static int ring_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;
  builder->level--;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(builder));

  if (builder->i[builder->level] == 0) {
    return WKTWriterWrite(builder, "EMPTY");
  } else {
    return WKTWriterWrite(builder, ")");
  }
}

static int geom_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriter* builder = (struct WKTWriter*)v->private_data;
  builder->level--;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(builder));

  if (builder->i[builder->level] == 0) {
    return WKTWriterWrite(builder, "EMPTY");
  } else {
    return WKTWriterWrite(builder, ")");
  }
  return ArrowBufferAppendInt8(&builder->values, ')');
}

GeoArrowErrorCode GeoArrowWKTWriterInit(struct GeoArrowWKTWriter* writer) {
  struct WKTWriter* builder = (struct WKTWriter*)ArrowMalloc(sizeof(struct WKTWriter));
  if (builder == NULL) {
    return ENOMEM;
  }

  builder->storage_type = NANOARROW_TYPE_STRING;
  builder->length = 0;
  builder->level = 0;
  builder->null_count = 0;
  ArrowBitmapInit(&builder->validity);
  ArrowBufferInit(&builder->offsets);
  ArrowBufferInit(&builder->values);
  writer->private_data = builder;

  return GEOARROW_OK;
}

void GeoArrowWKTWriterInitVisitor(struct GeoArrowWKTWriter* writer,
                                  struct GeoArrowVisitor* v) {
  GeoArrowVisitorInitVoid(v);
  v->private_data = writer->private_data;
  v->reserve_feat = &reserve_feat_wkt;
  v->feat_start = &feat_start_wkt;
  v->null_feat = &null_feat_wkt;
  v->geom_start = &geom_start_wkt;
  v->ring_start = &ring_start_wkt;
  v->coords = &coords_wkt;
  v->ring_end = &ring_end_wkt;
  v->geom_end = &geom_end_wkt;
}

GeoArrowErrorCode GeoArrowWKTWriterFinish(struct GeoArrowWKTWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error) {
  struct WKTWriter* builder = (struct WKTWriter*)writer->private_data;
  array->release = NULL;

  NANOARROW_RETURN_NOT_OK(
      ArrowBufferAppendInt32(&builder->offsets, builder->values.size_bytes));
  NANOARROW_RETURN_NOT_OK(ArrowArrayInit(array, builder->storage_type));
  ArrowArraySetValidityBitmap(array, &builder->validity);
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 1, &builder->offsets));
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 2, &builder->values));
  array->length = builder->length;
  array->null_count = builder->null_count;
  return ArrowArrayFinishBuilding(array, (struct ArrowError*)error);
}

void GeoArrowWKTWriterReset(struct GeoArrowWKTWriter* writer) {
  struct WKTWriter* builder = (struct WKTWriter*)writer->private_data;
  ArrowBitmapReset(&builder->validity);
  ArrowBufferReset(&builder->offsets);
  ArrowBufferReset(&builder->values);
  ArrowFree(builder);
  writer->private_data = NULL;
}
