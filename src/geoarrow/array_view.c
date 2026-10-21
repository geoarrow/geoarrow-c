
#include <errno.h>

#include "geoarrow/geoarrow.h"

#include "nanoarrow/nanoarrow.h"

static int32_t kZeroInt32 = 0;

static int GeoArrowArrayViewInitInternal(struct GeoArrowArrayView* array_view) {
  switch (array_view->schema_view.geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_BOX:
    case GEOARROW_GEOMETRY_TYPE_POINT:
      array_view->n_offsets = 0;
      break;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      array_view->n_offsets = 1;
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      array_view->n_offsets = 2;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      array_view->n_offsets = 3;
      break;
    default:
      // i.e., serialized type
      array_view->n_offsets = 1;
      break;
  }

  for (int i = 0; i < 4; i++) {
    array_view->length[i] = 0;
    array_view->offset[i] = 0;
  }

  array_view->validity_bitmap = NULL;
  for (int i = 0; i < 3; i++) {
    array_view->offsets[i] = NULL;
  }
  array_view->data = NULL;

  array_view->coords.n_coords = 0;
  switch (array_view->schema_view.dimensions) {
    case GEOARROW_DIMENSIONS_XY:
      array_view->coords.n_values = 2;
      break;
    case GEOARROW_DIMENSIONS_XYZ:
    case GEOARROW_DIMENSIONS_XYM:
      array_view->coords.n_values = 3;
      break;
    case GEOARROW_DIMENSIONS_XYZM:
      array_view->coords.n_values = 4;
      break;
    default:
      // i.e., serialized type
      array_view->coords.n_coords = 0;
      break;
  }

  if (array_view->schema_view.geometry_type == GEOARROW_GEOMETRY_TYPE_BOX) {
    array_view->coords.n_values *= 2;
  }

  switch (array_view->schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_SEPARATE:
      array_view->coords.coords_stride = 1;
      break;
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      array_view->coords.coords_stride = array_view->coords.n_values;
      break;
    default:
      // i.e., serialized type
      array_view->coords.coords_stride = 0;
      break;
  }

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayViewInitFromType(struct GeoArrowArrayView* array_view,
                                                enum GeoArrowType type) {
  memset(array_view, 0, sizeof(struct GeoArrowArrayView));
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInitFromType(&array_view->schema_view, type));
  return GeoArrowArrayViewInitInternal(array_view);
}

GeoArrowErrorCode GeoArrowArrayViewInitFromSchema(struct GeoArrowArrayView* array_view,
                                                  const struct ArrowSchema* schema,
                                                  struct GeoArrowError* error) {
  memset(array_view, 0, sizeof(struct GeoArrowArrayView));
  NANOARROW_RETURN_NOT_OK(
      GeoArrowSchemaViewInit(&array_view->schema_view, schema, error));
  return GeoArrowArrayViewInitInternal(array_view);
}

