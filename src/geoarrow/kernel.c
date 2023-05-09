
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
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

// Visitor-based kernels
//
// These kernels implement generic operations by visiting each feature in
// the input (since all GeoArrow types including WKB/WKT can be visited).
// This for conversion to/from WKB and WKT whose readers and writers are
// visitor-based. Most other operations are probably faster phrased as
// "cast to GeoArrow in batches then do the thing" (but require these kernels to
// do the "cast to GeoArrow" step).

struct GeoArrowGeometryTypesVisitorPrivate {
  enum GeoArrowGeometryType geometry_type;
  enum GeoArrowDimensions dimensions;
  uint64_t geometry_types_mask;
};

struct GeoArrowVisitorKernelPrivate {
  struct GeoArrowVisitor v;
  int visit_by_feature;
  struct GeoArrowWKBReader wkb_reader;
  struct GeoArrowWKTReader wkt_reader;
  struct GeoArrowArrayView array_view;
  struct ArrowArrayView na_array_view;
  struct GeoArrowWKBWriter wkb_writer;
  struct GeoArrowWKTWriter wkt_writer;
  struct GeoArrowBuilder builder;
  struct GeoArrowGeometryTypesVisitorPrivate geometry_types_private;
  int (*finish_push_batch)(struct GeoArrowVisitorKernelPrivate* private_data,
                           struct ArrowArray* out, struct GeoArrowError* error);
  int (*finish_start)(struct GeoArrowVisitorKernelPrivate* private_data,
                      struct ArrowSchema* schema, const char* options,
                      struct ArrowSchema* out, struct GeoArrowError* error);
};

static int kernel_get_arg_long(const char* options, const char* key, long* out,
                               int required, struct GeoArrowError* error) {
  struct ArrowStringView type_str;
  type_str.data = NULL;
  type_str.size_bytes = 0;
  NANOARROW_RETURN_NOT_OK(ArrowMetadataGetValue(options, ArrowCharView(key), &type_str));
  if (type_str.data == NULL && required) {
    ArrowErrorSet((struct ArrowError*)error, "Missing required parameter '%s'", key);
    return EINVAL;
  } else if (type_str.data == NULL && !required) {
    return NANOARROW_OK;
  }

  char type_str0[16];
  memset(type_str0, 0, sizeof(type_str0));
  snprintf(type_str0, sizeof(type_str0), "%.*s", (int)type_str.size_bytes, type_str.data);
  *out = atoi(type_str0);
  return NANOARROW_OK;
}

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

  if (private_data->na_array_view.storage_type != NANOARROW_TYPE_UNINITIALIZED) {
    ArrowArrayViewReset(&private_data->na_array_view);
  }

  if (private_data->wkb_writer.private_data != NULL) {
    GeoArrowWKBWriterReset(&private_data->wkb_writer);
  }

  if (private_data->wkt_writer.private_data != NULL) {
    GeoArrowWKTWriterReset(&private_data->wkt_writer);
  }

  ArrowFree(private_data);
  kernel->release = NULL;
}

static int kernel_push_batch_wkb(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                                 struct ArrowArray* out, struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;

  private_data->v.error = error;
  struct ArrowArrayView* array_view = &private_data->na_array_view;
  struct GeoArrowBufferView buffer_view;
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayViewSetArray(array_view, array, (struct ArrowError*)error));

  for (int64_t i = 0; i < array->length; i++) {
    if (ArrowArrayViewIsNull(array_view, i)) {
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_start(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.null_feat(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_end(&private_data->v));
    } else {
      struct ArrowBufferView value = ArrowArrayViewGetBytesUnsafe(array_view, i);
      buffer_view.data = value.data.as_uint8;
      buffer_view.size_bytes = value.size_bytes;
      int result = GeoArrowWKBReaderVisit(&private_data->wkb_reader, buffer_view,
                                          &private_data->v);
      if (result == EAGAIN) {
        NANOARROW_RETURN_NOT_OK(private_data->v.feat_end(&private_data->v));
      } else if (result != NANOARROW_OK) {
        return result;
      }
    }
  }

  return private_data->finish_push_batch(private_data, out, error);
}

