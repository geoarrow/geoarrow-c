
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

  NANOARROW_RETURN_NOT_OK(GeoArrowNativeWriterEnsureOutputInitialized(writer));
  v->private_data = writer;
  return GEOARROW_OK;
}