static int GeoArrowArrayViewSetArrayInternal(struct GeoArrowArrayView* array_view,
                                             const struct ArrowArray* array,
                                             struct GeoArrowError* error, int level) {
  // Set offset + length of the array
  array_view->offset[level] = array->offset;
  array_view->length[level] = array->length;

  if (level == array_view->n_offsets) {
    // We're at the coord array!

    // n_coords is last_offset[level - 1] or array->length if level == 0
    if (level > 0) {
      int32_t first_offset = array_view->first_offset[level - 1];
      array_view->coords.n_coords = array_view->last_offset[level - 1] - first_offset;
    } else {
      array_view->coords.n_coords = array->length;
    }

    switch (array_view->schema_view.coord_type) {
      case GEOARROW_COORD_TYPE_SEPARATE:
        if (array->n_children != array_view->coords.n_values) {
          GeoArrowErrorSet(error,
                           "Unexpected number of children for struct coordinate array "
                           "in GeoArrowArrayViewSetArray()");
          return EINVAL;
        }

        // Set the coord pointers to the data buffer of each child (applying
        // offset before assigning the pointer)
        for (int32_t i = 0; i < array_view->coords.n_values; i++) {
          if (array->children[i]->n_buffers != 2) {
            ArrowErrorSet(
                (struct ArrowError*)error,
                "Unexpected number of buffers for struct coordinate array child "
                "in GeoArrowArrayViewSetArray()");
            return EINVAL;
          }

          array_view->coords.values[i] = ((const double*)array->children[i]->buffers[1]) +
                                         array->children[i]->offset;
        }

        break;

      case GEOARROW_COORD_TYPE_INTERLEAVED:
        if (array->n_children != 1) {
          GeoArrowErrorSet(
              error,
              "Unexpected number of children for interleaved coordinate array "
              "in GeoArrowArrayViewSetArray()");
          return EINVAL;
        }

        if (array->children[0]->n_buffers != 2) {
          ArrowErrorSet(
              (struct ArrowError*)error,
              "Unexpected number of buffers for interleaved coordinate array child "
              "in GeoArrowArrayViewSetArray()");
          return EINVAL;
        }

        // Set the coord pointers to the first four doubles in the data buffers

        for (int32_t i = 0; i < array_view->coords.n_values; i++) {
          array_view->coords.values[i] = ((const double*)array->children[0]->buffers[1]) +
                                         array->children[0]->offset + i;
        }

        break;

      default:
        GeoArrowErrorSet(error, "Unexpected coordinate type GeoArrowArrayViewSetArray()");
        return EINVAL;
    }

    return GEOARROW_OK;
  }

  if (array->n_buffers != 2) {
    ArrowErrorSet(
        (struct ArrowError*)error,
        "Unexpected number of buffers in list array in GeoArrowArrayViewSetArray()");
    return EINVAL;
  }

  if (array->n_children != 1) {
    ArrowErrorSet(
        (struct ArrowError*)error,
        "Unexpected number of children in list array in GeoArrowArrayViewSetArray()");
    return EINVAL;
  }

  // Set the offsets buffer and the last_offset value of level
  if (array->length > 0) {
    array_view->offsets[level] = (const int32_t*)array->buffers[1];
    array_view->first_offset[level] = array_view->offsets[level][array->offset];
    array_view->last_offset[level] =
        array_view->offsets[level][array->offset + array->length];
  } else {
    array_view->offsets[level] = &kZeroInt32;
    array_view->first_offset[level] = 0;
    array_view->last_offset[level] = 0;
  }

  return GeoArrowArrayViewSetArrayInternal(array_view, array->children[0], error,
                                           level + 1);
}

