
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

static int kernel_start_void(struct GeoArrowKernel* kernel, struct ArrowSchema* schema,
                             const char* options, struct ArrowSchema* out,
                             struct GeoArrowError* error) {
  return ArrowSchemaInitFromType(out, NANOARROW_TYPE_NA);
}

static int kernel_push_batch_void(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                                  struct ArrowArray* out, struct GeoArrowError* error) {
  struct ArrowArray tmp;
  NANOARROW_RETURN_NOT_OK(ArrowArrayInitFromType(&tmp, NANOARROW_TYPE_NA));
  tmp.length = array->length;
  tmp.null_count = array->length;
  ArrowArrayMove(&tmp, out);
  return NANOARROW_OK;
}

static int kernel_finish_void(struct GeoArrowKernel* kernel, struct ArrowArray* out,
                              struct GeoArrowError* error) {
  if (out != NULL) {
    return EINVAL;
  }

  return NANOARROW_OK;
}

static void kernel_release_void(struct GeoArrowKernel* kernel) { kernel->release = NULL; }

void GeoArrowKernelInitVoid(struct GeoArrowKernel* kernel) {
  kernel->start = &kernel_start_void;
  kernel->push_batch = &kernel_push_batch_void;
  kernel->finish = &kernel_finish_void;
  kernel->release = &kernel_release_void;
  kernel->private_data = NULL;
}

static int kernel_push_batch_void_agg(struct GeoArrowKernel* kernel,
                                      struct ArrowArray* array, struct ArrowArray* out,
                                      struct GeoArrowError* error) {
  if (out != NULL) {
    return EINVAL;
  }

  return NANOARROW_OK;
}

static int kernel_finish_void_agg(struct GeoArrowKernel* kernel, struct ArrowArray* out,
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

struct GeoArrowVisitorKernelPrivate {
  struct GeoArrowVisitor v;
  struct GeoArrowWKBReader wkb_reader;
  struct GeoArrowWKTReader wkt_reader;
  struct GeoArrowArrayView array_view;
  struct ArrowArrayView na_array_view;
  struct GeoArrowBuilder builder;
  struct ArrowArray scratch;
  int (*finish_push_batch)(struct GeoArrowVisitorKernelPrivate* private_data,
                           struct ArrowArray* out, struct GeoArrowError* error);
};

static int finish_push_batch_do_nothing(struct GeoArrowVisitorKernelPrivate* private_data,
                           struct ArrowArray* out, struct GeoArrowError* error) {
                            return NANOARROW_OK;
                           }

static void kernel_release_visitor(struct GeoArrowKernel* kernel) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;
  if (private_data->wkb_reader.private_data != NULL) {
    GeoArrowWKBReaderReset(&private_data->wkb_reader);
  }

  if (private_data->wkt_reader.private_data != NULL) {
    GeoArrowWKTReaderReset(&private_data->wkt_reader);
  }

  if (private_data->builder.private_data != NULL) {
    GeoArrowBuilderReset(&private_data->builder);
  }

  if (private_data->na_array_view.n_children > 0) {
    ArrowArrayViewReset(&private_data->na_array_view);
  }

  if (private_data->scratch.release != NULL) {
    private_data->scratch.release(&private_data->scratch);
  }

  ArrowFree(private_data);
  kernel->release = NULL;
}

static int kernel_push_batch_wkb(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                                 struct ArrowArray* out, struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;
  return ENOTSUP;
}

static int kernel_push_batch_wkt(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                                 struct ArrowArray* out, struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;
  return ENOTSUP;
}

static int kernel_push_batch_geoarrow(struct GeoArrowKernel* kernel,
                                      struct ArrowArray* array, struct ArrowArray* out,
                                      struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;
  NANOARROW_RETURN_NOT_OK(
      GeoArrowArrayViewSetArray(&private_data->array_view, array, error));
  private_data->v.error = error;
  NANOARROW_RETURN_NOT_OK(GeoArrowArrayViewVisit(&private_data->array_view, 0,
                                                 array->length, &private_data->v));
  return private_data->finish_push_batch(private_data, out, error);
}

static int GeoArrowInitVisitorKernelInternal(struct GeoArrowKernel* kernel,
                                             enum GeoArrowType in_type) {

  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowVisitorKernelPrivate));
  if (private_data == NULL) {
    return ENOMEM;
  }

  int result;
  switch (in_type) {
    case GEOARROW_TYPE_UNINITIALIZED:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_LARGE_WKT:
      result = EINVAL;
      break;
    case GEOARROW_TYPE_WKT:
      kernel->push_batch = &kernel_push_batch_wkt;
      result = GeoArrowWKTReaderInit(&private_data->wkt_reader);
      break;
    case GEOARROW_TYPE_WKB:
      kernel->push_batch = &kernel_push_batch_wkt;
      result = GeoArrowWKBReaderInit(&private_data->wkb_reader);
      break;
    default:
      kernel->push_batch = &kernel_push_batch_geoarrow;
      result = GeoArrowArrayViewInitFromType(&private_data->array_view, in_type);
      break;
  }

  if (result != GEOARROW_OK) {
    ArrowFree(private_data);
    return result;
  }

  memset(private_data, 0, sizeof(struct GeoArrowVisitorKernelPrivate));
  kernel->release = kernel_release_visitor;
  kernel->private_data = private_data;
  return NANOARROW_OK;
}
