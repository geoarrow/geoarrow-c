
#include <stdio.h>
#include <string.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

struct WKTWriterPrivate {
  enum ArrowType storage_type;
  struct ArrowBitmap validity;
  struct ArrowBuffer offsets;
  struct ArrowBuffer values;
  enum GeoArrowGeometryType geometry_type[32];
  int64_t i[32];
  int32_t level;
  int64_t length;
  int64_t null_count;
  int64_t values_feat_start;
  int precision;
  int use_flat_multipoint;
  int64_t max_element_size_bytes;
  int feat_is_null;
};

static inline int WKTWriterCheckLevel(struct WKTWriterPrivate* private) {
  if (private->level >= 0 && private->level <= 31) {
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static inline int WKTWriterWrite(struct WKTWriterPrivate* private, const char* value) {
  return ArrowBufferAppend(&private->values, value, strlen(value));
}

static GeoArrowErrorCode WKTWriterWriteGeometryType(
    struct WKTWriterPrivate* private_data, enum GeoArrowGeometryType geometry_type,
    enum GeoArrowDimensions dimensions) {
  const char* geometry_type_name = GeoArrowGeometryTypeString(geometry_type);
  NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, geometry_type_name));

  switch (dimensions) {
    case GEOARROW_DIMENSIONS_XY:
      break;
    case GEOARROW_DIMENSIONS_XYZ:
      NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, " Z"));
      break;
    case GEOARROW_DIMENSIONS_XYM:
      NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, " M"));
      break;
    case GEOARROW_DIMENSIONS_XYZM:
      NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, " ZM"));
      break;
    default:
      NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, " <not valid>"));
      return EINVAL;
  }

  GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, " "));

  return GEOARROW_OK;
}

static GeoArrowErrorCode WKTWriterReserveCoords(struct WKTWriterPrivate* private_data,
                                                int64_t n_coords, int n_values) {
  int64_t max_chars_per_coord_theoretical =
      // space + comma after coordinate
      (n_coords * 2) +
      // spaces between ordinates
      (n_coords * (n_values - 1)) +
      // GeoArrowPrintDouble might require up to 40 accessible bytes per call
      (40 * n_values);

  // Use a heuristic to estimate the number of characters we are about to write
  // to avoid more then one allocation for this call. This is normally substantially
  // less than the theoretical amount.
  int64_t max_chars_estimated =
      (n_coords * 2) +               // space + comma after coordinate
      (n_coords * (n_values - 1)) +  // spaces between ordinates
      // precision + decimal + estimate of normal
      // digits to the left of the decimal
      ((private_data->precision + 1 + 8) * n_coords * n_values) +
      // Ensure that the last reserve() call doesn't trigger an allocation
      max_chars_per_coord_theoretical;

  GEOARROW_RETURN_NOT_OK(ArrowBufferReserve(&private_data->values, max_chars_estimated));
  return GEOARROW_OK;
}

static GeoArrowErrorCode WKTWriterCheckElementOutputSize(
    struct WKTWriterPrivate* private_data) {
  // Check if we've hit our max number of bytes for this feature
  if (private_data->max_element_size_bytes >= 0 &&
      (private_data->values.size_bytes - private_data->values_feat_start) >=
          private_data->max_element_size_bytes) {
    return EAGAIN;
  } else {
    return GEOARROW_OK;
  }
}

static inline void WKTWriterWriteDoubleUnsafe(struct WKTWriterPrivate* private,
                                              double value) {
  // Always ensure that we have at least 40 writable bytes remaining before calling
  // GeoArrowPrintDouble()
  NANOARROW_DCHECK((private->values.capacity_bytes - private->values.size_bytes) >= 40);
  private->values.size_bytes +=
      GeoArrowPrintDouble(value, private->precision,
                          ((char*)private->values.data) + private->values.size_bytes);
}

static int feat_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  private->level = -1;
  private->length++;
  private->feat_is_null = 0;
  private->values_feat_start = private->values.size_bytes;

  if (private->values.size_bytes > 2147483647) {
    return EOVERFLOW;
  }
  return ArrowBufferAppendInt32(&private->offsets, (int32_t)private->values.size_bytes);
}

static int null_feat_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  private->feat_is_null = 1;
  return GEOARROW_OK;
}