static GeoArrowErrorCode GeoArrowArrayViewSetArraySerialized(
    struct GeoArrowArrayView* array_view, const struct ArrowArray* array) {
  array_view->length[0] = array->length;
  array_view->offset[0] = array->offset;

  array_view->offsets[0] = (const int32_t*)array->buffers[1];
  array_view->data = (const uint8_t*)array->buffers[2];
  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewSetArrayBox(
    struct GeoArrowArrayView* array_view, const struct ArrowArray* array,
    struct GeoArrowError* error) {
  array_view->length[0] = array->length;
  array_view->offset[0] = array->offset;
  array_view->coords.n_coords = array->length;

  if (array->n_children != array_view->coords.n_values) {
    GeoArrowErrorSet(error,
                     "Unexpected number of children for box array struct "
                     "in GeoArrowArrayViewSetArray()");
    return EINVAL;
  }

  // Set the coord pointers to the data buffer of each child (applying
  // offset before assigning the pointer)
  for (int32_t i = 0; i < array_view->coords.n_values; i++) {
    if (array->children[i]->n_buffers != 2) {
      ArrowErrorSet((struct ArrowError*)error,
                    "Unexpected number of buffers for box array child "
                    "in GeoArrowArrayViewSetArray()");
      return EINVAL;
    }

    array_view->coords.values[i] =
        ((const double*)array->children[i]->buffers[1]) + array->children[i]->offset;
  }

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayViewSetArray(struct GeoArrowArrayView* array_view,
                                            const struct ArrowArray* array,
                                            struct GeoArrowError* error) {
  switch (array_view->schema_view.type) {
    case GEOARROW_TYPE_WKT:
    case GEOARROW_TYPE_WKB:
      NANOARROW_RETURN_NOT_OK(GeoArrowArrayViewSetArraySerialized(array_view, array));
      break;
    case GEOARROW_TYPE_BOX:
    case GEOARROW_TYPE_BOX_Z:
    case GEOARROW_TYPE_BOX_M:
    case GEOARROW_TYPE_BOX_ZM:
      NANOARROW_RETURN_NOT_OK(GeoArrowArrayViewSetArrayBox(array_view, array, error));
      break;
    default:
      NANOARROW_RETURN_NOT_OK(
          GeoArrowArrayViewSetArrayInternal(array_view, array, error, 0));
      break;
  }

  array_view->validity_bitmap = array->buffers[0];
  return GEOARROW_OK;
}

static inline void GeoArrowCoordViewUpdate(const struct GeoArrowCoordView* src,
                                           struct GeoArrowCoordView* dst, int64_t offset,
                                           int64_t length) {
  for (int j = 0; j < dst->n_values; j++) {
    dst->values[j] = src->values[j] + (offset * src->coords_stride);
  }
  dst->n_coords = length;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitNativePoint(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowVisitor* v) {
  struct GeoArrowCoordView coords = array_view->coords;

  for (int64_t i = 0; i < length; i++) {
    NANOARROW_RETURN_NOT_OK(v->feat_start(v));
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POINT,
                                            array_view->schema_view.dimensions));
      GeoArrowCoordViewUpdate(&array_view->coords, &coords,
                              array_view->offset[0] + offset + i, 1);
      NANOARROW_RETURN_NOT_OK(v->coords(v, &coords));
      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
    }

    NANOARROW_RETURN_NOT_OK(v->feat_end(v));

    for (int j = 0; j < coords.n_values; j++) {
      coords.values[j] += coords.coords_stride;
    }
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitNativeLinestring(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowVisitor* v) {
  struct GeoArrowCoordView coords = array_view->coords;

  int64_t coord_offset;
  int64_t n_coords;
  for (int64_t i = 0; i < length; i++) {
    NANOARROW_RETURN_NOT_OK(v->feat_start(v));
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                            array_view->schema_view.dimensions));
      coord_offset = array_view->offsets[0][array_view->offset[0] + offset + i];
      n_coords =
          array_view->offsets[0][array_view->offset[0] + offset + i + 1] - coord_offset;
      coord_offset += array_view->offset[1];
      GeoArrowCoordViewUpdate(&array_view->coords, &coords, coord_offset, n_coords);
      NANOARROW_RETURN_NOT_OK(v->coords(v, &coords));
      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
    }

    NANOARROW_RETURN_NOT_OK(v->feat_end(v));
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitNativePolygon(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowVisitor* v) {
  struct GeoArrowCoordView coords = array_view->coords;

  int64_t ring_offset;
  int64_t n_rings;
  int64_t coord_offset;
  int64_t n_coords;
  for (int64_t i = 0; i < length; i++) {
    NANOARROW_RETURN_NOT_OK(v->feat_start(v));
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POLYGON,
                                            array_view->schema_view.dimensions));
      ring_offset = array_view->offsets[0][array_view->offset[0] + offset + i];
      n_rings =
          array_view->offsets[0][array_view->offset[0] + offset + i + 1] - ring_offset;
      ring_offset += array_view->offset[1];

      for (int64_t j = 0; j < n_rings; j++) {
        NANOARROW_RETURN_NOT_OK(v->ring_start(v));
        coord_offset = array_view->offsets[1][ring_offset + j];
        n_coords = array_view->offsets[1][ring_offset + j + 1] - coord_offset;
        coord_offset += array_view->offset[2];
        GeoArrowCoordViewUpdate(&array_view->coords, &coords, coord_offset, n_coords);
        NANOARROW_RETURN_NOT_OK(v->coords(v, &coords));
        NANOARROW_RETURN_NOT_OK(v->ring_end(v));
      }

      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
    }

    NANOARROW_RETURN_NOT_OK(v->feat_end(v));
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitNativeMultipoint(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowVisitor* v) {
  struct GeoArrowCoordView coords = array_view->coords;

  int64_t coord_offset;
  int64_t n_coords;
  for (int64_t i = 0; i < length; i++) {
    NANOARROW_RETURN_NOT_OK(v->feat_start(v));
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_MULTIPOINT,
                                            array_view->schema_view.dimensions));
      coord_offset = array_view->offsets[0][array_view->offset[0] + offset + i];
      n_coords =
          array_view->offsets[0][array_view->offset[0] + offset + i + 1] - coord_offset;
      coord_offset += array_view->offset[1];
      for (int64_t j = 0; j < n_coords; j++) {
        NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POINT,
                                              array_view->schema_view.dimensions));
        GeoArrowCoordViewUpdate(&array_view->coords, &coords, coord_offset + j, 1);
        NANOARROW_RETURN_NOT_OK(v->coords(v, &coords));
        NANOARROW_RETURN_NOT_OK(v->geom_end(v));
      }
      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
    }

    NANOARROW_RETURN_NOT_OK(v->feat_end(v));
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitNativeMultilinestring(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowVisitor* v) {
  struct GeoArrowCoordView coords = array_view->coords;

  int64_t linestring_offset;
  int64_t n_linestrings;
  int64_t coord_offset;
  int64_t n_coords;
  for (int64_t i = 0; i < length; i++) {
    NANOARROW_RETURN_NOT_OK(v->feat_start(v));
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                                            array_view->schema_view.dimensions));
      linestring_offset = array_view->offsets[0][array_view->offset[0] + offset + i];
      n_linestrings = array_view->offsets[0][array_view->offset[0] + offset + i + 1] -
                      linestring_offset;
      linestring_offset += array_view->offset[1];

      for (int64_t j = 0; j < n_linestrings; j++) {
        NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_LINESTRING,
                                              array_view->schema_view.dimensions));
        coord_offset = array_view->offsets[1][linestring_offset + j];
        n_coords = array_view->offsets[1][linestring_offset + j + 1] - coord_offset;
        coord_offset += array_view->offset[2];
        GeoArrowCoordViewUpdate(&array_view->coords, &coords, coord_offset, n_coords);
        NANOARROW_RETURN_NOT_OK(v->coords(v, &coords));
        NANOARROW_RETURN_NOT_OK(v->geom_end(v));
      }

      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
    }

    NANOARROW_RETURN_NOT_OK(v->feat_end(v));
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitNativeMultipolygon(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowVisitor* v) {
  struct GeoArrowCoordView coords = array_view->coords;

  int64_t polygon_offset;
  int64_t n_polygons;
  int64_t ring_offset;
  int64_t n_rings;
  int64_t coord_offset;
  int64_t n_coords;
  for (int64_t i = 0; i < length; i++) {
    NANOARROW_RETURN_NOT_OK(v->feat_start(v));
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + offset + i)) {
      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                                            array_view->schema_view.dimensions));

      polygon_offset = array_view->offsets[0][array_view->offset[0] + offset + i];
      n_polygons =
          array_view->offsets[0][array_view->offset[0] + offset + i + 1] - polygon_offset;
      polygon_offset += array_view->offset[1];

      for (int64_t j = 0; j < n_polygons; j++) {
        NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POLYGON,
                                              array_view->schema_view.dimensions));

        ring_offset = array_view->offsets[1][polygon_offset + j];
        n_rings = array_view->offsets[1][polygon_offset + j + 1] - ring_offset;
        ring_offset += array_view->offset[2];

        for (int64_t k = 0; k < n_rings; k++) {
          NANOARROW_RETURN_NOT_OK(v->ring_start(v));
          coord_offset = array_view->offsets[2][ring_offset + k];
          n_coords = array_view->offsets[2][ring_offset + k + 1] - coord_offset;
          coord_offset += array_view->offset[3];
          GeoArrowCoordViewUpdate(&array_view->coords, &coords, coord_offset, n_coords);
          NANOARROW_RETURN_NOT_OK(v->coords(v, &coords));
          NANOARROW_RETURN_NOT_OK(v->ring_end(v));
        }

        NANOARROW_RETURN_NOT_OK(v->geom_end(v));
      }

      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
    }

    NANOARROW_RETURN_NOT_OK(v->feat_end(v));
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowArrayViewVisitNativeBox(
    const struct GeoArrowArrayView* array_view, int64_t offset, int64_t length,
    struct GeoArrowVisitor* v) {
  // We aren't going to attempt Z, M, or ZM boxes since there is no canonical
  // way to do this (maybe only if the non-XY dimensions are constant?).
  if (array_view->schema_view.dimensions != GEOARROW_DIMENSIONS_XY) {
    GeoArrowErrorSet(v->error, "Can't visit box with non-XY dimensions");
    return ENOTSUP;
  }

  // These are the polygon coords and the arrays to back them
  struct GeoArrowCoordView poly_coords;
  memset(&poly_coords, 0, sizeof(struct GeoArrowCoordView));

  int n_dim = array_view->coords.n_values / 2;
  double x[5];
  double y[5];
  poly_coords.n_values = n_dim;
  poly_coords.n_coords = 5;
  poly_coords.coords_stride = 1;
  poly_coords.values[0] = x;
  poly_coords.values[1] = y;

  // index into each box coord's values[] for each polygon coordinate
  int box_coord_poly_map_x[] = {0, n_dim, n_dim, 0, 0};
  int box_coord_poly_map_y[] = {1, 1, n_dim + 1, n_dim + 1, 1};

  for (int64_t i = 0; i < length; i++) {
    int64_t raw_offset = array_view->offset[0] + offset + i;
    NANOARROW_RETURN_NOT_OK(v->feat_start(v));
    if (!array_view->validity_bitmap ||
        ArrowBitGet(array_view->validity_bitmap, raw_offset)) {
      // Check for empty dimensions
      int n_empty_dims = 0;
      for (int i = 0; i < n_dim; i++) {
        double dim_min = GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset, i);
        double dim_max =
            GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset, n_dim + i);
        n_empty_dims += dim_min > dim_max;
      }

      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POLYGON,
                                            array_view->schema_view.dimensions));

      // If any dimension has a negative range, we consider the polygon empty
      // (i.e., there are no points for which...)
      if (n_empty_dims == 0) {
        // Populate the polygon coordinates
        for (int i = 0; i < 5; i++) {
          x[i] = GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset,
                                           box_coord_poly_map_x[i]);
          y[i] = GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset,
                                           box_coord_poly_map_y[i]);
        }

        // Call the visitor
        NANOARROW_RETURN_NOT_OK(v->ring_start(v));
        NANOARROW_RETURN_NOT_OK(v->coords(v, &poly_coords));
        NANOARROW_RETURN_NOT_OK(v->ring_end(v));
      }

      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    } else {
      NANOARROW_RETURN_NOT_OK(v->null_feat(v));
    }

    NANOARROW_RETURN_NOT_OK(v->feat_end(v));
  }

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayViewVisitNative(const struct GeoArrowArrayView* array_view,
                                               int64_t offset, int64_t length,
                                               struct GeoArrowVisitor* v) {
  switch (array_view->schema_view.geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_BOX:
      return GeoArrowArrayViewVisitNativeBox(array_view, offset, length, v);
    case GEOARROW_GEOMETRY_TYPE_POINT:
      return GeoArrowArrayViewVisitNativePoint(array_view, offset, length, v);
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      return GeoArrowArrayViewVisitNativeLinestring(array_view, offset, length, v);
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      return GeoArrowArrayViewVisitNativePolygon(array_view, offset, length, v);
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      return GeoArrowArrayViewVisitNativeMultipoint(array_view, offset, length, v);
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      return GeoArrowArrayViewVisitNativeMultilinestring(array_view, offset, length, v);
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      return GeoArrowArrayViewVisitNativeMultipolygon(array_view, offset, length, v);
    default:
      return ENOTSUP;
  }
}

