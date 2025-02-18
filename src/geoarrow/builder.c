
#include <string.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

struct BuilderPrivate {
  // The ArrowSchema (without extension) for this builder
  struct ArrowSchema schema;

  // The ArrowArray responsible for owning the memory
  struct ArrowArray array;

  // Cached pointers pointing inside the array's private data
  // Depending on what exactly is being built, these pointers
  // might be NULL.
  struct ArrowBitmap* validity;
  struct ArrowBuffer* buffers[9];
};

static ArrowErrorCode GeoArrowBuilderInitArrayAndCachePointers(
    struct GeoArrowBuilder* builder) {
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;

  NANOARROW_RETURN_NOT_OK(
      ArrowArrayInitFromSchema(&private->array, &private->schema, NULL));

  private->validity = ArrowArrayValidityBitmap(&private->array);

  struct _GeoArrowFindBufferResult res;
  for (int64_t i = 0; i < builder->view.n_buffers; i++) {
    res.array = NULL;
    _GeoArrowArrayFindBuffer(&private->array, &res, i, 0, 0);
    if (res.array == NULL) {
      return EINVAL;
    }

    private->buffers[i] = ArrowArrayBuffer(res.array, res.i);
    builder->view.buffers[i].data.as_uint8 = NULL;
    builder->view.buffers[i].size_bytes = 0;
    builder->view.buffers[i].capacity_bytes = 0;
  }

  // Reset the coordinate counts and values
  builder->view.coords.size_coords = 0;
  builder->view.coords.capacity_coords = 0;
  for (int i = 0; i < 4; i++) {
    builder->view.coords.values[i] = NULL;
  }

  return GEOARROW_OK;
}

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
  builder->private_data = private;

  // Initialize our copy of the schema for the storage type
  int result = GeoArrowSchemaInit(&private->schema, type);
  if (result != GEOARROW_OK) {
    ArrowFree(private);
    builder->private_data = NULL;
    return result;
  }

  // Update a few things about the writable view from the regular view
  // that never change.
  builder->view.coords.n_values = array_view.coords.n_values;
  builder->view.coords.coords_stride = array_view.coords.coords_stride;
  builder->view.n_offsets = array_view.n_offsets;
  switch (builder->view.schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_SEPARATE:
      builder->view.n_buffers = 1 + array_view.n_offsets + array_view.coords.n_values;
      break;

    // interleaved + WKB + WKT
    default:
      builder->view.n_buffers = 1 + array_view.n_offsets + 1;
      break;
  }

  // Initialize an empty array; cache the ArrowBitmap and ArrowBuffer pointers we need
  result = GeoArrowBuilderInitArrayAndCachePointers(builder);
  if (result != GEOARROW_OK) {
    private->schema.release(&private->schema);
    ArrowFree(private);
    builder->private_data = NULL;
    return result;
  }

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
                                                const struct ArrowSchema* schema,
                                                struct GeoArrowError* error) {
  memset(builder, 0, sizeof(struct GeoArrowBuilder));
  NANOARROW_RETURN_NOT_OK(
      GeoArrowSchemaViewInit(&builder->view.schema_view, schema, error));
  return GeoArrowBuilderInitInternal(builder);
}

GeoArrowErrorCode GeoArrowBuilderReserveBuffer(struct GeoArrowBuilder* builder, int64_t i,
                                               int64_t additional_size_bytes) {
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
  struct ArrowBuffer* buffer_src = private->buffers[i];
  struct GeoArrowWritableBufferView* buffer_dst = builder->view.buffers + i;

  // Sync any changes from the builder's view of the buffer to nanoarrow's
  buffer_src->size_bytes = buffer_dst->size_bytes;

  // Use nanoarrow's reserve
  NANOARROW_RETURN_NOT_OK(ArrowBufferReserve(buffer_src, additional_size_bytes));

  // Sync any changes back to the builder's view
  builder->view.buffers[i].data.data = buffer_src->data;
  builder->view.buffers[i].capacity_bytes = buffer_src->capacity_bytes;
  return GEOARROW_OK;
}

struct GeoArrowBufferDeallocatorPrivate {
  void (*custom_free)(uint8_t* ptr, int64_t size, void* private_data);
  void* private_data;
};