static int kernel_push_batch_wkt(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                                 struct ArrowArray* out, struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;

  private_data->v.error = error;
  struct ArrowArrayView* array_view = &private_data->na_array_view;
  struct GeoArrowStringView buffer_view;
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayViewSetArray(array_view, array, (struct ArrowError*)error));

  for (int64_t i = 0; i < array->length; i++) {
    if (ArrowArrayViewIsNull(array_view, i)) {
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_start(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.null_feat(&private_data->v));
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_end(&private_data->v));
    } else {
      struct ArrowStringView value = ArrowArrayViewGetStringUnsafe(array_view, i);
      buffer_view.data = value.data;
      buffer_view.size_bytes = value.size_bytes;
      int result = GeoArrowWKTReaderVisit(&private_data->wkt_reader, buffer_view,
                                          &private_data->v);
      if (result == EAGAIN) {
        NANOARROW_RETURN_NOT_OK(private_data->v.feat_end(&private_data->v));
      } else if (result != NANOARROW_OK) {
        return result;
      }
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

static int kernel_push_batch_geoarrow_by_feature(struct GeoArrowKernel* kernel,
                                                 struct ArrowArray* array,
                                                 struct ArrowArray* out,
                                                 struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;

  NANOARROW_RETURN_NOT_OK(
      GeoArrowArrayViewSetArray(&private_data->array_view, array, error));

  private_data->v.error = error;

  for (int64_t i = 0; i < array->length; i++) {
    int result =
        GeoArrowArrayViewVisit(&private_data->array_view, i, 1, &private_data->v);

    if (result == EAGAIN) {
      NANOARROW_RETURN_NOT_OK(private_data->v.feat_end(&private_data->v));
    } else if (result != NANOARROW_OK) {
      return result;
    }
  }

  return private_data->finish_push_batch(private_data, out, error);
}

static int kernel_visitor_start(struct GeoArrowKernel* kernel, struct ArrowSchema* schema,
                                const char* options, struct ArrowSchema* out,
                                struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;

  struct GeoArrowSchemaView schema_view;
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInit(&schema_view, schema, error));

  switch (schema_view.type) {
    case GEOARROW_TYPE_UNINITIALIZED:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_LARGE_WKT:
      return EINVAL;
    case GEOARROW_TYPE_WKT:
      kernel->push_batch = &kernel_push_batch_wkt;
      NANOARROW_RETURN_NOT_OK(GeoArrowWKTReaderInit(&private_data->wkt_reader));
      ArrowArrayViewInitFromType(&private_data->na_array_view, NANOARROW_TYPE_STRING);
      break;
    case GEOARROW_TYPE_WKB:
      kernel->push_batch = &kernel_push_batch_wkb;
      GeoArrowWKBReaderInit(&private_data->wkb_reader);
      ArrowArrayViewInitFromType(&private_data->na_array_view, NANOARROW_TYPE_BINARY);
      break;
    default:
      if (private_data->visit_by_feature) {
        kernel->push_batch = &kernel_push_batch_geoarrow_by_feature;
      } else {
        kernel->push_batch = &kernel_push_batch_geoarrow;
      }
      NANOARROW_RETURN_NOT_OK(
          GeoArrowArrayViewInitFromType(&private_data->array_view, schema_view.type));
      break;
  }

  return private_data->finish_start(private_data, schema, options, out, error);
}

// Kernel visit_void_agg
//
// This kernel visits every feature and returns a single null item at the end.
// This is useful for (1) testing and (2) validating well-known text or well-known
// binary.

static int finish_start_visit_void_agg(struct GeoArrowVisitorKernelPrivate* private_data,
                                       struct ArrowSchema* schema, const char* options,
                                       struct ArrowSchema* out,
                                       struct GeoArrowError* error) {
  return ArrowSchemaInitFromType(out, NANOARROW_TYPE_NA);
}

// Kernels as_wkt and format_wkt
//
// Visits every feature in the input and writes the corresponding well-known text output.
// For the format_wkt kernel, optionally specify significant_digits and
// max_element_size_bytes.