static int geom_start_wkt(struct GeoArrowVisitor* v,
                          enum GeoArrowGeometryType geometry_type,
                          enum GeoArrowDimensions dimensions) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  private->level++;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  if (private->level > 0 && private->i[private->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, ", "));
  } else if (private->level > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, "("));
  }

  if (private->level == 0 || private->geometry_type[private->level - 1] ==
                                 GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION) {
    if (geometry_type < GEOARROW_GEOMETRY_TYPE_POINT ||
        geometry_type > GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION) {
      GeoArrowErrorSet(v->error, "WKTWriter::geom_start(): Unexpected `geometry_type`");
      return EINVAL;
    }

    GEOARROW_RETURN_NOT_OK(
        WKTWriterWriteGeometryType(private, geometry_type, dimensions));
  }

  if (private->level > 0) {
    private->i[private->level - 1]++;
  }

  private->geometry_type[private->level] = geometry_type;
  private->i[private->level] = 0;
  return GEOARROW_OK;
}

static int ring_start_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  private->level++;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  if (private->level > 0 && private->i[private->level - 1] > 0) {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, ", "));
  } else {
    NANOARROW_RETURN_NOT_OK(WKTWriterWrite(private, "("));
  }

  if (private->level > 0) {
    private->i[private->level - 1]++;
  }

  private->geometry_type[private->level] = GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  private->i[private->level] = 0;
  return GEOARROW_OK;
}

static int coords_wkt(struct GeoArrowVisitor* v, const struct GeoArrowCoordView* coords) {
  int64_t n_coords = coords->n_coords;
  int32_t n_dims = coords->n_values;
  if (n_coords == 0) {
    return GEOARROW_OK;
  }

  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));
  GEOARROW_RETURN_NOT_OK(WKTWriterReserveCoords(private, n_coords, n_dims));

  int64_t max_chars_per_coord_theoretical =
      // space + comma after coordinate
      (n_coords * 2) +
      // spaces between ordinates
      (n_coords * (n_dims - 1)) +
      // GeoArrowPrintDouble might require up to 40 accessible bytes per call
      (40 * n_dims);

  // Use a heuristic to estimate the number of characters we are about to write
  // to avoid more then one allocation for this call. This is normally substantially
  // less than the theoretical amount.
  int64_t max_chars_estimated =
      (n_coords * 2) +             // space + comma after coordinate
      (n_coords * (n_dims - 1)) +  // spaces between ordinates
      // precision + decimal + estimate of normal
      // digits to the left of the decimal
      ((private->precision + 1 + 8) * n_coords * n_dims) +
      // Ensure that the last reserve() call doesn't trigger an allocation
      max_chars_per_coord_theoretical;
  NANOARROW_RETURN_NOT_OK(ArrowBufferReserve(&private->values, max_chars_estimated));

  // Write the first coordinate, possibly with a leading comma if there was
  // a previous call to coords, or the opening (if it wasn't). Special case
  // for the flat multipoint output MULTIPOINT (1 2, 3 4, ...) which doesn't
  // have extra () for inner POINTs
  if (private->i[private->level] != 0) {
    ArrowBufferAppendUnsafe(&private->values, ", ", 2);
  } else if (private->level < 1 || !private->use_flat_multipoint ||
             private->geometry_type[private->level - 1] !=
                 GEOARROW_GEOMETRY_TYPE_MULTIPOINT) {
    ArrowBufferAppendUnsafe(&private->values, "(", 1);
  }

  // Actually write the first coordinate (no leading comma)
  // Reserve the theoretical amount for each coordinate because we need this to guarantee
  // that there won't be a segfault when writing a coordinate. This probably results in
  // a few dozen bytes of of overallocation.
  NANOARROW_RETURN_NOT_OK(
      ArrowBufferReserve(&private->values, max_chars_per_coord_theoretical));
  WKTWriterWriteDoubleUnsafe(private, GEOARROW_COORD_VIEW_VALUE(coords, 0, 0));
  for (int32_t j = 1; j < n_dims; j++) {
    ArrowBufferAppendUnsafe(&private->values, " ", 1);
    WKTWriterWriteDoubleUnsafe(private, GEOARROW_COORD_VIEW_VALUE(coords, 0, j));
  }

  // Write the remaining coordinates (which all have leading commas)
  for (int64_t i = 1; i < n_coords; i++) {
    // Check if we've hit our max number of bytes for this feature
    if (private->max_element_size_bytes >= 0 &&
        (private->values.size_bytes - private->values_feat_start) >=
            private->max_element_size_bytes) {
      return EAGAIN;
    }

    NANOARROW_RETURN_NOT_OK(
        ArrowBufferReserve(&private->values, max_chars_per_coord_theoretical));
    ArrowBufferAppendUnsafe(&private->values, ", ", 2);
    WKTWriterWriteDoubleUnsafe(private, GEOARROW_COORD_VIEW_VALUE(coords, i, 0));
    for (int32_t j = 1; j < n_dims; j++) {
      ArrowBufferAppendUnsafe(&private->values, " ", 1);
      WKTWriterWriteDoubleUnsafe(private, GEOARROW_COORD_VIEW_VALUE(coords, i, j));
    }
  }

  private->i[private->level] += n_coords;
  return GEOARROW_OK;
}

