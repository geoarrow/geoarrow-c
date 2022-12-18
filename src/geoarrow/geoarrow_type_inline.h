
#ifndef GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED
#define GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED

#include "geoarrow_type.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline const char* GeoArrowExtensionNameFromType(enum GeoArrowType type) {
  switch (type) {
    case GEOARROW_TYPE_WKB:
    case GEOARROW_TYPE_LARGE_WKB:
      return "geoarrow.wkb";

    case GEOARROW_TYPE_POINT:
    case GEOARROW_TYPE_POINT_Z:
    case GEOARROW_TYPE_POINT_M:
    case GEOARROW_TYPE_POINT_ZM:
      return "geoarrow.point";

    case GEOARROW_TYPE_LINESTRING:
    case GEOARROW_TYPE_LINESTRING_Z:
    case GEOARROW_TYPE_LINESTRING_M:
    case GEOARROW_TYPE_LINESTRING_ZM:
      return "geoarrow.linestring";

    case GEOARROW_TYPE_POLYGON:
    case GEOARROW_TYPE_POLYGON_Z:
    case GEOARROW_TYPE_POLYGON_M:
    case GEOARROW_TYPE_POLYGON_ZM:
      return "geoarrow.polygon";

    case GEOARROW_TYPE_MULTIPOINT:
    case GEOARROW_TYPE_MULTIPOINT_Z:
    case GEOARROW_TYPE_MULTIPOINT_M:
    case GEOARROW_TYPE_MULTIPOINT_ZM:
      return "geoarrow.multipoint";

    case GEOARROW_TYPE_MULTILINESTRING:
    case GEOARROW_TYPE_MULTILINESTRING_Z:
    case GEOARROW_TYPE_MULTILINESTRING_M:
    case GEOARROW_TYPE_MULTILINESTRING_ZM:
      return "geoarrow.multilinestring";

    case GEOARROW_TYPE_MULTIPOLYGON:
    case GEOARROW_TYPE_MULTIPOLYGON_Z:
    case GEOARROW_TYPE_MULTIPOLYGON_M:
    case GEOARROW_TYPE_MULTIPOLYGON_ZM:
      return "geoarrow.multipolygon";

    default:
      return NULL;
  }
}

static inline enum GeoArrowGeometryType GeoArrowGeometryTypeFromType(
    enum GeoArrowType type) {
  switch (type) {
    case GEOARROW_TYPE_UNINITIALIZED:
      return GEOARROW_GEOMETRY_TYPE_GEOMETRY;
    case GEOARROW_TYPE_POINT:
    case GEOARROW_TYPE_POINT_Z:
    case GEOARROW_TYPE_POINT_M:
    case GEOARROW_TYPE_POINT_ZM:
      return GEOARROW_GEOMETRY_TYPE_POINT;

    case GEOARROW_TYPE_LINESTRING:
    case GEOARROW_TYPE_LINESTRING_Z:
    case GEOARROW_TYPE_LINESTRING_M:
    case GEOARROW_TYPE_LINESTRING_ZM:
      return GEOARROW_GEOMETRY_TYPE_LINESTRING;

    case GEOARROW_TYPE_POLYGON:
    case GEOARROW_TYPE_POLYGON_Z:
    case GEOARROW_TYPE_POLYGON_M:
    case GEOARROW_TYPE_POLYGON_ZM:
      return GEOARROW_GEOMETRY_TYPE_POLYGON;

    case GEOARROW_TYPE_MULTIPOINT:
    case GEOARROW_TYPE_MULTIPOINT_Z:
    case GEOARROW_TYPE_MULTIPOINT_M:
    case GEOARROW_TYPE_MULTIPOINT_ZM:
      return GEOARROW_GEOMETRY_TYPE_MULTIPOINT;

    case GEOARROW_TYPE_MULTILINESTRING:
    case GEOARROW_TYPE_MULTILINESTRING_Z:
    case GEOARROW_TYPE_MULTILINESTRING_M:
    case GEOARROW_TYPE_MULTILINESTRING_ZM:
      return GEOARROW_GEOMETRY_TYPE_MULTILINESTRING;

    case GEOARROW_TYPE_MULTIPOLYGON:
    case GEOARROW_TYPE_MULTIPOLYGON_Z:
    case GEOARROW_TYPE_MULTIPOLYGON_M:
    case GEOARROW_TYPE_MULTIPOLYGON_ZM:
      return GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON;

    default:
      return GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  }
}

