
#include <string.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

// Bytes for four quiet (little-endian) NANs
static uint8_t kEmptyPointCoords[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f,
                                      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};

struct GeoArrowNativeWriterPrivate {
  struct GeoArrowBuilder builder;

  struct ArrowBitmap validity;
  int64_t null_count;

  // Fields to keep track of state
  int output_initialized;
  int feat_is_null;
  int nesting_multipoint;
  double empty_coord_values[4];
  struct GeoArrowCoordView empty_coord;
  enum GeoArrowDimensions last_dimensions;
  int64_t size[32];
  int32_t level;
};

static GeoArrowErrorCode GeoArrowNativeWriterEnsureOutputInitialized(
    struct GeoArrowNativeWriter* writer);

GeoArrowErrorCode GeoArrowNativeWriterInit(struct GeoArrowNativeWriter* writer,
                                           enum GeoArrowType type) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowNativeWriterPrivate));
  if (private_data == NULL) {
    return ENOMEM;
  }

  memset(private_data, 0, sizeof(struct GeoArrowNativeWriterPrivate));

  GeoArrowErrorCode result = GeoArrowBuilderInitFromType(&private_data->builder, type);
  if (result != GEOARROW_OK) {
    ArrowFree(private_data);
    return result;
  }

  ArrowBitmapInit(&private_data->validity);

  // Initialize one empty coordinate
  memcpy(private_data->empty_coord_values, kEmptyPointCoords, 4 * sizeof(double));
  private_data->empty_coord.values[0] = private_data->empty_coord_values;
  private_data->empty_coord.values[1] = private_data->empty_coord_values + 1;
  private_data->empty_coord.values[2] = private_data->empty_coord_values + 2;
  private_data->empty_coord.values[3] = private_data->empty_coord_values + 3;
  private_data->empty_coord.n_coords = 1;
  private_data->empty_coord.n_values = 4;
  private_data->empty_coord.coords_stride = 1;

  writer->private_data = private_data;

  result = GeoArrowNativeWriterEnsureOutputInitialized(writer);
  if (result != GEOARROW_OK) {
    GeoArrowNativeWriterReset(writer);
    return result;
  }

  return GEOARROW_OK;
}

void GeoArrowNativeWriterReset(struct GeoArrowNativeWriter* writer) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  GeoArrowBuilderReset(&private_data->builder);
  ArrowBitmapReset(&private_data->validity);
  ArrowFree(private_data);
}

static GeoArrowErrorCode GeoArrowNativeWriterEnsureOutputInitialized(
    struct GeoArrowNativeWriter* writer) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  if (private_data->output_initialized) {
    return GEOARROW_OK;
  }

  struct GeoArrowBuilder* builder = &private_data->builder;

  int32_t zero = 0;
  for (int i = 0; i < private_data->builder.view.n_offsets; i++) {
    NANOARROW_RETURN_NOT_OK(GeoArrowBuilderOffsetAppend(builder, i, &zero, 1));
  }

  private_data->null_count = 0;
  NANOARROW_RETURN_NOT_OK(ArrowBitmapResize(&private_data->validity, 0, 0));

  private_data->builder.view.coords.size_coords = 0;
  private_data->builder.view.coords.capacity_coords = 0;

  private_data->output_initialized = 1;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowNativeWriterFinish(struct GeoArrowNativeWriter* writer,
                                             struct ArrowArray* array,
                                             struct GeoArrowError* error) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  // We could in theory wrap this buffer instead of copy it
  struct GeoArrowBufferView validity_view;
  validity_view.data = private_data->validity.buffer.data;
  validity_view.size_bytes = private_data->validity.buffer.size_bytes;
  if (validity_view.size_bytes > 0) {
    GEOARROW_RETURN_NOT_OK(
        GeoArrowBuilderAppendBuffer(&private_data->builder, 0, validity_view));
  }

  struct ArrowArray tmp;
  GEOARROW_RETURN_NOT_OK(GeoArrowBuilderFinish(&private_data->builder, &tmp, error));
  tmp.null_count = private_data->null_count;

  private_data->output_initialized = 0;

  GeoArrowErrorCode result = GeoArrowNativeWriterEnsureOutputInitialized(writer);
  if (result != GEOARROW_OK) {
    ArrowArrayRelease(&tmp);
    GeoArrowErrorSet(error, "Failed to reinitialize writer");
    return result;
  }

  ArrowArrayMove(&tmp, array);
  return GEOARROW_OK;
}

