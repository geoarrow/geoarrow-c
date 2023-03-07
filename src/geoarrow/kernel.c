
#include <errno.h>
#include <stddef.h>

#include "nanoarrow.h"

#include "geoarrow.h"

int kernel_start_void(struct GeoArrowKernel* kernel, struct ArrowSchema* schema,
                      const char* options, struct ArrowSchema* out,
                      struct GeoArrowError* error) {
  return ArrowSchemaInitFromType(out, NANOARROW_TYPE_NA);
}

int kernel_push_batch_void(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                           struct ArrowArray* out, struct GeoArrowError* error) {
  struct ArrowArray tmp;
  NANOARROW_RETURN_NOT_OK(ArrowArrayInitFromType(&tmp, NANOARROW_TYPE_NA));
  tmp.length = array->length;
  tmp.null_count = array->length;
  ArrowArrayMove(&tmp, out);
  return NANOARROW_OK;
}

int kernel_finish_void(struct GeoArrowKernel* kernel, struct ArrowArray* out,
                       struct GeoArrowError* error) {
  if (out != NULL) {
    return EINVAL;
  }

  return NANOARROW_OK;
}

void kernel_release_void(struct GeoArrowKernel* kernel) { kernel->release = NULL; }

void GeoArrowKernelInitVoid(struct GeoArrowKernel* kernel) {
  kernel->start = &kernel_start_void;
  kernel->push_batch = &kernel_push_batch_void;
  kernel->finish = &kernel_finish_void;
  kernel->release = &kernel_release_void;
  kernel->private_data = NULL;
}

int kernel_push_batch_void_agg(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                               struct ArrowArray* out, struct GeoArrowError* error) {
  if (out != NULL) {
    return EINVAL;
  }

  return NANOARROW_OK;
}

int kernel_finish_void_agg(struct GeoArrowKernel* kernel, struct ArrowArray* out,
                           struct GeoArrowError* error) {
  struct ArrowArray tmp;
  NANOARROW_RETURN_NOT_OK(ArrowArrayInitFromType(&tmp, NANOARROW_TYPE_NA));
  tmp.length = 1;
  tmp.null_count = 1;
  ArrowArrayMove(&tmp, out);
  return NANOARROW_OK;
}

void GeoArrowKernelInitVoidAgg(struct GeoArrowKernel* kernel) {
  kernel->start = &kernel_start_void;
  kernel->push_batch = &kernel_push_batch_void_agg;
  kernel->finish = &kernel_finish_void_agg;
  kernel->release = &kernel_release_void;
  kernel->private_data = NULL;
}