static void GeoArrowBufferDeallocateWrapper(struct ArrowBufferAllocator* allocator,
                                            uint8_t* ptr, int64_t size) {
  struct GeoArrowBufferDeallocatorPrivate* private_data =
      (struct GeoArrowBufferDeallocatorPrivate*)allocator->private_data;
  private_data->custom_free(ptr, size, private_data->private_data);
  ArrowFree(private_data);
}

GeoArrowErrorCode GeoArrowBuilderSetOwnedBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value,
    void (*custom_free)(uint8_t* ptr, int64_t size, void* private_data),
    void* private_data) {
  if (i < 0 || i >= builder->view.n_buffers) {
    return EINVAL;
  }

  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
  struct ArrowBuffer* buffer_src = private->buffers[i];

  struct GeoArrowBufferDeallocatorPrivate* deallocator =
      (struct GeoArrowBufferDeallocatorPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowBufferDeallocatorPrivate));
  if (deallocator == NULL) {
    return ENOMEM;
  }

  deallocator->custom_free = custom_free;
  deallocator->private_data = private_data;

  ArrowBufferReset(buffer_src);
  buffer_src->allocator =
      ArrowBufferDeallocator(&GeoArrowBufferDeallocateWrapper, deallocator);
  buffer_src->data = (uint8_t*)value.data;
  buffer_src->size_bytes = value.size_bytes;
  buffer_src->capacity_bytes = value.size_bytes;

  // Sync this information to the writable view
  builder->view.buffers[i].data.data = buffer_src->data;
  builder->view.buffers[i].size_bytes = buffer_src->size_bytes;
  builder->view.buffers[i].capacity_bytes = buffer_src->capacity_bytes;

  return GEOARROW_OK;
}

static void GeoArrowSetArrayLengthFromBufferLength(struct GeoArrowSchemaView* schema_view,
                                                   struct _GeoArrowFindBufferResult* res,
                                                   int64_t size_bytes);

static void GeoArrowSetCoordContainerLength(struct GeoArrowBuilder* builder);

GeoArrowErrorCode GeoArrowBuilderFinish(struct GeoArrowBuilder* builder,
                                        struct ArrowArray* array,
                                        struct GeoArrowError* error) {
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;

  // If the coordinate appender was used, we may need to update the buffer sizes
  struct GeoArrowWritableCoordView* writable_view = &builder->view.coords;
  int64_t last_buffer = builder->view.n_buffers - 1;
  int n_values = writable_view->n_values;
  int64_t size_by_coords;

  switch (builder->view.schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      size_by_coords = writable_view->size_coords * sizeof(double) * n_values;
      if (size_by_coords > builder->view.buffers[last_buffer].size_bytes) {
        builder->view.buffers[last_buffer].size_bytes = size_by_coords;
      }
      break;

    case GEOARROW_COORD_TYPE_SEPARATE:
      for (int64_t i = last_buffer - n_values + 1; i <= last_buffer; i++) {
        size_by_coords = writable_view->size_coords * sizeof(double);
        if (size_by_coords > builder->view.buffers[i].size_bytes) {
          builder->view.buffers[i].size_bytes = size_by_coords;
        }
      }
      break;

    default:
      return EINVAL;
  }

  // If the validity bitmap was used, we need to update the validity buffer size
  if (private->validity->buffer.data != NULL &&
      builder->view.buffers[0].data.data == NULL) {
    builder->view.buffers[0].data.as_uint8 = private->validity->buffer.data;
    builder->view.buffers[0].size_bytes = private->validity->buffer.size_bytes;
    builder->view.buffers[0].capacity_bytes = private->validity->buffer.capacity_bytes;
  }

  // Sync builder's buffers back to the array; set array lengths from buffer sizes
  struct _GeoArrowFindBufferResult res;
  for (int64_t i = 0; i < builder->view.n_buffers; i++) {
    private->buffers[i]->size_bytes = builder->view.buffers[i].size_bytes;

    res.array = NULL;
    _GeoArrowArrayFindBuffer(&private->array, &res, i, 0, 0);
    if (res.array == NULL) {
      return EINVAL;
    }

    GeoArrowSetArrayLengthFromBufferLength(&builder->view.schema_view, &res,
                                           private->buffers[i]->size_bytes);
  }

  // Set the struct or fixed-size list container length
  GeoArrowSetCoordContainerLength(builder);

  // Call finish building, which will flush the buffer pointers into the array
  // and validate sizes.
  NANOARROW_RETURN_NOT_OK(
      ArrowArrayFinishBuildingDefault(&private->array, (struct ArrowError*)error));

  // If the first buffer is non-null, we don't know what it is
  if (private->array.buffers[0] != NULL) {
    private->array.null_count = -1;
  }

  // Move the result out of private so we can maybe prepare for the next round
  struct ArrowArray tmp;
  ArrowArrayMove(&private->array, &tmp);

  // Prepare for another round of building
  int result = GeoArrowBuilderInitArrayAndCachePointers(builder);
  if (result != GEOARROW_OK) {
    tmp.release(&tmp);
    return result;
  }

  // Move the result
  ArrowArrayMove(&tmp, array);
  return GEOARROW_OK;
}

