
#include "nanoarrow.h"

#include "geoarrow.h"

struct WKTBuilder {
  enum ArrowType storage_type;
  struct ArrowBitmap validity;
  struct ArrowBuffer offsets;
  struct ArrowBuffer values;
};

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