static void GeoArrowArrayViewInitGeometryNode(struct GeoArrowGeometryNode* node) {
  memset(node, 0, sizeof(*node));
  for (int j = 0; j < 4; j++) {
    node->coords[j] = _GeoArrowkEmptyPointCoords;
  }
}

// Array nesting and geometry nesting differ: multipoints have a point node for
// every coordinate, whereas linestrings represent a whole sequence in one node.
// Resolve the geometry levels once, then traverse using only nesting levels.
struct GeoArrowArrayViewGeometryLevels {
  int n_levels;
  enum GeoArrowGeometryType geometry_type[3];
};

static const struct GeoArrowArrayViewGeometryLevels kGeoArrowArrayViewGeometryLevels[] = {
    {0, {GEOARROW_GEOMETRY_TYPE_GEOMETRY}},
    {1, {GEOARROW_GEOMETRY_TYPE_POINT}},
    {1, {GEOARROW_GEOMETRY_TYPE_LINESTRING}},
    {2, {GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_GEOMETRY_TYPE_LINESTRING}},
    {2, {GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_GEOMETRY_TYPE_POINT}},
    {2, {GEOARROW_GEOMETRY_TYPE_MULTILINESTRING, GEOARROW_GEOMETRY_TYPE_LINESTRING}},
    {3,
     {GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_GEOMETRY_TYPE_POLYGON,
      GEOARROW_GEOMETRY_TYPE_LINESTRING}}};