static int ring_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));
  if (private->i[private->level] == 0) {
    private->level--;
    return WKTWriterWrite(private, "EMPTY");
  } else {
    private->level--;
    return WKTWriterWrite(private, ")");
  }
}

static int geom_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;
  NANOARROW_RETURN_NOT_OK(WKTWriterCheckLevel(private));

  if (private->i[private->level] == 0) {
    private->level--;
    return WKTWriterWrite(private, "EMPTY");
  } else if (private->level < 1 || !private->use_flat_multipoint ||
             private->geometry_type[private->level - 1] !=
                 GEOARROW_GEOMETRY_TYPE_MULTIPOINT) {
    private->level--;
    return WKTWriterWrite(private, ")");
  } else {
    private->level--;
    return GEOARROW_OK;
  }
}

static int feat_end_wkt(struct GeoArrowVisitor* v) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)v->private_data;

  if (private->feat_is_null) {
    if (private->validity.buffer.data == NULL) {
      NANOARROW_RETURN_NOT_OK(ArrowBitmapReserve(&private->validity, private->length));
      ArrowBitmapAppendUnsafe(&private->validity, 1, private->length - 1);
    }

    private->null_count++;
    return ArrowBitmapAppend(&private->validity, 0, 1);
  } else if (private->validity.buffer.data != NULL) {
    return ArrowBitmapAppend(&private->validity, 1, 1);
  }

  if (private->max_element_size_bytes >= 0 &&
      (private->values.size_bytes - private->values_feat_start) >
          private->max_element_size_bytes) {
    private->values.size_bytes =
        private->values_feat_start + private->max_element_size_bytes;
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowWKTWriterAppendValidity(struct GeoArrowWKTWriter* writer,
                                                         int is_valid) {
  struct WKTWriterPrivate* private_data = (struct WKTWriterPrivate*)writer->private_data;

  private_data->length++;

  if (!is_valid) {
    if (private_data->validity.buffer.data == NULL) {
      NANOARROW_RETURN_NOT_OK(
          ArrowBitmapReserve(&private_data->validity, private_data->length));
      ArrowBitmapAppendUnsafe(&private_data->validity, 1, private_data->length - 1);
    }

    private_data->null_count++;
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 0, 1));
  } else if (private_data->validity.buffer.data != NULL) {
    NANOARROW_RETURN_NOT_OK(ArrowBitmapAppend(&private_data->validity, 1, 1));
  }

  return GEOARROW_OK;
}

static inline void GeoArrowWKTWriterAppendCoordUnsafe(
    struct WKTWriterPrivate* private_data, const uint8_t** cursor, const int32_t* strides,
    int n_values) {
  double value;

  memcpy(&value, cursor[0], sizeof(double));
  WKTWriterWriteDoubleUnsafe(private_data, value);
  cursor[0] += strides[0];

  for (int j = 1; j < n_values; j++) {
    ArrowBufferAppendUnsafe(&private_data->values, " ", 1);
    memcpy(&value, cursor[j], sizeof(double));
    WKTWriterWriteDoubleUnsafe(private_data, value);
    cursor[j] += strides[j];
  }
}

