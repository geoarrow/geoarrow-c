
#include "nanoarrow.h"

#include "geoarrow.h"

struct WKTBuilder {
  enum ArrowType storage_type;
  struct ArrowBitmap validity;
  struct ArrowBuffer offsets;
  struct ArrowBuffer values;
};

static int reserve_feat_wkt(struct GeoArrowVisitor* v, int64_t n) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;

  if (builder->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&builder->validity, n));
  }

  return ArrowBufferReserve(&builder->offsets, n * sizeof(int32_t));
}

static int feat_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  return ArrowBufferAppendInt32(&builder->offsets, builder->values.size_bytes);
}

static int null_feat_wkt(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  if (builder->validity.buffer.data == NULL) {
    int64_t n_feat = builder->offsets.size_bytes / sizeof(int32_t);
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&builder->validity, n_feat));
    ArrowBitmapAppendUnsafe(&builder->validity, 1, n_feat - 1);
  }

  return ArrowBitmapAppend(&builder->validity, 0, 1);
}

static int geom_start_wkt(struct GeoArrowVisitor* v, enum GeoArrowGeometryType geometry_type,
                    enum GeoArrowDimensions dimensions) { return GEOARROW_OK; }

static int ring_start_wkt(struct GeoArrowVisitor* v) { return GEOARROW_OK; }

static int coords_wkt(struct GeoArrowVisitor* v, const double** values, int64_t n_coords,
                int32_t n_dims) { return GEOARROW_OK; }

static int ring_end_wkt(struct GeoArrowVisitor* v) { return GEOARROW_OK; }

static int geom_end_wkt(struct GeoArrowVisitor* v) { return GEOARROW_OK; }

static int feat_end_wkt(struct GeoArrowVisitor* v) { return GEOARROW_OK; }

GeoArrowErrorCode GeoArrowWKTBuilderInit(struct GeoArrowVisitor* v) {
    GeoArrowVisitorInitVoid(v);
    struct WKTBuilder* builder = (struct WKTBuilder*)ArrowMalloc(sizeof(struct WKTBuilder));
    if (builder == NULL) {
      return ENOMEM;
    }

    builder->storage_type = NANOARROW_TYPE_STRING;
    ArrowBitmapInit(&builder->validity);
    ArrowBufferInit(&builder->offsets);
    ArrowBufferInit(&builder->values);
    v->private_data = builder;

    return GEOARROW_OK;
}

void GeoArrowWKTBuilderReset(struct GeoArrowVisitor* v) {
  struct WKTBuilder* builder = (struct WKTBuilder*)v->private_data;
  ArrowBitmapReset(&builder->validity);
  ArrowBufferReset(&builder->offsets);
  ArrowBufferReset(&builder->values);
  ArrowFree(builder);
}