static int finish_start_as_wkt(struct GeoArrowVisitorKernelPrivate* private_data,
                               struct ArrowSchema* schema, const char* options,
                               struct ArrowSchema* out, struct GeoArrowError* error) {
  GeoArrowWKTWriterInitVisitor(&private_data->wkt_writer, &private_data->v);

  struct ArrowSchema tmp;
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitExtension(&tmp, GEOARROW_TYPE_WKT));
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaSetMetadataFrom(&tmp, schema));
  ArrowSchemaMove(&tmp, out);

  return GEOARROW_OK;
}

static int finish_start_format_wkt(struct GeoArrowVisitorKernelPrivate* private_data,
                                   struct ArrowSchema* schema, const char* options,
                                   struct ArrowSchema* out, struct GeoArrowError* error) {
  long significant_digits = private_data->wkt_writer.significant_digits;
  NANOARROW_RETURN_NOT_OK(
      kernel_get_arg_long(options, "significant_digits", &significant_digits, 0, error));
  private_data->wkt_writer.significant_digits = significant_digits;

  long max_element_size_bytes = private_data->wkt_writer.max_element_size_bytes;
  NANOARROW_RETURN_NOT_OK(kernel_get_arg_long(options, "max_element_size_bytes",
                                              &max_element_size_bytes, 0, error));
  private_data->wkt_writer.max_element_size_bytes = max_element_size_bytes;

  GeoArrowWKTWriterInitVisitor(&private_data->wkt_writer, &private_data->v);

  struct ArrowSchema tmp;
  NANOARROW_RETURN_NOT_OK(ArrowSchemaInitFromType(&tmp, NANOARROW_TYPE_STRING));
  ArrowSchemaMove(&tmp, out);

  return GEOARROW_OK;
}

static int finish_push_batch_as_wkt(struct GeoArrowVisitorKernelPrivate* private_data,
                                    struct ArrowArray* out, struct GeoArrowError* error) {
  return GeoArrowWKTWriterFinish(&private_data->wkt_writer, out, error);
}

// Kernel as_wkb
//
// Visits every feature in the input and writes the corresponding well-known binary output

static int finish_start_as_wkb(struct GeoArrowVisitorKernelPrivate* private_data,
                               struct ArrowSchema* schema, const char* options,
                               struct ArrowSchema* out, struct GeoArrowError* error) {
  struct ArrowSchema tmp;
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitExtension(&tmp, GEOARROW_TYPE_WKB));
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaSetMetadataFrom(&tmp, schema));
  ArrowSchemaMove(&tmp, out);

  return GEOARROW_OK;
}

static int finish_push_batch_as_wkb(struct GeoArrowVisitorKernelPrivate* private_data,
                                    struct ArrowArray* out, struct GeoArrowError* error) {
  return GeoArrowWKBWriterFinish(&private_data->wkb_writer, out, error);
}

// Kernel as_geoarrow
//
// Visits every feature in the input and writes a geoarrow-encoded array.
// Takes option 'type' as the desired integer enum GeoArrowType.

static int finish_start_as_geoarrow(struct GeoArrowVisitorKernelPrivate* private_data,
                                    struct ArrowSchema* schema, const char* options,
                                    struct ArrowSchema* out,
                                    struct GeoArrowError* error) {
  long out_type_long;
  NANOARROW_RETURN_NOT_OK(kernel_get_arg_long(options, "type", &out_type_long, 1, error));
  enum GeoArrowType out_type = (enum GeoArrowType)out_type_long;

  if (out_type != private_data->builder.view.schema_view.type) {
    GeoArrowBuilderReset(&private_data->builder);
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderInitFromType(&private_data->builder, out_type));
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderInitVisitor(&private_data->builder, &private_data->v));
  }

  struct ArrowSchema tmp;
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitExtension(&tmp, out_type));
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaSetMetadataFrom(&tmp, schema));
  ArrowSchemaMove(&tmp, out);

  return GEOARROW_OK;
}

static int finish_push_batch_as_geoarrow(
    struct GeoArrowVisitorKernelPrivate* private_data, struct ArrowArray* out,
    struct GeoArrowError* error) {
  // The GeoArrowBuilder is not currently designed to visit + finish more than
  // once and until it is, we have to reset + reinit here.
  int result = GeoArrowBuilderFinish(&private_data->builder, out, error);
  enum GeoArrowType type = private_data->builder.view.schema_view.type;
  GeoArrowBuilderReset(&private_data->builder);
  int result2 = GeoArrowBuilderInitFromType(&private_data->builder, type);
  if (result != GEOARROW_OK) {
    return result;
  } else if (result2 != GEOARROW_OK) {
    return result2;
  } else {
    return GEOARROW_OK;
  }
}