static inline void GeoArrowArrayViewSetGeometryNode(
    const struct GeoArrowArrayView* array_view,
    const struct GeoArrowArrayViewGeometryLevels* levels, int level, uint32_t size,
    struct GeoArrowGeometryNode* node) {
  GeoArrowArrayViewInitGeometryNode(node);
  node->geometry_type = (uint8_t)levels->geometry_type[level];
  node->dimensions = (uint8_t)array_view->schema_view.dimensions;
  node->level = (uint8_t)level;
  node->size = size;
}

static inline void GeoArrowArrayViewSetGeometryCoords(
    const struct GeoArrowArrayView* array_view, int64_t offset,
    struct GeoArrowGeometryNode* node) {
  // Empty sequences retain readable NaN coordinates, even when the array has
  // no coordinate buffers or this sequence is at the end of a buffer.
  if (node->size == 0) {
    return;
  }

  for (int j = 0; j < array_view->coords.n_values; j++) {
    node->coords[j] = (const uint8_t*)(array_view->coords.values[j] +
                                       offset * array_view->coords.coords_stride);
    node->coord_stride[j] = array_view->coords.coords_stride * sizeof(double);
  }
}

struct GeoArrowArrayViewGeometryRange {
  int64_t begin;
  uint32_t size;
};

static inline struct GeoArrowArrayViewGeometryRange GeoArrowArrayViewGeometryRangeAt(
    const struct GeoArrowArrayView* array_view, int level, int64_t i) {
  struct GeoArrowArrayViewGeometryRange range;
  range.begin = array_view->offsets[level][i];
  range.size = (uint32_t)(array_view->offsets[level][i + 1] - range.begin);
  range.begin += array_view->offset[level + 1];
  return range;
}