// Count the storage needed without validating the node tree. Check only fields
// used to address coordinate memory; structural validation is a separate operation.
static GeoArrowErrorCode GeoArrowNativeWriterCountCoords(struct GeoArrowGeometryView geom,
                                                         int64_t* coord_count,
                                                         int64_t* empty_points,
                                                         struct GeoArrowError* error) {
  if (geom.root == NULL || geom.size_nodes <= 0) {
    GeoArrowErrorSet(error, "Expected a non-empty geometry node view");
    return EINVAL;
  }

  *coord_count = 0;
  *empty_points = 0;
  for (int64_t i = 0; i < geom.size_nodes; i++) {
    const struct GeoArrowGeometryNode* node = geom.root + i;
    if (node->geometry_type != GEOARROW_GEOMETRY_TYPE_POINT &&
        node->geometry_type != GEOARROW_GEOMETRY_TYPE_LINESTRING) {
      continue;
    }
    if (node->dimensions < GEOARROW_DIMENSIONS_XY ||
        node->dimensions > GEOARROW_DIMENSIONS_XYZM) {
      GeoArrowErrorSet(error, "Invalid coordinate dimensions");
      return EINVAL;
    }
    if (node->size > 0) {
      for (int j = 0; j < _GeoArrowkNumDimensions[node->dimensions]; j++) {
        if (node->coords[j] == NULL) {
          GeoArrowErrorSet(error, "Missing coordinate data");
          return EINVAL;
        }
      }
    }
    *coord_count += node->size;
    *empty_points +=
        node->geometry_type == GEOARROW_GEOMETRY_TYPE_POINT && node->size == 0;
    if (*coord_count > INT32_MAX || *empty_points > INT32_MAX) {
      GeoArrowErrorSet(error, "Geometry coordinate count exceeds INT32_MAX");
      return EOVERFLOW;
    }
  }
  return GEOARROW_OK;
}

struct GeoArrowNativeWriterAppendPlan {
  struct GeoArrowGeometryView geom;
  struct GeoArrowGeometryNode empty;
  int wrap;
  int64_t coord_count;
  int64_t empty_points;
  int32_t last_offset[3];
  int64_t n_offsets[3];
};

static void GeoArrowNativeWriterPlanEmpty(struct GeoArrowNativeWriterAppendPlan* plan,
                                          enum GeoArrowGeometryType type,
                                          enum GeoArrowDimensions dimensions) {
  memset(&plan->empty, 0, sizeof(plan->empty));
  plan->empty.geometry_type = (uint8_t)type;
  plan->empty.dimensions = (uint8_t)dimensions;
  plan->geom.root = &plan->empty;
  plan->geom.size_nodes = 1;
  plan->wrap = 0;
  plan->coord_count = 0;
  plan->empty_points = type == GEOARROW_GEOMETRY_TYPE_POINT;
}

