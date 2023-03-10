
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

static void GeoArrowKernelInitVoid(struct GeoArrowKernel* kernel) {
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

static void GeoArrowKernelInitVoidAgg(struct GeoArrowKernel* kernel) {
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
                                        struct ArrowArray* out,
                                        struct GeoArrowError* error) {
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

  private_data->v.error = error;
  struct ArrowArrayView array_view;
  struct GeoArrowBufferView buffer_view;
  ArrowArrayViewInitFromType(&array_view, NANOARROW_TYPE_BINARY);
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayViewSetArray(&array_view, array, (struct ArrowError*)error));

  for (int64_t i = 0; i < array->length; i++) {
    if (ArrowArrayViewIsNull(&array_view, i)) {
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_start(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.null_feat(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_end(&private_data->v));
    } else {
      struct ArrowBufferView value = ArrowArrayViewGetBytesUnsafe(&array_view, i);
      buffer_view.data = value.data.as_uint8;
      buffer_view.size_bytes = value.size_bytes;
      NANOARROW_RETURN_NOT_OK(GeoArrowWKBReaderVisit(&private_data->wkb_reader,
                                                     buffer_view, &private_data->v));
    }
  }

  return private_data->finish_push_batch(private_data, out, error);
}

static int kernel_push_batch_wkt(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                                 struct ArrowArray* out, struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;

  private_data->v.error = error;
  struct ArrowArrayView array_view;
  struct GeoArrowStringView buffer_view;
  ArrowArrayViewInitFromType(&array_view, NANOARROW_TYPE_STRING);
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayViewSetArray(&array_view, array, (struct ArrowError*)error));

  for (int64_t i = 0; i < array->length; i++) {
    if (ArrowArrayViewIsNull(&array_view, i)) {
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_start(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.null_feat(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_end(&private_data->v));
    } else {
      struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(&array_view, i);
      buffer_view.data = value.data;
      buffer_view.size_bytes = value.size_bytes;
      NANOARROW_RETURN_NOT_OK(GeoArrowWKTReaderVisit(&private_data->wkt_reader,
                                                     buffer_view, &private_data->v));
    }
  }

  return private_data->finish_push_batch(private_data, out, error);
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

static int kernel_visitor_start(struct GeoArrowKernel* kernel, struct ArrowSchema* schema,
                                const char* options, struct ArrowSchema* out,
                                struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;

  struct GeoArrowSchemaView schema_view;
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInit(&schema_view, schema, error));

  int result;
  switch (schema_view.type) {
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
      result = GeoArrowArrayViewInitFromType(&private_data->array_view, schema_view.type);
      break;
  }

  return NANOARROW_OK;
}

static int GeoArrowInitVisitorKernelInternal(struct GeoArrowKernel* kernel,
                                             const char* name) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowVisitorKernelPrivate));
  if (private_data == NULL) {
    return ENOMEM;
  }

  memset(private_data, 0, sizeof(struct GeoArrowVisitorKernelPrivate));
  private_data->finish_push_batch = &finish_push_batch_do_nothing;
  kernel->start = &kernel_visitor_start;
  kernel->push_batch = &kernel_push_batch_void_agg;
  kernel->finish = &kernel_finish_void_agg;
  kernel->release = &kernel_release_visitor;
  kernel->private_data = private_data;
  return NANOARROW_OK;
}

GeoArrowErrorCode GeoArrowKernelInit(struct GeoArrowKernel* kernel, const char* name,
                                     const char* options) {
  if (strcmp(name, "void") == 0) {
    GeoArrowKernelInitVoid(kernel);
    return NANOARROW_OK;
  } else if (strcmp(name, "void_agg") == 0) {
    GeoArrowKernelInitVoidAgg(kernel);
    return NANOARROW_OK;
  } else if (strcmp(name, "visit_void")) {
    return GeoArrowInitVisitorKernelInternal(kernel, name);
  }

  return ENOTSUP;
}
