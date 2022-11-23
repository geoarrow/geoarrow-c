
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

struct WKTBuilder {
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

static inline int WKTBuilderCheckLevel(struct WKTBuilder* builder) {
  if (builder->level >= 0 && builder->level <= 32) {
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static inline int WKTBuilderWrite(struct WKTBuilder* builder, const char* value) {
  return ArrowBufferAppend(&builder->values, value, strlen(value));
}

static int reserve_feat_wkt(struct GeoArrowVisitor* v, int64_t n) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;

  if (builder->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&builder->validity, n));
  }

  NANOARROW_RETURN_NOT_OK(ArrowBufferReserve(&builder->offsets, n * sizeof(int32_t)));
  return ArrowBufferReserve(&builder->values, n * strlen("POINT ()"));
}

static int feat_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  builder->level = 0;
  builder->length++;
  return ArrowBufferAppendInt32(&builder->offsets, builder->values.size_bytes);
}

static int null_feat_wkt(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
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
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTBuilderCheckLevel(builder));

  if (builder->level > 0 && builder->i[builder->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTBuilderWrite(builder, ", "));
    builder->i[builder->level - 1]++;
  }

  if (builder->level == 0 || builder->geometry_type[builder->level] ==
                                 GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION) {
    const char* geometry_type_name = GeoArrowGeometryTypeString(geometry_type);
    if (geometry_type_name == NULL) {
      ArrowErrorSet((struct ArrowError*)v->error,
                    "WKTBuilder::geom_start(): Unexpected `geometry_type`");
      return EINVAL;
    }

    NANOARROW_RETURN_NOT_OK(WKTBuilderWrite(builder, geometry_type_name));

    switch (dimensions) {
      case GEOARROW_DIMENSIONS_XY:
        break;
      case GEOARROW_DIMENSIONS_XYZ:
        NANOARROW_RETURN_NOT_OK(WKTBuilderWrite(builder, " Z"));
        break;
      case GEOARROW_DIMENSIONS_XYM:
        NANOARROW_RETURN_NOT_OK(WKTBuilderWrite(builder, " M"));
        break;
      case GEOARROW_DIMENSIONS_XYZM:
        NANOARROW_RETURN_NOT_OK(WKTBuilderWrite(builder, " ZM"));
        break;
      default:
        ArrowErrorSet((struct ArrowError*)v->error,
                      "WKTBuilder::geom_start(): Unexpected `dimensions`");
        return EINVAL;
    }
  }

  NANOARROW_RETURN_NOT_OK(WKTBuilderWrite(builder, " "));

  builder->geometry_type[builder->level] = geometry_type;
  builder->i[builder->level] = 0;
  builder->level++;
  return GEOARROW_OK;
}

static int ring_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTBuilderCheckLevel(builder));
  if (builder->level > 0 && builder->i[builder->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTBuilderWrite(builder, ", "));
    builder->i[builder->level - 1]++;
  }

  builder->geometry_type[builder->level] = GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  builder->i[builder->level] = 0;
  builder->level++;
  return GEOARROW_OK;
}

static int coords_wkt(struct GeoArrowVisitor* v, const double** values, int64_t n_coords,
                      int32_t n_dims) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  return GEOARROW_OK;
}

static int ring_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  builder->level--;
  NANOARROW_RETURN_NOT_OK(WKTBuilderCheckLevel(builder));

  if (builder->i[builder->level] == 0) {
    return WKTBuilderWrite(builder, "EMPTY");
  } else {
    return WKTBuilderWrite(builder, ")");
  }
}

static int geom_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  builder->level--;
  NANOARROW_RETURN_NOT_OK(WKTBuilderCheckLevel(builder));

  if (builder->i[builder->level] == 0) {
    return WKTBuilderWrite(builder, "EMPTY");
  } else {
    return WKTBuilderWrite(builder, ")");
  }
  return ArrowBufferAppendInt8(&builder->values, ')');
}

GeoArrowErrorCode GeoArrowWKTBuilderInit(struct GeoArrowVisitor* v) {
  GeoArrowVisitorInitVoid(v);
  struct WKTBuilder* builder = (struct WKTBuilder*)ArrowMalloc(sizeof(struct WKTBuilder));
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
  v->private_data = builder;

  v->reserve_feat = &reserve_feat_wkt;
  v->feat_start = &feat_start_wkt;
  v->null_feat = &null_feat_wkt;
  v->geom_start = &geom_start_wkt;
  v->ring_start = &ring_start_wkt;
  v->coords = &coords_wkt;
  v->ring_end = &ring_end_wkt;
  v->geom_end = &geom_end_wkt;

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowWKTBuilderFinish(struct GeoArrowVisitor* v,
                                           struct ArrowArray* array) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  array->release = NULL;

  NANOARROW_RETURN_NOT_OK(ArrowBufferAppendInt32(&builder->offsets, builder->values.size_bytes));
  NANOARROW_RETURN_NOT_OK(ArrowArrayInit(array, builder->storage_type));
  ArrowArraySetValidityBitmap(array, &builder->validity);
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 1, &builder->offsets));
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 2, &builder->values));
  array->length = builder->length;
  array->null_count = builder->null_count;
  return ArrowArrayFinishBuilding(array, (struct ArrowError*)v->error);
}

void GeoArrowWKTBuilderReset(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  ArrowBitmapReset(&builder->validity);
  ArrowBufferReset(&builder->offsets);
  ArrowBufferReset(&builder->values);
  ArrowFree(builder);
  v->private_data = NULL;
}