// Support matching types and single/multi conversions. Except for POINT output,
// an otherwise incompatible geometry with no coordinates still becomes EMPTY.
static GeoArrowErrorCode GeoArrowNativeWriterPlanConversion(
    struct GeoArrowNativeWriterAppendPlan* plan, enum GeoArrowGeometryType type,
    enum GeoArrowDimensions dimensions, struct GeoArrowError* error) {
  const struct GeoArrowGeometryNode* node = plan->geom.root;
  int compatible = node->geometry_type == type;
  int unwrap = 0;
  switch (type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      // POINT storage has room for exactly one coordinate.
      if (plan->coord_count > 1) {
        GeoArrowErrorSet(error, "Can't append multiple coordinates to POINT output");
        return EINVAL;
      }
      if (compatible) {
        return GEOARROW_OK;
      }
      if (node->geometry_type == GEOARROW_GEOMETRY_TYPE_MULTIPOINT && node->size == 1 &&
          plan->geom.size_nodes > 1) {
        plan->geom.root++;
        plan->geom.size_nodes--;
        return GEOARROW_OK;
      }
      GeoArrowErrorSet(
          error, "Expected POINT or MULTIPOINT with exactly one point for POINT output");
      return EINVAL;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      unwrap = node->geometry_type == GEOARROW_GEOMETRY_TYPE_MULTILINESTRING;
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      unwrap = node->geometry_type == GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      plan->wrap = node->geometry_type == GEOARROW_GEOMETRY_TYPE_POINT;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      plan->wrap = node->geometry_type == GEOARROW_GEOMETRY_TYPE_LINESTRING;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      plan->wrap = node->geometry_type == GEOARROW_GEOMETRY_TYPE_POLYGON;
      break;
    default:
      return ENOTSUP;
  }

  if (compatible || plan->wrap) {
    return GEOARROW_OK;
  }
  if (unwrap && node->size == 1) {
    if (plan->geom.size_nodes <= 1) {
      GeoArrowErrorSet(error, "Missing child geometry to unwrap");
      return EINVAL;
    }
    plan->geom.root++;
    plan->geom.size_nodes--;
    return GEOARROW_OK;
  }
  if (plan->coord_count == 0) {
    GeoArrowNativeWriterPlanEmpty(plan, type, dimensions);
    return GEOARROW_OK;
  }
  if (unwrap) {
    GeoArrowErrorSet(error, "Can't append %s with size >1 to array of type %s",
                     GeoArrowGeometryTypeString(node->geometry_type),
                     GeoArrowGeometryTypeString(type));
  } else {
    GeoArrowErrorSet(error, "Can't append %s to array of type %s",
                     GeoArrowGeometryTypeString(node->geometry_type),
                     GeoArrowGeometryTypeString(type));
  }
  return EINVAL;
}

static void GeoArrowNativeWriterAppendCoordsUnsafe(struct GeoArrowNativeWriter* writer,
                                                   struct GeoArrowGeometryView geom) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  struct GeoArrowWritableCoordView* out_coords = &private_data->builder.view.coords;
  uint8_t* out[4];
  int32_t strides[4];
  for (int i = 0; i < out_coords->n_values; i++) {
    out[i] = (uint8_t*)(out_coords->values[i] +
                        out_coords->coords_stride * out_coords->size_coords);
    strides[i] = out_coords->coords_stride * sizeof(double);
  }
  GeoArrowGeometryViewCopyCoordsGeneric(
      geom, out, strides, private_data->builder.view.schema_view.dimensions);
}

