
#include <errno.h>

#include "geoarrow.h"

#include "nanoarrow.h"

static int32_t kZeroInt32 = 0;

static int GeoArrowArrayViewInitInternal(struct GeoArrowArrayView* array_view,
                                         struct GeoArrowError* error) {
  switch (array_view->schema_view.geometry_type) {
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
      ArrowErrorSet((struct ArrowError*)error,
                    "Unsupported geometry type in GeoArrowArrayViewInit()");
      return EINVAL;
  }

  array_view->length = 0;
  array_view->validity_bitmap = NULL;
  for (int i = 0; i < 4; i++) {
    array_view->offsets[i] = NULL;
  }

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
      ArrowErrorSet((struct ArrowError*)error,
                    "Unsupported dimensions in GeoArrowArrayViewInit()");
      return EINVAL;
  }

  switch (array_view->schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_SEPARATE:
      array_view->coords.coords_stride = 1;
      break;
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      array_view->coords.coords_stride = array_view->coords.n_values;
      break;
    default:
      ArrowErrorSet((struct ArrowError*)error,
                    "Unsupported coord type in GeoArrowArrayViewInit()");
      return EINVAL;
  }

  for (int i = 0; i < 4; i++) {
    array_view->coords.values[i] = NULL;
  }

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowArrayViewInitFromType(struct GeoArrowArrayView* array_view,
                                                enum GeoArrowType type) {
  NANOARROW_RETURN_NOT_OK(GeoArrowSchemaViewInitFromType(&array_view->schema_view, type));
  return GeoArrowArrayViewInitInternal(array_view, NULL);
}

GeoArrowErrorCode GeoArrowArrayViewInitFromSchema(struct GeoArrowArrayView* array_view,
                                                  struct ArrowSchema* schema,
                                                  struct GeoArrowError* error) {
  NANOARROW_RETURN_NOT_OK(
      GeoArrowSchemaViewInit(&array_view->schema_view, schema, error));
  return GeoArrowArrayViewInitInternal(array_view, error);
}

static int GeoArrowArrayViewSetArrayInternal(struct GeoArrowArrayView* array_view,
                                             struct ArrowArray* array,
                                             struct GeoArrowError* error, int level) {
  if (array->offset != 0) {
    // This should be supported at some point
    ArrowErrorSet((struct ArrowError*)error,
                  "ArrowArray with offset != 0 is not yet supported in "
                  "GeoArrowArrayViewSetArray()");
    return ENOTSUP;
  }

  if (level == array_view->n_offsets) {
    // We're at the coord array!

    // n_coords is lengths[level - 1] or array->length if level == 0
    if (level > 0) {
      array_view->coords.n_coords = array_view->lengths[level - 1];
    } else {
      array_view->coords.n_coords = array->length;
    }

    switch (array_view->schema_view.coord_type) {
      case GEOARROW_COORD_TYPE_SEPARATE:
        if (array->n_children != array_view->coords.n_values) {
          ArrowErrorSet((struct ArrowError*)error,
                        "Unexpected number of children for struct coordinate array "
                        "in GeoArrowArrayViewSetArray()");
          return EINVAL;
        }

        // Set the coord pointers to the data buffer of each child
        for (int32_t i = 0; i < array_view->coords.n_values; i++) {
          if (array->children[i]->n_buffers != 2) {
            ArrowErrorSet(
                (struct ArrowError*)error,
                "Unexpected number of buffers for struct coordinate array child "
                "in GeoArrowArrayViewSetArray()");
            return EINVAL;
          }

          array_view->coords.values[i] = ((const double*)array->children[i]->buffers[1]);
        }

        break;

      case GEOARROW_COORD_TYPE_INTERLEAVED:
        if (array->n_children != 1) {
          ArrowErrorSet((struct ArrowError*)error,
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
          array_view->coords.values[i] =
              ((const double*)array->children[0]->buffers[1]) + i;
        }

        break;

      default:
        ArrowErrorSet((struct ArrowError*)error,
                      "Unexpected coordinate type GeoArrowArrayViewSetArray()");
        return EINVAL;
    }

    return GEOARROW_OK;
  }

  if (array->n_buffers != 2) {
    ArrowErrorSet((struct ArrowError*)error,
                  "Unexpected number of buffers in GeoArrowArrayViewSetArray()");
    return EINVAL;
  }

  if (array->n_children != 1) {
    ArrowErrorSet((struct ArrowError*)error,
                  "Unexpected number of children in GeoArrowArrayViewSetArray()");
    return EINVAL;
  }

  // Set the offsets buffer and the lengths value of level + 1
  if (array->length > 0) {
    array_view->offsets[level] = (const int32_t*)array->buffers[1];
    array_view->lengths[level + 1] =
        array_view->offsets[level][array->offset + array->length];
  } else {
    array_view->offsets[level] = &kZeroInt32;
    array_view->lengths[level + 1] = 0;
  }

  return GeoArrowArrayViewSetArrayInternal(array_view, array->children[0], error,
                                           level + 1);
}

GeoArrowErrorCode GeoArrowArrayViewSetArray(struct GeoArrowArrayView* array_view,
                                            struct ArrowArray* array,
                                            struct GeoArrowError* error) {
  NANOARROW_RETURN_NOT_OK(GeoArrowArrayViewSetArrayInternal(array_view, array, error, 0));

  return GEOARROW_OK;
}