static inline enum GeoArrowDimensions GeoArrowDimensionsFromType(enum GeoArrowType type) {
  switch (type) {
    case GEOARROW_TYPE_UNINITIALIZED:
      return GEOARROW_DIMENSIONS_UNKNOWN;
    case GEOARROW_TYPE_POINT:
    case GEOARROW_TYPE_LINESTRING:
    case GEOARROW_TYPE_POLYGON:
    case GEOARROW_TYPE_MULTIPOINT:
    case GEOARROW_TYPE_MULTILINESTRING:
    case GEOARROW_TYPE_MULTIPOLYGON:
      return GEOARROW_DIMENSIONS_XY;

    case GEOARROW_TYPE_POINT_Z:
    case GEOARROW_TYPE_LINESTRING_Z:
    case GEOARROW_TYPE_POLYGON_Z:
    case GEOARROW_TYPE_MULTIPOINT_Z:
    case GEOARROW_TYPE_MULTILINESTRING_Z:
    case GEOARROW_TYPE_MULTIPOLYGON_Z:
      return GEOARROW_DIMENSIONS_XYZ;

    case GEOARROW_TYPE_POINT_M:
    case GEOARROW_TYPE_LINESTRING_M:
    case GEOARROW_TYPE_POLYGON_M:
    case GEOARROW_TYPE_MULTIPOINT_M:
    case GEOARROW_TYPE_MULTILINESTRING_M:
    case GEOARROW_TYPE_MULTIPOLYGON_M:
      return GEOARROW_DIMENSIONS_XYM;

    case GEOARROW_TYPE_POINT_ZM:
    case GEOARROW_TYPE_LINESTRING_ZM:
    case GEOARROW_TYPE_POLYGON_ZM:
    case GEOARROW_TYPE_MULTIPOINT_ZM:
    case GEOARROW_TYPE_MULTILINESTRING_ZM:
    case GEOARROW_TYPE_MULTIPOLYGON_ZM:
      return GEOARROW_DIMENSIONS_XYZM;

    default:
      return GEOARROW_DIMENSIONS_UNKNOWN;
  }
}

static inline enum GeoArrowCoordType GeoArrowCoordTypeFromType(enum GeoArrowType type) {
  switch (type) {
    case GEOARROW_TYPE_UNINITIALIZED:
      return GEOARROW_COORD_TYPE_UNKNOWN;
    case GEOARROW_TYPE_POINT:
    case GEOARROW_TYPE_LINESTRING:
    case GEOARROW_TYPE_POLYGON:
    case GEOARROW_TYPE_MULTIPOINT:
    case GEOARROW_TYPE_MULTILINESTRING:
    case GEOARROW_TYPE_MULTIPOLYGON:
    case GEOARROW_TYPE_POINT_Z:
    case GEOARROW_TYPE_LINESTRING_Z:
    case GEOARROW_TYPE_POLYGON_Z:
    case GEOARROW_TYPE_MULTIPOINT_Z:
    case GEOARROW_TYPE_MULTILINESTRING_Z:
    case GEOARROW_TYPE_MULTIPOLYGON_Z:
    case GEOARROW_TYPE_POINT_M:
    case GEOARROW_TYPE_LINESTRING_M:
    case GEOARROW_TYPE_POLYGON_M:
    case GEOARROW_TYPE_MULTIPOINT_M:
    case GEOARROW_TYPE_MULTILINESTRING_M:
    case GEOARROW_TYPE_MULTIPOLYGON_M:
    case GEOARROW_TYPE_POINT_ZM:
    case GEOARROW_TYPE_LINESTRING_ZM:
    case GEOARROW_TYPE_POLYGON_ZM:
    case GEOARROW_TYPE_MULTIPOINT_ZM:
    case GEOARROW_TYPE_MULTILINESTRING_ZM:
    case GEOARROW_TYPE_MULTIPOLYGON_ZM:
      return GEOARROW_COORD_TYPE_SEPARATE;

    default:
      return GEOARROW_COORD_TYPE_UNKNOWN;
  }
}