static GeoArrowErrorCode GeoArrowNativeWriterAppendInternal(
    struct GeoArrowNativeWriter* writer, struct GeoArrowGeometryView geom, int is_null,
    struct GeoArrowError* error) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  struct GeoArrowBuilder* builder = &private_data->builder;
  enum GeoArrowGeometryType type = builder->view.schema_view.geometry_type;
  enum GeoArrowDimensions dimensions = builder->view.schema_view.dimensions;
  if (type < GEOARROW_GEOMETRY_TYPE_POINT || type > GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON) {
    GeoArrowErrorSet(error, "Geometry append requires a native geometry array");
    return ENOTSUP;
  }

  struct GeoArrowNativeWriterAppendPlan plan;
  plan.wrap = 0;
  plan.geom = geom;
  if (is_null) {
    GeoArrowNativeWriterPlanEmpty(&plan, type, dimensions);
  } else {
    GEOARROW_RETURN_NOT_OK(GeoArrowNativeWriterCountCoords(geom, &plan.coord_count,
                                                           &plan.empty_points, error));
    GEOARROW_RETURN_NOT_OK(
        GeoArrowNativeWriterPlanConversion(&plan, type, dimensions, error));
  }

  int64_t n_coords = plan.coord_count;
  if (type == GEOARROW_GEOMETRY_TYPE_POINT) {
    n_coords = 1;
  } else if (type == GEOARROW_GEOMETRY_TYPE_MULTIPOINT) {
    n_coords += plan.empty_points;
  }
  int n_offsets = builder->view.n_offsets;
  int64_t offset_add[3] = {0, 0, 0};
  memset(plan.n_offsets, 0, sizeof(plan.n_offsets));
  // Linestrings and multipoints have exactly one offset per feature. In
  // particular, avoid traversing all the multipoint children again for offsets.
  if (n_offsets == 1) {
    offset_add[0] = n_coords;
    plan.n_offsets[0] = 1;
  } else if (plan.wrap) {
    offset_add[0] = 1;
    plan.n_offsets[0] = 1;
  }
  for (int64_t i = 0; i < plan.geom.size_nodes && n_offsets > 1; i++) {
    const struct GeoArrowGeometryNode* node = plan.geom.root + i;
    int level = node->level - plan.geom.root->level + plan.wrap;
    if (level < 0) {
      GeoArrowErrorSet(error, "Geometry level would index before offset buffers");
      return EINVAL;
    }
    if (level < n_offsets) {
      offset_add[level] += node->size;
      if (offset_add[level] > INT32_MAX) {
        GeoArrowErrorSet(error, "Geometry offsets exceed INT32_MAX");
        return EOVERFLOW;
      }
      plan.n_offsets[level]++;
    }
  }
  for (int i = 0; i < n_offsets; i++) {
    const struct GeoArrowWritableBufferView* buffer = builder->view.buffers + i + 1;
    plan.last_offset[i] = buffer->data.as_int32[buffer->size_bytes / sizeof(int32_t) - 1];
    if (offset_add[i] > INT32_MAX - (int64_t)plan.last_offset[i]) {
      GeoArrowErrorSet(error, "Appending geometry would overflow native array offsets");
      return EOVERFLOW;
    }
  }

  // Allocate before committing offsets, coordinates, or validity. Read the length
  // from the builder so direct appends and visitor-produced features can alternate.
  int64_t length =
      n_offsets == 0 ? builder->view.coords.size_coords
                     : builder->view.buffers[1].size_bytes / (int64_t)sizeof(int32_t) - 1;
  for (int i = 0; i < n_offsets; i++) {
    if (!GeoArrowBuilderOffsetCheck(builder, i, plan.n_offsets[i])) {
      GEOARROW_RETURN_NOT_OK(GeoArrowBuilderReserveBuffer(
          builder, i + 1, plan.n_offsets[i] * sizeof(int32_t)));
    }
  }
  if (is_null || private_data->validity.buffer.data != NULL) {
    GEOARROW_RETURN_NOT_OK(ArrowBitmapReserve(
        &private_data->validity, length + 1 - private_data->validity.size_bits));
    // Backfill existing valid features now so visitor appends also remain usable
    // if a subsequent coordinate allocation fails.
    ArrowBitmapAppendUnsafe(&private_data->validity, 1,
                            length - private_data->validity.size_bits);
  }
  if (n_coords > 0) {
    GEOARROW_RETURN_NOT_OK(GeoArrowBuilderCoordsReserve(builder, n_coords));
  }

  if (n_offsets == 1) {
    plan.last_offset[0] += (int32_t)n_coords;
    GeoArrowBuilderOffsetAppendUnsafe(builder, 0, plan.last_offset, 1);
  } else if (plan.wrap) {
    plan.last_offset[0]++;
    GeoArrowBuilderOffsetAppendUnsafe(builder, 0, plan.last_offset, 1);
  }
  for (int64_t i = 0; i < plan.geom.size_nodes && n_offsets > 1; i++) {
    const struct GeoArrowGeometryNode* node = plan.geom.root + i;
    int level = node->level - plan.geom.root->level + plan.wrap;
    if (level < n_offsets) {
      plan.last_offset[level] += (int32_t)node->size;
      GeoArrowBuilderOffsetAppendUnsafe(builder, level, plan.last_offset + level, 1);
    }
  }

  if (type == GEOARROW_GEOMETRY_TYPE_POINT && plan.coord_count == 0) {
    GeoArrowBuilderCoordsAppendUnsafe(builder, &private_data->empty_coord, dimensions, 0,
                                      1);
  } else if (type == GEOARROW_GEOMETRY_TYPE_MULTIPOINT && plan.empty_points > 0) {
    for (int64_t i = 0; i < plan.geom.size_nodes; i++) {
      const struct GeoArrowGeometryNode* node = plan.geom.root + i;
      if (node->geometry_type != GEOARROW_GEOMETRY_TYPE_POINT) {
        continue;
      }
      if (node->size == 0) {
        GeoArrowBuilderCoordsAppendUnsafe(builder, &private_data->empty_coord, dimensions,
                                          0, 1);
      } else {
        struct GeoArrowGeometryView point = {node, 1};
        GeoArrowNativeWriterAppendCoordsUnsafe(writer, point);
        builder->view.coords.size_coords++;
      }
    }
  } else if (n_coords > 0) {
    GeoArrowNativeWriterAppendCoordsUnsafe(writer, plan.geom);
    builder->view.coords.size_coords += n_coords;
  }

  if (private_data->validity.buffer.data != NULL) {
    if (private_data->validity.size_bits == 0) {
      ArrowBitmapAppendUnsafe(&private_data->validity, 1, length);
    }
    ArrowBitmapAppendUnsafe(&private_data->validity, !is_null, 1);
  }
  private_data->null_count += is_null;
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowNativeWriterAppendNull(struct GeoArrowNativeWriter* writer) {
  struct GeoArrowGeometryView unused = {NULL, 0};
  return GeoArrowNativeWriterAppendInternal(writer, unused, 1, NULL);
}