// Kernel unique_geometry_types_agg
//
// This kernel collects all geometry type + dimension combinations in the
// input. EMPTY values are not counted as any particular geometry type;
// however, note that POINTs as represented in WKB or GeoArrow cannot be
// EMPTY and this kernel does not check for the convention of EMPTY as
// all coordinates == nan. This is mosty to facilitate choosing an appropriate destination
// type (e.g., point, linestring, etc.). This is the only kernel whose visitor is not
// exposed as a standalone visitor in the geoarrow.h header.
//
// The internals use GeoArrowDimensions * 8 + GeoArrowGeometryType as the
// "key" for a given combination. This gives an integer between 0 and 39.
// The types are accumulated in a uint64_t bitmask and translated into the
// corresponding ISO WKB type codes at the end.
static int32_t kGeoArrowGeometryTypeWkbValues[] = {
    -1000, -999, -998, -997, -996, -995, -994, -993, 0,    1,    2,    3,    4,    5,
    6,     7,    1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 2000, 2001, 2002, 2003,
    2004,  2005, 2006, 2007, 3000, 3001, 3002, 3003, 3004, 3005, 3006, 3007};

static int feat_start_geometry_types(struct GeoArrowVisitor* v) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)v->private_data;
  private_data->geometry_types_private.geometry_type = GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  private_data->geometry_types_private.dimensions = GEOARROW_DIMENSIONS_UNKNOWN;
  return GEOARROW_OK;
}

static int geom_start_geometry_types(struct GeoArrowVisitor* v,
                                     enum GeoArrowGeometryType geometry_type,
                                     enum GeoArrowDimensions dimensions) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)v->private_data;

  // Only record the first seen geometry type/dimension combination
  if (private_data->geometry_types_private.geometry_type ==
      GEOARROW_GEOMETRY_TYPE_GEOMETRY) {
    private_data->geometry_types_private.geometry_type = geometry_type;
    private_data->geometry_types_private.dimensions = dimensions;
  }

  return GEOARROW_OK;
}

static int coords_geometry_types(struct GeoArrowVisitor* v,
                                 const struct GeoArrowCoordView* coords) {
  if (coords->n_coords > 0) {
    struct GeoArrowVisitorKernelPrivate* private_data =
        (struct GeoArrowVisitorKernelPrivate*)v->private_data;

    // At the first coordinate, add the geometry type to the bitmask
    int bitshift = private_data->geometry_types_private.dimensions * 8 +
                   private_data->geometry_types_private.geometry_type;
    uint64_t bitmask = ((uint64_t)1) << bitshift;
    private_data->geometry_types_private.geometry_types_mask |= bitmask;
    return EAGAIN;
  } else {
    return GEOARROW_OK;
  }
}

static int finish_start_unique_geometry_types_agg(
    struct GeoArrowVisitorKernelPrivate* private_data, struct ArrowSchema* schema,
    const char* options, struct ArrowSchema* out, struct GeoArrowError* error) {
  private_data->v.feat_start = &feat_start_geometry_types;
  private_data->v.geom_start = &geom_start_geometry_types;
  private_data->v.coords = &coords_geometry_types;
  private_data->v.private_data = private_data;
  return ArrowSchemaInitFromType(out, NANOARROW_TYPE_INT32);
}