static void GeoArrowArrayViewSetGeometryNodes(
    const struct GeoArrowArrayView* array_view,
    const struct GeoArrowArrayViewGeometryLevels* levels, int64_t i,
    struct GeoArrowGeometryNode* node) {
  struct GeoArrowArrayViewGeometryRange range = {array_view->offset[0] + i, 1};
  if (array_view->n_offsets > 0) {
    range = GeoArrowArrayViewGeometryRangeAt(array_view, 0, range.begin);
  }

  GeoArrowArrayViewSetGeometryNode(array_view, levels, 0, range.size, node);
  if (levels->n_levels == 1) {
    GeoArrowArrayViewSetGeometryCoords(array_view, range.begin, node);
    return;
  }

  ++node;
  if (levels->n_levels == 2) {
    if (array_view->n_offsets == 1) {
      // The leaf array contains individual points rather than sequences.
      for (uint32_t j = 0; j < range.size; j++, node++) {
        GeoArrowArrayViewSetGeometryNode(array_view, levels, 1, 1, node);
        GeoArrowArrayViewSetGeometryCoords(array_view, range.begin + j, node);
      }
    } else {
      for (uint32_t j = 0; j < range.size; j++, node++) {
        struct GeoArrowArrayViewGeometryRange coords =
            GeoArrowArrayViewGeometryRangeAt(array_view, 1, range.begin + j);
        GeoArrowArrayViewSetGeometryNode(array_view, levels, 1, coords.size, node);
        GeoArrowArrayViewSetGeometryCoords(array_view, coords.begin, node);
      }
    }
    return;
  }

  // Native geometries have at most three geometry levels. Explicit loops let
  // the compiler specialize the node initialization for each array level.
  for (uint32_t j = 0; j < range.size; j++) {
    struct GeoArrowArrayViewGeometryRange children =
        GeoArrowArrayViewGeometryRangeAt(array_view, 1, range.begin + j);
    GeoArrowArrayViewSetGeometryNode(array_view, levels, 1, children.size, node++);
    for (uint32_t k = 0; k < children.size; k++, node++) {
      struct GeoArrowArrayViewGeometryRange coords =
          GeoArrowArrayViewGeometryRangeAt(array_view, 2, children.begin + k);
      GeoArrowArrayViewSetGeometryNode(array_view, levels, 2, coords.size, node);
      GeoArrowArrayViewSetGeometryCoords(array_view, coords.begin, node);
    }
  }
}