GeoArrowErrorCode GeoArrowNativeWriterAppend(struct GeoArrowNativeWriter* writer,
                                             struct GeoArrowGeometryView geom,
                                             struct GeoArrowError* error) {
  return GeoArrowNativeWriterAppendInternal(writer, geom, 0, error);
}

static int feat_start_point(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->level = 0;
  private_data->size[0] = 0;
  private_data->feat_is_null = 0;
  return GEOARROW_OK;
}

static int geom_start_point(struct GeoArrowVisitor* v,
                            enum GeoArrowGeometryType geometry_type,
                            enum GeoArrowDimensions dimensions) {
  NANOARROW_UNUSED(geometry_type);

  // level++, geometry type, dimensions, reset size
  // validate dimensions, maybe against some options that indicate
  // error for mismatch, fill, or drop behaviour
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->last_dimensions = dimensions;
  return GEOARROW_OK;
}

static int ring_start_point(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int coords_point(struct GeoArrowVisitor* v,
                        const struct GeoArrowCoordView* coords) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->size[0] += coords->n_coords;
  return GeoArrowBuilderCoordsAppend(&private_data->builder, coords,
                                     private_data->last_dimensions, 0, coords->n_coords);
}

static int ring_end_point(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int geom_end_point(struct GeoArrowVisitor* v) {
  NANOARROW_UNUSED(v);
  return GEOARROW_OK;
}

static int null_feat_point(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->feat_is_null = 1;
  return GEOARROW_OK;
}

static int feat_end_point(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  // If there weren't any coords (i.e., EMPTY), we need to write some NANs here
  // if there was >1 coords, we also need to error or we'll get misaligned output
  if (private_data->size[0] == 0) {
    int n_dim =
        _GeoArrowkNumDimensions[private_data->builder.view.schema_view.dimensions];
    private_data->empty_coord.n_values = n_dim;
    NANOARROW_RETURN_NOT_OK(coords_point(v, &private_data->empty_coord));
  } else if (private_data->size[0] != 1) {
    GeoArrowErrorSet(v->error, "Can't convert feature with >1 coordinate to POINT");
    return EINVAL;
  }

  if (private_data->feat_is_null) {
    int64_t current_length = private_data->builder.view.coords.size_coords;
    if (private_data->validity.buffer.data == NULL) {
      NANOARROW_RETURN_NOT_OK(
          ArrowBitmapReserve(&private_data->validity, current_length));
      ArrowBitmapAppendUnsafe(&private_data->validity, 1, current_length - 1);
    }

    private_data->null_count++;
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 0, 1));
  } else if (private_data->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 1, 1));
  }

  return GEOARROW_OK;
}

static void GeoArrowVisitorInitPoint(struct GeoArrowVisitor* v) {
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
}

static int feat_start_multipoint(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->level = 0;
  private_data->size[0] = 0;
  private_data->size[1] = 0;
  private_data->feat_is_null = 0;
  private_data->nesting_multipoint = 0;
  return GEOARROW_OK;
}

static int geom_start_multipoint(struct GeoArrowVisitor* v,
                                 enum GeoArrowGeometryType geometry_type,
                                 enum GeoArrowDimensions dimensions) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->last_dimensions = dimensions;

  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      private_data->level++;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      private_data->nesting_multipoint = 1;
      private_data->level++;
      break;
    case GEOARROW_GEOMETRY_TYPE_POINT:
      if (private_data->nesting_multipoint) {
        private_data->nesting_multipoint++;
      }
    default:
      break;
  }

  return GEOARROW_OK;
}