void GeoArrowBuilderReset(struct GeoArrowBuilder* builder) {
  if (builder->private_data != NULL) {
    struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;

    if (private->schema.release != NULL) {
      private->schema.release(&private->schema);
    }

    if (private->array.release != NULL) {
      private->array.release(&private->array);
    }

    ArrowFree(private);
    builder->private_data = NULL;
  }
}

static void GeoArrowSetArrayLengthFromBufferLength(struct GeoArrowSchemaView* schema_view,
                                                   struct _GeoArrowFindBufferResult* res,
                                                   int64_t size_bytes) {
  // By luck, buffer index 1 for every array is the one we use to infer the length;
  // however, this is a slightly different formula for each type/depth
  if (res->i != 1) {
    return;
  }

  // ...but in all cases, if the size is 0, the length is 0
  if (size_bytes == 0) {
    res->array->length = 0;
    return;
  }

  switch (schema_view->type) {
    case GEOARROW_TYPE_WKB:
    case GEOARROW_TYPE_WKT:
      res->array->length = (size_bytes / sizeof(int32_t)) - 1;
      return;
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_LARGE_WKT:
      res->array->length = (size_bytes / sizeof(int64_t)) - 1;
      return;
    default:
      break;
  }

  int coord_level;
  switch (schema_view->geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_BOX:
    case GEOARROW_GEOMETRY_TYPE_POINT:
      coord_level = 0;
      break;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      coord_level = 1;
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      coord_level = 2;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      coord_level = 3;
      break;
    default:
      return;
  }

  if (res->level < coord_level) {
    // This is an offset buffer
    res->array->length = (size_bytes / sizeof(int32_t)) - 1;
  } else {
    // This is a data buffer
    res->array->length = size_bytes / sizeof(double);
  }
}

static void GeoArrowSetCoordContainerLength(struct GeoArrowBuilder* builder) {
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;

  // At this point all the array lengths should be set except for the
  // fixed-size list or struct parent to the coordinate array(s).
  int scale = -1;
  switch (builder->view.schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_SEPARATE:
      scale = 1;
      break;
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      switch (builder->view.schema_view.dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          scale = 2;
          break;
        case GEOARROW_DIMENSIONS_XYZ:
        case GEOARROW_DIMENSIONS_XYM:
          scale = 3;
          break;
        case GEOARROW_DIMENSIONS_XYZM:
          scale = 4;
          break;
        default:
          return;
      }
      break;
    default:
      // e.g., WKB
      break;
  }

  switch (builder->view.schema_view.geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_BOX:
    case GEOARROW_GEOMETRY_TYPE_POINT:
      private
      ->array.length = private->array.children[0]->length / scale;
      break;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      private
      ->array.children[0]->length =
          private->array.children[0]->children[0]->length / scale;
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      private
      ->array.children[0]->children[0]->length =
          private->array.children[0]->children[0]->children[0]->length / scale;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      private
      ->array.children[0]->children[0]->children[0]->length =
          private->array.children[0]->children[0]->children[0]->children[0]->length /
          scale;
      break;
    default:
      // e.g., WKB
      break;
  }
}