static GeoArrowErrorCode GeoArrowArrayViewGetBoxGeometry(
    const struct GeoArrowArrayView* array_view, int64_t i, struct GeoArrowGeometry* out) {
  int64_t raw_offset = array_view->offset[0] + i;
  double xmin = GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset, 0);
  double ymin = GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset, 1);
  double xmax = GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset, 2);
  double ymax = GEOARROW_COORD_VIEW_VALUE(&array_view->coords, raw_offset, 3);
  return GeoArrowGeometrySetBox(out, xmin, ymin, xmax, ymax);
}

GeoArrowErrorCode GeoArrowArrayViewGetGeometry(const struct GeoArrowArrayView* array_view,
                                               int64_t i, struct GeoArrowGeometry* out) {
  enum GeoArrowGeometryType type = array_view->schema_view.geometry_type;
  switch (type) {
    case GEOARROW_GEOMETRY_TYPE_BOX:
      if (array_view->schema_view.dimensions != GEOARROW_DIMENSIONS_XY) {
        return ENOTSUP;
      }
      break;
    case GEOARROW_GEOMETRY_TYPE_POINT:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      break;
    default:
      return ENOTSUP;
  }

  if (i < 0 || i >= array_view->length[0]) {
    return EINVAL;
  }

  if (array_view->validity_bitmap != NULL &&
      !ArrowBitGet(array_view->validity_bitmap, array_view->offset[0] + i)) {
    GEOARROW_RETURN_NOT_OK(GeoArrowGeometryResizeNodesInline(out, 1));
    GeoArrowArrayViewInitGeometryNode(out->root);
    return GEOARROW_OK;
  }

  if (type == GEOARROW_GEOMETRY_TYPE_BOX) {
    return GeoArrowArrayViewGetBoxGeometry(array_view, i, out);
  }

  const struct GeoArrowArrayViewGeometryLevels* levels =
      &kGeoArrowArrayViewGeometryLevels[type];

  // Count nodes before allocating so allocation failure leaves out unchanged.
  // Each geometry level contributes one node per child at that array level.
  int n_child_levels = levels->n_levels - 1;

  int64_t size_nodes = 1;
  int64_t begin = i;
  int64_t end = i + 1;
  for (int level = 0; level < n_child_levels && begin < end; level++) {
    begin = array_view->offsets[level][array_view->offset[level] + begin];
    end = array_view->offsets[level][array_view->offset[level] + end];
    size_nodes += end - begin;
  }

  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryResizeNodesInline(out, size_nodes));
  GeoArrowArrayViewSetGeometryNodes(array_view, levels, i, out->root);
  return GEOARROW_OK;
}