static int ring_start_multipoint(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->level++;
  return GEOARROW_OK;
}

static int coords_multipoint(struct GeoArrowVisitor* v,
                             const struct GeoArrowCoordView* coords) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->size[1] += coords->n_coords;
  return GeoArrowBuilderCoordsAppend(&private_data->builder, coords,
                                     private_data->last_dimensions, 0, coords->n_coords);
}

static int ring_end_multipoint(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  private_data->level--;
  private_data->size[0]++;
  if (private_data->builder.view.coords.size_coords > 2147483647) {
    return EOVERFLOW;
  }
  int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
  NANOARROW_RETURN_NOT_OK(
      GeoArrowBuilderOffsetAppend(&private_data->builder, 0, &n_coord32, 1));

  return GEOARROW_OK;
}

static int geom_end_multipoint(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  // Ignore geom_end calls from the end of a POINT nested within a MULTIPOINT
  if (private_data->nesting_multipoint == 2) {
    private_data->nesting_multipoint--;
    return GEOARROW_OK;
  }

  if (private_data->level == 1) {
    private_data->size[0]++;
    private_data->level--;
    if (private_data->builder.view.coords.size_coords > 2147483647) {
      return EOVERFLOW;
    }
    int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(&private_data->builder, 0, &n_coord32, 1));
  }

  return GEOARROW_OK;
}

static int null_feat_multipoint(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->feat_is_null = 1;
  return GEOARROW_OK;
}

static int feat_end_multipoint(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  // If we didn't finish any sequences, finish at least one. This is usually an
  // EMPTY but could also be a single point.
  if (private_data->size[0] == 0) {
    if (private_data->builder.view.coords.size_coords > 2147483647) {
      return EOVERFLOW;
    }
    int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(&private_data->builder, 0, &n_coord32, 1));
  } else if (private_data->size[0] != 1) {
    GeoArrowErrorSet(v->error, "Can't convert feature with >1 sequence to LINESTRING");
    return EINVAL;
  }

  if (private_data->feat_is_null) {
    int64_t current_length =
        private_data->builder.view.buffers[1].size_bytes / sizeof(int32_t) - 1;
    if (private_data->validity.buffer.data == NULL) {
      NANOARROW_RETURN_NOT_OK(
          ArrowBitmapReserve(&private_data->validity, current_length));
      ArrowBitmapAppendUnsafe(&private_data->validity, 1, current_length - 1);
    }

    private_data->null_count++;
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 0, 1));
  } else if (private_data->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 1, 1));
  }

  return GEOARROW_OK;
}

static void GeoArrowVisitorInitLinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowError* previous_error = v->error;
  GeoArrowVisitorInitVoid(v);
  v->error = previous_error;

  v->feat_start = &feat_start_multipoint;
  v->null_feat = &null_feat_multipoint;
  v->geom_start = &geom_start_multipoint;
  v->ring_start = &ring_start_multipoint;
  v->coords = &coords_multipoint;
  v->ring_end = &ring_end_multipoint;
  v->geom_end = &geom_end_multipoint;
  v->feat_end = &feat_end_multipoint;
}

static int feat_start_multilinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->level = 0;
  private_data->size[0] = 0;
  private_data->size[1] = 0;
  private_data->feat_is_null = 0;
  return GEOARROW_OK;
}

static int geom_start_multilinestring(struct GeoArrowVisitor* v,
                                      enum GeoArrowGeometryType geometry_type,
                                      enum GeoArrowDimensions dimensions) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->last_dimensions = dimensions;

  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      private_data->level++;
      break;
    default:
      break;
  }

  return GEOARROW_OK;
}

static int ring_start_multilinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->level++;
  return GEOARROW_OK;
}

static int coords_multilinestring(struct GeoArrowVisitor* v,
                                  const struct GeoArrowCoordView* coords) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->size[1] += coords->n_coords;
  return GeoArrowBuilderCoordsAppend(&private_data->builder, coords,
                                     private_data->last_dimensions, 0, coords->n_coords);
}

static int ring_end_multilinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  private_data->level--;
  if (private_data->size[1] > 0) {
    if (private_data->builder.view.coords.size_coords > 2147483647) {
      return EOVERFLOW;
    }
    int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(&private_data->builder, 1, &n_coord32, 1));
    private_data->size[0]++;
    private_data->size[1] = 0;
  }

  return GEOARROW_OK;
}

