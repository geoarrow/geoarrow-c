
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

// Bytes for four quiet (little-endian) NANs
static uint8_t kEmptyPointCoords[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};

struct BuilderPrivate {
  // The ArrowArray responsible for owning the memory
  struct ArrowArray array;

  // Cached pointers pointing inside the array's private data
  // Depending on what exactly is being built, these pointers
  // might be NULL.
  struct ArrowBitmap* validity;
  struct ArrowBuffer* buffers[8];

  // Fields to keep track of state when using the visitor pattern
  int64_t length;
  int64_t null_count;
  double empty_coord_values[4];
  struct GeoArrowCoordView empty_coord;
  enum GeoArrowDimensions last_dimensions;
  int64_t size[32];
  int32_t level;
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

  // Initalize one empty coordinate for the visitor pattern
  memcpy(private->empty_coord_values, kEmptyPointCoords, 4 * sizeof(double));
  private->empty_coord.values[0] = private->empty_coord_values;
  private->empty_coord.values[1] = private->empty_coord_values + 1;
  private->empty_coord.values[2] = private->empty_coord_values + 2;
  private->empty_coord.values[3] = private->empty_coord_values + 3;
  private->empty_coord.n_coords = 1;
  private->empty_coord.n_values = 4;
  private->empty_coord.coords_stride = 1;

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
  int last_buffer = builder->view.n_buffers - 1;
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
      for (int i = last_buffer - n_values + 1; i <= last_buffer; i++) {
        size_by_coords = writable_view->size_coords * sizeof(double);
        if (size_by_coords > builder->view.buffers[i].size_bytes) {
          builder->view.buffers[i].size_bytes = size_by_coords;
        }
      }
      break;

    default:
      return EINVAL;
  }

  // If the validity bitmap was used, we may need to update the validity buffer size
  if (private->validity->buffer.data != NULL &&
      builder->view.buffers[0].data.data == NULL) {
    builder->view.buffers[0].data.as_uint8 = private->validity->buffer.data;
    builder->view.buffers[0].size_bytes = private->validity->buffer.size_bytes;
    builder->view.buffers[0].capacity_bytes = private->validity->buffer.capacity_bytes;
  }

  // Sync builder buffer's back to the array; set array lengths from buffer sizes
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
      ArrowArrayFinishBuilding(&private->array, (struct ArrowError*)error));

  // Move the result
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

static void GeoArrowSetArrayLengthFromBufferLength(struct GeoArrowSchemaView* schema_view,
                                                   struct _GeoArrowFindBufferResult* res,
                                                   int64_t size_bytes) {
  // By luck, buffer index 1 for every array is the one we use to infer the length;
  // however, this is a slightly different formula for each type/depth
  if (res->i != 1) {
    return;
  }

  switch (schema_view->type) {
    case GEOARROW_TYPE_WKB:
      res->array->length = (size_bytes / sizeof(int32_t)) - 1;
      return;
    case GEOARROW_TYPE_LARGE_WKB:
      res->array->length = (size_bytes / sizeof(int64_t)) - 1;
      return;
    default:
      break;
  }

  int coord_level;
  switch (schema_view->geometry_type) {
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
  int scale;
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
    case GEOARROW_GEOMETRY_TYPE_POINT:
      private
      ->array.length = private->array.children[0]->length;
      break;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      private
      ->array.children[0]->length = private->array.children[0]->children[0]->length;
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      private
      ->array.children[0]->children[0]->length =
          private->array.children[0]->children[0]->children[0]->length;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      private
      ->array.children[0]->children[0]->children[0]->length =
          private->array.children[0]->children[0]->children[0]->children[0]->length;
      break;
    default:
      // e.g., WKB
      break;
  }
}

static int feat_start_point(struct GeoArrowVisitor* v) {
  struct GeoArrowBuilder* builder = (struct GeoArrowBuilder*)v->private_data;
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
  private->level = 0;
  private->size[0] = 0;
  return GEOARROW_OK;
}

static int geom_start_point(struct GeoArrowVisitor* v,
                            enum GeoArrowGeometryType geometry_type,
                            enum GeoArrowDimensions dimensions) {
  // level++, geometry type, dimensions, reset size
  // validate dimensions, maybe against some options that indicate
  // error for mismatch, fill, or drop behaviour
  struct GeoArrowBuilder* builder = (struct GeoArrowBuilder*)v->private_data;
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
  private->last_dimensions = builder->view.schema_view.dimensions;
  return GEOARROW_OK;
}

static int ring_start_point(struct GeoArrowVisitor* v) { return GEOARROW_OK; }

static int coords_point(struct GeoArrowVisitor* v,
                        const struct GeoArrowCoordView* coords) {
  struct GeoArrowBuilder* builder = (struct GeoArrowBuilder*)v->private_data;
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;
  private->size[0] += coords->n_coords;
  return GeoArrowBuilderCoordsAppend(builder, coords, private->last_dimensions, 0,
                                     coords->n_coords);
}

static int ring_end_point(struct GeoArrowVisitor* v) { return GEOARROW_OK; }

static int geom_end_point(struct GeoArrowVisitor* v) { return GEOARROW_OK; }

static int null_feat_point(struct GeoArrowVisitor* v) {
  struct GeoArrowBuilder* builder = (struct GeoArrowBuilder*)v->private_data;
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;

  int n_dim = _GeoArrowkNumDimensions[builder->view.schema_view.dimensions];
  private->empty_coord.n_values = n_dim;
  NANOARROW_RETURN_NOT_OK(coords_point(v, &private->empty_coord));

  int64_t current_length = builder->view.coords.size_coords;
  if (private->validity->buffer.data == NULL && current_length > 0) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(private->validity, current_length));
    ArrowBitmapAppendUnsafe(private->validity, 1, current_length - 1);
  }

  NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(private->validity, 0, 1));

  return GEOARROW_OK;
}

static int feat_end_point(struct GeoArrowVisitor* v) {
  struct GeoArrowBuilder* builder = (struct GeoArrowBuilder*)v->private_data;
  struct BuilderPrivate* private = (struct BuilderPrivate*)builder->private_data;

  // If there weren't any coords (i.e., EMPTY), we need to write some NANs here
  // if there was >1 coords, we also need to error or we'll get misaligned output
  if (private->size[0] == 0) {
    int n_dim = _GeoArrowkNumDimensions[builder->view.schema_view.dimensions];
    private->empty_coord.n_values = n_dim;
    NANOARROW_RETURN_NOT_OK(coords_point(v, &private->empty_coord));
  } else if (private->size[0] != 1) {
    ArrowErrorSet((struct ArrowError*)v->error,
                  "Can't convert feature with >1 coordinate to POINT");
    return EINVAL;
  }

  return GEOARROW_OK;
}

static void GeoArrowVisitorInitPoint(struct GeoArrowBuilder* builder,
                                     struct GeoArrowVisitor* v) {
  struct GeoArrowError* previous_error = v->error;
  GeoArrowVisitorInitVoid(v);
  v->error = previous_error;

  v->feat_start = &feat_start_point;
  v->null_feat = &null_feat_point;
  v->geom_start = &geom_start_point;
  v->ring_start = &ring_start_point;
  v->coords = &coords_point;
  v->ring_end = &ring_end_point;
  v->geom_end = &geom_end_point;
  v->feat_end = &feat_end_point;
  v->private_data = builder;
}

void GeoArrowBuilderInitVisitor(struct GeoArrowBuilder* builder,
                                struct GeoArrowVisitor* v) {
  switch (builder->view.schema_view.geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      GeoArrowVisitorInitPoint(builder, v);
      break;
    default:
      break;
  }
}