static GeoArrowErrorCode GeoArrowWKTWriterAppendCoordsUnsafe(
    struct WKTWriterPrivate* private_data, const struct GeoArrowGeometryNode* node,
    int n_values) {
  const uint8_t* cursor[4];
  memcpy(cursor, node->coords, sizeof(cursor));

  int32_t stride[4];
  memcpy(stride, node->coord_stride, sizeof(stride));

  // Append the first coord
  GeoArrowWKTWriterAppendCoordUnsafe(private_data, cursor, stride, n_values);

  // Append the rest of the coords
  for (uint32_t i = 1; i < node->size; i++) {
    GEOARROW_RETURN_NOT_OK(WKTWriterCheckElementOutputSize(private_data));
    ArrowBufferAppendUnsafe(&private_data->values, ", ", 2);
    GeoArrowWKTWriterAppendCoordUnsafe(private_data, cursor, stride, n_values);
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowWKTWriterAppendSequenceOrEmpty(
    struct WKTWriterPrivate* private_data, const struct GeoArrowGeometryNode* node) {
  if (node->size == 0) {
    GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, "EMPTY"));
    return GEOARROW_OK;
  }

  uint32_t n_coords = node->size;
  int n_values = _GeoArrowkNumDimensions[node->dimensions];

  GEOARROW_RETURN_NOT_OK(WKTWriterReserveCoords(private_data, n_coords, n_values));

  GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, "("));
  GeoArrowWKTWriterAppendCoordsUnsafe(private_data, node, n_values);
  GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, ")"));

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowWKTWriterAppendFlatMultipoint(
    struct WKTWriterPrivate* private_data, const struct GeoArrowGeometryNode* node,
    const struct GeoArrowGeometryNode** node_iter) {
  int n_values;
  const uint8_t* cursor[4];
  int32_t stride[4] = {0, 0, 0, 0};

  GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, "("));

  // Append the first child as a coord
  const struct GeoArrowGeometryNode* child_node = (*node_iter)++;
  n_values = _GeoArrowkNumDimensions[child_node->dimensions];
  GEOARROW_RETURN_NOT_OK(
      WKTWriterReserveCoords(private_data, child_node->size, n_values));
  memcpy(cursor, child_node->coords, sizeof(cursor));
  GeoArrowWKTWriterAppendCoordUnsafe(private_data, cursor, stride, n_values);

  // Append the rest of the children as coords
  for (uint32_t i = 1; i < node->size; i++) {
    GEOARROW_RETURN_NOT_OK(WKTWriterCheckElementOutputSize(private_data));

    const struct GeoArrowGeometryNode* child_node = (*node_iter)++;
    n_values = _GeoArrowkNumDimensions[child_node->dimensions];
    GEOARROW_RETURN_NOT_OK(
        WKTWriterReserveCoords(private_data, child_node->size, n_values));

    memcpy(cursor, child_node->coords, sizeof(cursor));
    GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, ", "));
    GeoArrowWKTWriterAppendCoordUnsafe(private_data, cursor, stride, n_values);
  }

  GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, ")"));
  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowWKTWriterAppendImpl(
    struct WKTWriterPrivate* private_data, const struct GeoArrowGeometryNode** node_iter,
    int tag) {
  const struct GeoArrowGeometryNode* node = (*node_iter)++;

  if (tag) {
    enum GeoArrowGeometryType geometry_type =
        (enum GeoArrowGeometryType)node->geometry_type;
    enum GeoArrowDimensions dimensions = (enum GeoArrowDimensions)node->dimensions;

    GEOARROW_RETURN_NOT_OK(
        WKTWriterWriteGeometryType(private_data, geometry_type, dimensions));
  }

  switch (node->geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      GEOARROW_RETURN_NOT_OK(GeoArrowWKTWriterAppendSequenceOrEmpty(private_data, node));
      return GEOARROW_OK;

    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      if (private_data->use_flat_multipoint) {
        return GeoArrowWKTWriterAppendFlatMultipoint(private_data, node, node_iter);
      }
#if defined(__GNUC__) || defined(__clang__)
      __attribute__((fallthrough));
#endif
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
    case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION: {
      if (node->size == 0) {
        GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, "EMPTY"));
        return GEOARROW_OK;
      }

      GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, "("));
      GEOARROW_RETURN_NOT_OK(GeoArrowWKTWriterAppendImpl(
          private_data, node_iter,
          node->geometry_type == GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION));
      for (uint32_t i = 1; i < node->size; i++) {
        GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, ", "));
        GEOARROW_RETURN_NOT_OK(GeoArrowWKTWriterAppendImpl(
            private_data, node_iter,
            node->geometry_type == GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION));
      }
      GEOARROW_RETURN_NOT_OK(WKTWriterWrite(private_data, ")"));
      return GEOARROW_OK;
    }
    default:
      return EINVAL;
  }
}