static int geom_end_multilinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  if (private_data->level == 1) {
    private_data->level--;
    if (private_data->size[1] > 0) {
      if (private_data->builder.view.coords.size_coords > 2147483647) {
        return EOVERFLOW;
      }
      int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
      NANOARROW_RETURN_NOT_OK(
          GeoArrowBuilderOffsetAppend(&private_data->builder, 1, &n_coord32, 1));
      private_data->size[0]++;
      private_data->size[1] = 0;
    }
  }

  return GEOARROW_OK;
}

static int null_feat_multilinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->feat_is_null = 1;
  return GEOARROW_OK;
}

static int feat_end_multilinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  // If we have an unfinished sequence left over, finish it now. This could have
  // occurred if the last geometry that was visited was a POINT.
  if (private_data->size[1] > 0) {
    if (private_data->builder.view.coords.size_coords > 2147483647) {
      return EOVERFLOW;
    }
    int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(&private_data->builder, 1, &n_coord32, 1));
  }

  // Finish off the sequence of sequences. This is a polygon or multilinestring
  // so it can any number of them.
  int32_t n_seq32 =
      (int32_t)(private_data->builder.view.buffers[2].size_bytes / sizeof(int32_t)) - 1;
  NANOARROW_RETURN_NOT_OK(
      GeoArrowBuilderOffsetAppend(&private_data->builder, 0, &n_seq32, 1));

  if (private_data->feat_is_null) {
    int64_t current_length =
        private_data->builder.view.buffers[1].size_bytes / sizeof(int32_t) - 1;
    if (private_data->validity.buffer.data == NULL) {
      NANOARROW_RETURN_NOT_OK(
          ArrowBitmapReserve(&private_data->validity, current_length));
      ArrowBitmapAppendUnsafe(&private_data->validity, 1, current_length - 1);
    }

    private_data->null_count++;
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 0, 1));
  } else if (private_data->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 1, 1));
  }

  return GEOARROW_OK;
}

static void GeoArrowVisitorInitMultiLinestring(struct GeoArrowVisitor* v) {
  struct GeoArrowError* previous_error = v->error;
  GeoArrowVisitorInitVoid(v);
  v->error = previous_error;

  v->feat_start = &feat_start_multilinestring;
  v->null_feat = &null_feat_multilinestring;
  v->geom_start = &geom_start_multilinestring;
  v->ring_start = &ring_start_multilinestring;
  v->coords = &coords_multilinestring;
  v->ring_end = &ring_end_multilinestring;
  v->geom_end = &geom_end_multilinestring;
  v->feat_end = &feat_end_multilinestring;
}

static int feat_start_multipolygon(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->level = 0;
  private_data->size[0] = 0;
  private_data->size[1] = 0;
  private_data->size[2] = 0;
  private_data->feat_is_null = 0;
  return GEOARROW_OK;
}

static int geom_start_multipolygon(struct GeoArrowVisitor* v,
                                   enum GeoArrowGeometryType geometry_type,
                                   enum GeoArrowDimensions dimensions) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->last_dimensions = dimensions;

  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      private_data->level++;
      break;
    default:
      break;
  }

  return GEOARROW_OK;
}

static int ring_start_multipolygon(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->level++;
  return GEOARROW_OK;
}

static int coords_multipolygon(struct GeoArrowVisitor* v,
                               const struct GeoArrowCoordView* coords) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->size[2] += coords->n_coords;
  return GeoArrowBuilderCoordsAppend(&private_data->builder, coords,
                                     private_data->last_dimensions, 0, coords->n_coords);
}

static int ring_end_multipolygon(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  private_data->level--;
  if (private_data->size[2] > 0) {
    if (private_data->builder.view.coords.size_coords > 2147483647) {
      return EOVERFLOW;
    }
    int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(&private_data->builder, 2, &n_coord32, 1));
    private_data->size[1]++;
    private_data->size[2] = 0;
  }

  return GEOARROW_OK;
}