static int kernel_finish_unique_geometry_types_agg(struct GeoArrowKernel* kernel,
                                                   struct ArrowArray* out,
                                                   struct GeoArrowError* error) {
  struct GeoArrowVisitorKernelPrivate* private_data =
      (struct GeoArrowVisitorKernelPrivate*)kernel->private_data;
  uint64_t result_mask = private_data->geometry_types_private.geometry_types_mask;

  int n_types = 0;
  for (int i = 0; i < 40; i++) {
    uint64_t bitmask = ((uint64_t)1) << i;
    n_types += (result_mask & bitmask) != 0;
  }

  struct ArrowArray tmp;
  NANOARROW_RETURN_NOT_OK(ArrowArrayInitFromType(&tmp, NANOARROW_TYPE_INT32));
  struct ArrowBuffer* data = ArrowArrayBuffer(&tmp, 1);
  int result = ArrowBufferReserve(data, n_types * sizeof(int32_t));
  if (result != NANOARROW_OK) {
    tmp.release(&tmp);
    return result;
  }

  int result_i = 0;
  int32_t* data_int32 = (int32_t*)data->data;
  for (int i = 0; i < 40; i++) {
    uint64_t bitmask = ((uint64_t)1) << i;
    if (result_mask & bitmask) {
      data_int32[result_i++] = kGeoArrowGeometryTypeWkbValues[i];
    }
  }

  result = ArrowArrayFinishBuilding(&tmp, NULL);
  if (result != NANOARROW_OK) {
    tmp.release(&tmp);
    return result;
  }

  tmp.length = n_types;
  tmp.null_count = 0;
  ArrowArrayMove(&tmp, out);
  return GEOARROW_OK;
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
  GeoArrowVisitorInitVoid(&private_data->v);
  private_data->visit_by_feature = 0;

  if (strcmp(name, "visit_void_agg") == 0) {
    kernel->finish = &kernel_finish_void_agg;
    private_data->finish_start = &finish_start_visit_void_agg;
  } else if (strcmp(name, "as_wkt") == 0) {
    kernel->finish = &kernel_finish_void;
    private_data->finish_start = &finish_start_as_wkt;
    private_data->finish_push_batch = &finish_push_batch_as_wkt;
    NANOARROW_RETURN_NOT_OK(GeoArrowWKTWriterInit(&private_data->wkt_writer));
  } else if (strcmp(name, "format_wkt") == 0) {
    kernel->finish = &kernel_finish_void;
    private_data->finish_start = &finish_start_format_wkt;
    private_data->finish_push_batch = &finish_push_batch_as_wkt;
    NANOARROW_RETURN_NOT_OK(GeoArrowWKTWriterInit(&private_data->wkt_writer));
    private_data->visit_by_feature = 1;
  } else if (strcmp(name, "as_wkb") == 0) {
    kernel->finish = &kernel_finish_void;
    private_data->finish_start = &finish_start_as_wkb;
    private_data->finish_push_batch = &finish_push_batch_as_wkb;
    NANOARROW_RETURN_NOT_OK(GeoArrowWKBWriterInit(&private_data->wkb_writer));
    GeoArrowWKBWriterInitVisitor(&private_data->wkb_writer, &private_data->v);
  } else if (strcmp(name, "as_geoarrow") == 0) {
    kernel->finish = &kernel_finish_void;
    private_data->finish_start = &finish_start_as_geoarrow;
    private_data->finish_push_batch = &finish_push_batch_as_geoarrow;
  } else if (strcmp(name, "unique_geometry_types") == 0) {
    kernel->finish = &kernel_finish_unique_geometry_types_agg;
    private_data->finish_start = &finish_start_unique_geometry_types_agg;
    private_data->visit_by_feature = 1;
  }

  kernel->start = &kernel_visitor_start;
  kernel->push_batch = &kernel_push_batch_void_agg;
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
  } else if (strcmp(name, "visit_void_agg") == 0) {
    return GeoArrowInitVisitorKernelInternal(kernel, name);
  } else if (strcmp(name, "as_wkt") == 0) {
    return GeoArrowInitVisitorKernelInternal(kernel, name);
  } else if (strcmp(name, "format_wkt") == 0) {
    return GeoArrowInitVisitorKernelInternal(kernel, name);
  } else if (strcmp(name, "as_wkb") == 0) {
    return GeoArrowInitVisitorKernelInternal(kernel, name);
  } else if (strcmp(name, "as_geoarrow") == 0) {
    return GeoArrowInitVisitorKernelInternal(kernel, name);
  } else if (strcmp(name, "unique_geometry_types") == 0) {
    return GeoArrowInitVisitorKernelInternal(kernel, name);
  }

  return ENOTSUP;
}
