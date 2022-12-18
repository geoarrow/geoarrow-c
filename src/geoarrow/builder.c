
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
  struct ArrowBuffer* buffers[8];

  // To avoid one additional layer of private_data, these are fields
  // needed by various builder types.

  // Fields to keep track of states when using the visitor pattern
  enum GeoArrowGeometryType geometry_type[32];
  enum GeoArrowDimensions dimensions[32];
  int64_t size_pos[32];
  uint32_t size[32];
  int32_t level;

  // Options
  int significant_digits;
  int use_flat_multipoint;
};

static GeoArrowErrorCode GeoArrowBuilderInitInternal(struct GeoArrowBuilder* builder) {
  enum GeoArrowType type = builder->view.schema_view.type;

  // Initialize an array view to help set some fields
  struct GeoArrowArrayView array_view;
  NANOARROW_RETURN_NOT_OK(GeoArrowArrayViewInitFromType(&array_view, type));

  struct BuilderPrivate* private =
      (struct BuilderPrivate*)ArrowMalloc(sizeof(struct BuilderPrivate));
  if (private == NULL) {
    return ENOMEM;
  }

  memset(private, 0, sizeof(struct BuilderPrivate));

  struct ArrowSchema schema;
  int result = GeoArrowSchemaInit(&schema, type);
  if (result != GEOARROW_OK) {
    ArrowFree(private);
    return result;
  }

  result = ArrowArrayInitFromSchema(&private->array, &schema, NULL);
  schema.release(&schema);
  if (result != GEOARROW_OK) {
    ArrowFree(private);
    return result;
  }

  // Update a few things about the writable view from the regular view
  builder->view.coords.n_values = array_view.coords.n_values;
  builder->view.coords.coords_stride = array_view.coords.coords_stride;
  switch (builder->view.schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_SEPARATE:
      builder->view.n_buffers = 1 + array_view.n_offsets + array_view.coords.n_values;
      break;

    // interleaved + WKB + WKT
    default:
      builder->view.n_buffers = 1 + array_view.n_offsets + 1;
      break;
  }

  // Cache the ArrowBitmap and ArrowBuffer pointers we need to allocate
  private->validity = ArrowArrayValidityBitmap(&private->array);

  struct _GeoArrowFindBufferResult res;
  for (int64_t i = 0; i < builder->view.n_buffers; i++) {
    res.array = NULL;
    _GeoArrowArrayFindBuffer(&private->array, &res, i, 0, 0);
    if (res.array == NULL) {
      ArrowFree(private);
      return EINVAL;
    }

    private->buffers[i] = ArrowArrayBuffer(res.array, res.i);
  }

  // Some default options
  private->significant_digits = 16;
  private->use_flat_multipoint = 1;

  builder->private_data = private;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowBuilderInitFromType(struct GeoArrowBuilder* builder,
                                              enum GeoArrowType type) {
  memset(builder, 0, sizeof(struct GeoArrowBuilder));
  NANOARROW_RETURN_NOT_OK(
      GeoArrowSchemaViewInitFromType(&builder->view.schema_view, type));
  return GeoArrowBuilderInitInternal(builder);
}

GeoArrowErrorCode GeoArrowBuilderInitFromSchema(struct GeoArrowBuilder* builder,
                                                struct ArrowSchema* schema,
                                                struct GeoArrowError* error) {
  memset(builder, 0, sizeof(struct GeoArrowBuilder));
  NANOARROW_RETURN_NOT_OK(
      GeoArrowSchemaViewInit(&builder->view.schema_view, schema, error));
  return GeoArrowBuilderInitInternal(builder);
}

void GeoArrowBuilderInitVisitor(struct GeoArrowBuilder* builder,
                                struct GeoArrowVisitor* v) {
  return;
}

GeoArrowErrorCode GeoArrowBuilderFinish(struct GeoArrowBuilder* builder,
                                        struct ArrowArray* array,
                                        struct GeoArrowError* error) {
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayFinishBuilding(&private->array, (struct ArrowError*)error));
  memcpy(array, &private->array, sizeof(struct ArrowArray));
  private->array.release = NULL;
  return GEOARROW_OK;
}

void GeoArrowBuilderReset(struct GeoArrowBuilder* builder) {
  if (builder->private_data != NULL) {
    struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
    if (private->array.release != NULL) {
      private->array.release(&private->array);
    }

    ArrowFree(private);
    builder->private_data = NULL;
  }
}