static int geom_end_multipolygon(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  if (private_data->level == 2) {
    private_data->level--;
    if (private_data->size[2] > 0) {
      if (private_data->builder.view.coords.size_coords > 2147483647) {
        return EOVERFLOW;
      }
      int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
      NANOARROW_RETURN_NOT_OK(
          GeoArrowBuilderOffsetAppend(&private_data->builder, 2, &n_coord32, 1));
      private_data->size[1]++;
      private_data->size[2] = 0;
    }
  } else if (private_data->level == 1) {
    private_data->level--;
    if (private_data->size[1] > 0) {
      int32_t n_seq32 =
          (int32_t)(private_data->builder.view.buffers[3].size_bytes / sizeof(int32_t)) -
          1;
      NANOARROW_RETURN_NOT_OK(
          GeoArrowBuilderOffsetAppend(&private_data->builder, 1, &n_seq32, 1));
      private_data->size[0]++;
      private_data->size[1] = 0;
    }
  }

  return GEOARROW_OK;
}

static int null_feat_multipolygon(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;
  private_data->feat_is_null = 1;
  return GEOARROW_OK;
}

static int feat_end_multipolygon(struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriter* writer = (struct GeoArrowNativeWriter*)v->private_data;
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  // If we have an unfinished sequence left over, finish it now. This could have
  // occurred if the last geometry that was visited was a POINT.
  if (private_data->size[2] > 0) {
    if (private_data->builder.view.coords.size_coords > 2147483647) {
      return EOVERFLOW;
    }
    int32_t n_coord32 = (int32_t)private_data->builder.view.coords.size_coords;
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(&private_data->builder, 2, &n_coord32, 1));
    private_data->size[1]++;
  }

  // If we have an unfinished sequence of sequences left over, finish it now.
  // This could have occurred if the last geometry that was visited was a POINT.
  if (private_data->size[1] > 0) {
    int32_t n_seq32 =
        (int32_t)(private_data->builder.view.buffers[3].size_bytes / sizeof(int32_t)) - 1;
    NANOARROW_RETURN_NOT_OK(
        GeoArrowBuilderOffsetAppend(&private_data->builder, 1, &n_seq32, 1));
  }

  // Finish off the sequence of sequence of sequences. This is a multipolygon
  // so it can be any number of them.
  int32_t n_seq_seq32 =
      (int32_t)(private_data->builder.view.buffers[2].size_bytes / sizeof(int32_t)) - 1;
  NANOARROW_RETURN_NOT_OK(
      GeoArrowBuilderOffsetAppend(&private_data->builder, 0, &n_seq_seq32, 1));

  if (private_data->feat_is_null) {
    int64_t current_length =
        private_data->builder.view.buffers[1].size_bytes / sizeof(int32_t) - 1;
    if (private_data->validity.buffer.data == NULL) {
      NANOARROW_RETURN_NOT_OK(
          ArrowBitmapReserve(&private_data->validity, current_length));
      ArrowBitmapAppendUnsafe(&private_data->validity, 1, current_length - 1);
    }

    private_data->null_count++;
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 0, 1));
  } else if (private_data->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 1, 1));
  }

  return GEOARROW_OK;
}

static void GeoArrowVisitorInitMultiPolygon(struct GeoArrowVisitor* v) {
  struct GeoArrowError* previous_error = v->error;
  GeoArrowVisitorInitVoid(v);
  v->error = previous_error;

  v->feat_start = &feat_start_multipolygon;
  v->null_feat = &null_feat_multipolygon;
  v->geom_start = &geom_start_multipolygon;
  v->ring_start = &ring_start_multipolygon;
  v->coords = &coords_multipolygon;
  v->ring_end = &ring_end_multipolygon;
  v->geom_end = &geom_end_multipolygon;
  v->feat_end = &feat_end_multipolygon;
}

GeoArrowErrorCode GeoArrowNativeWriterInitVisitor(struct GeoArrowNativeWriter* writer,
                                                  struct GeoArrowVisitor* v) {
  struct GeoArrowNativeWriterPrivate* private_data =
      (struct GeoArrowNativeWriterPrivate*)writer->private_data;

  switch (private_data->builder.view.schema_view.geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      GeoArrowVisitorInitPoint(v);
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      GeoArrowVisitorInitLinestring(v);
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      GeoArrowVisitorInitMultiLinestring(v);
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      GeoArrowVisitorInitMultiPolygon(v);
      break;
    default:
      return EINVAL;
  }

  v->private_data = writer;
  return GEOARROW_OK;
}
