
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

struct BuilderPrivate {
  // The ArrowArray responsible for owning the memory
  struct ArrowArray array;

  // Cached pointers pointing inside the array's private data
  // Depending on what exactly is being built, these pointers
  // might be NULL.
  struct ArrowBitmap* validity;
  struct ArrowBuffer* offsets[3];
  struct ArrowBuffer* data[4];

  // One more level of private data spec. Allocated with ArrowMalloc().
  void* private_data;
};

GeoArrowErrorCode GeoArrowBuilderInitFromType(struct GeoArrowBuilder* builder,
                                              enum GeoArrowType type) {
  NANOARROW_RETURN_NOT_OK(GeoArrowArrayViewInitFromType(&builder->view, type));
  builder->private_data = NULL;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowBuilderInitFromSchema(struct GeoArrowBuilder* builder,
                                                struct ArrowSchema* schema,
                                                struct GeoArrowError* error) {
  NANOARROW_RETURN_NOT_OK(GeoArrowArrayViewInitFromSchema(&builder->view, schema, error));
  builder->private_data = NULL;
  return GEOARROW_OK;
}

void GeoArrowBuilderInitVisitor(struct GeoArrowBuilder* builder,
                                struct GeoArrowVisitor* v) {
  return;
}

GeoArrowErrorCode GeoArrowBuilderFinish(struct GeoArrowBuilder* builder,
                                        struct ArrowArray* array,
                                        struct GeoArrowError* error) {
  return ENOTSUP;
}

void GeoArrowBuilderReset(struct GeoArrowBuilder* builder) {
  if (builder->private_data != NULL) {
    struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
    if (private->array.release != NULL) {
      private->array.release(&private->array);
    }

    // TODO: switch on type and clean up private_data
  }
}