static inline enum GeoArrowType GeoArrowMakeType(enum GeoArrowGeometryType geometry_type,
                                                 enum GeoArrowDimensions dimensions,
                                                 enum GeoArrowCoordType coord_type) {
  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      switch (dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POINT;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZ:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POINT_Z;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POINT_M;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POINT_ZM;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        default:
          return GEOARROW_TYPE_UNINITIALIZED;
      }
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      switch (dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_LINESTRING;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZ:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_LINESTRING_Z;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_LINESTRING_M;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_LINESTRING_ZM;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        default:
          return GEOARROW_TYPE_UNINITIALIZED;
      }
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      switch (dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POLYGON;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZ:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POLYGON_Z;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POLYGON_M;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_POLYGON_ZM;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        default:
          return GEOARROW_TYPE_UNINITIALIZED;
      }
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      switch (dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOINT;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZ:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOINT_Z;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOINT_M;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOINT_ZM;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        default:
          return GEOARROW_TYPE_UNINITIALIZED;
      }
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      switch (dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTILINESTRING;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZ:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTILINESTRING_Z;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTILINESTRING_M;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTILINESTRING_ZM;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        default:
          return GEOARROW_TYPE_UNINITIALIZED;
      }
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      switch (dimensions) {
        case GEOARROW_DIMENSIONS_XY:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOLYGON;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZ:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOLYGON_Z;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOLYGON_M;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        case GEOARROW_DIMENSIONS_XYZM:
          switch (coord_type) {
            case GEOARROW_COORD_TYPE_SEPARATE:
              return GEOARROW_TYPE_MULTIPOLYGON_ZM;
            default:
              return GEOARROW_TYPE_UNINITIALIZED;
          }
        default:
          return GEOARROW_TYPE_UNINITIALIZED;
      }
    default:
      return GEOARROW_TYPE_UNINITIALIZED;
  }
}

static inline const char* GeoArrowGeometryTypeString(
    enum GeoArrowGeometryType geometry_type) {
  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      return "POINT";
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      return "LINESTRING";
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      return "POLYGON";
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      return "MULTIPOINT";
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      return "MULTILINESTRING";
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      return "MULTIPOLYGON";
    case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION:
      return "GEOMETRYCOLLECTION";
    default:
      return NULL;
  }
}

struct _GeoArrowFindBufferResult {
  struct ArrowArray* array;
  int level;
  int i;
};

static inline int _GeoArrowArrayFindBuffer(struct ArrowArray* array,
                                   struct _GeoArrowFindBufferResult* res, int i, int level,
                                   int skip_first) {
  int total_buffers = (array->n_buffers - skip_first);
  if (i < total_buffers) {
    res->array = array;
    res->i = i + skip_first;
    res->level = level;
    return total_buffers;
  }

  i -= total_buffers;

  for (int64_t child_id = 0; child_id < array->n_children; child_id++) {
    int child_buffers =
        _GeoArrowArrayFindBuffer(array->children[child_id], res, i, level + 1, 1);
    total_buffers += child_buffers;
    if (i < child_buffers) {
      return total_buffers;
    }
    i -= child_buffers;
  }

  return total_buffers;
}

#ifdef __cplusplus
}
#endif

#endif