GeoArrowErrorCode GeoArrowWKTWriterAppendNull(struct GeoArrowWKTWriter* writer) {
  struct WKTWriterPrivate* private_data = (struct WKTWriterPrivate*)writer->private_data;
  GEOARROW_RETURN_NOT_OK(ArrowBufferAppendInt32(
      &private_data->offsets, (int32_t)private_data->values.size_bytes));
  GEOARROW_RETURN_NOT_OK(GeoArrowWKTWriterAppendValidity(writer, 0));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowWKTWriterAppend(struct GeoArrowWKTWriter* writer,
                                          struct GeoArrowGeometryView geom) {
  struct WKTWriterPrivate* private_data = (struct WKTWriterPrivate*)writer->private_data;
  private_data->use_flat_multipoint = writer->use_flat_multipoint;
  private_data->max_element_size_bytes = writer->max_element_size_bytes;
  private_data->precision = writer->precision;

  private_data->values_feat_start = private_data->values.size_bytes;

  GEOARROW_RETURN_NOT_OK(ArrowBufferAppendInt32(
      &private_data->offsets, (int32_t)private_data->values.size_bytes));

  const struct GeoArrowGeometryNode* node_iter = geom.root;
  GeoArrowErrorCode result = GeoArrowWKTWriterAppendImpl(private_data, &node_iter, 1);
  switch (result) {
    case GEOARROW_OK:
      break;
    case EAGAIN:
      if (private_data->max_element_size_bytes >= 0 &&
          (private_data->values.size_bytes - private_data->values_feat_start) >
              private_data->max_element_size_bytes) {
        private_data->values.size_bytes =
            private_data->values_feat_start + private_data->max_element_size_bytes;
      }
      break;
    default:
      return result;
  }

  GEOARROW_RETURN_NOT_OK(GeoArrowWKTWriterAppendValidity(writer, 1));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowWKTWriterInit(struct GeoArrowWKTWriter* writer) {
  struct WKTWriterPrivate* private =
      (struct WKTWriterPrivate*)ArrowMalloc(sizeof(struct WKTWriterPrivate));
  if (private == NULL) {
    return ENOMEM;
  }

  private->storage_type = NANOARROW_TYPE_STRING;
  private->length = 0;
  private->level = 0;
  private->null_count = 0;
  ArrowBitmapInit(&private->validity);
  ArrowBufferInit(&private->offsets);
  ArrowBufferInit(&private->values);
  writer->precision = 16;
  private->precision = 16;
  writer->use_flat_multipoint = 1;
  private->use_flat_multipoint = 1;
  writer->max_element_size_bytes = -1;
  private->max_element_size_bytes = -1;
  writer->private_data = private;

  return GEOARROW_OK;
}

void GeoArrowWKTWriterInitVisitor(struct GeoArrowWKTWriter* writer,
                                  struct GeoArrowVisitor* v) {
  GeoArrowVisitorInitVoid(v);

  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)writer->private_data;

  // Clamp writer->precision to a specific range of valid values
  if (writer->precision < 0 || writer->precision > 16) {
    private->precision = 16;
  } else {
    private->precision = writer->precision;
  }

  private->use_flat_multipoint = writer->use_flat_multipoint;
  private->max_element_size_bytes = writer->max_element_size_bytes;

  v->private_data = writer->private_data;
  v->feat_start = &feat_start_wkt;
  v->null_feat = &null_feat_wkt;
  v->geom_start = &geom_start_wkt;
  v->ring_start = &ring_start_wkt;
  v->coords = &coords_wkt;
  v->ring_end = &ring_end_wkt;
  v->geom_end = &geom_end_wkt;
  v->feat_end = &feat_end_wkt;
}

GeoArrowErrorCode GeoArrowWKTWriterFinish(struct GeoArrowWKTWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)writer->private_data;
  array->release = NULL;

  if (private->values.size_bytes > 2147483647) {
    return EOVERFLOW;
  }
  NANOARROW_RETURN_NOT_OK(
      ArrowBufferAppendInt32(&private->offsets, (int32_t)private->values.size_bytes));
  NANOARROW_RETURN_NOT_OK(ArrowArrayInitFromType(array, private->storage_type));
  ArrowArraySetValidityBitmap(array, &private->validity);
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 1, &private->offsets));
  NANOARROW_RETURN_NOT_OK(ArrowArraySetBuffer(array, 2, &private->values));
  array->length = private->length;
  array->null_count = private->null_count;
  private->length = 0;
  private->null_count = 0;
  return ArrowArrayFinishBuildingDefault(array, (struct ArrowError*)error);
}

void GeoArrowWKTWriterReset(struct GeoArrowWKTWriter* writer) {
  struct WKTWriterPrivate* private = (struct WKTWriterPrivate*)writer->private_data;
  ArrowBitmapReset(&private->validity);
  ArrowBufferReset(&private->offsets);
  ArrowBufferReset(&private->values);
  ArrowFree(private);
  writer->private_data = NULL;
}
