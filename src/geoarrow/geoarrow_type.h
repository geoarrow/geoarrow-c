
#ifndef GEOARROW_GEOARROW_TYPES_H_INCLUDED
#define GEOARROW_GEOARROW_TYPES_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum GeoArrowType {
  GEOARROW_TYPE_UNINITIALIZED,

  GEOARROW_TYPE_WKB,
  GEOARROW_TYPE_LARGE_WKB,

  GEOARROW_TYPE_POINT,
  GEOARROW_TYPE_LINESTRING,
  GEOARROW_TYPE_POLYGON,
  GEOARROW_TYPE_MULTIPOINT,
  GEOARROW_TYPE_MULTILINESTRING,
  GEOARROW_TYPE_MULTIPOLYGON,

  GEOARROW_TYPE_POINT_Z,
  GEOARROW_TYPE_LINESTRING_Z,
  GEOARROW_TYPE_POLYGON_Z,
  GEOARROW_TYPE_MULTIPOINT_Z,
  GEOARROW_TYPE_MULTILINESTRING_Z,
  GEOARROW_TYPE_MULTIPOLYGON_Z,

  GEOARROW_TYPE_POINT_M,
  GEOARROW_TYPE_LINESTRING_M,
  GEOARROW_TYPE_POLYGON_M,
  GEOARROW_TYPE_MULTIPOINT_M,
  GEOARROW_TYPE_MULTILINESTRING_M,
  GEOARROW_TYPE_MULTIPOLYGON_M,

  GEOARROW_TYPE_POINT_ZM,
  GEOARROW_TYPE_LINESTRING_ZM,
  GEOARROW_TYPE_POLYGON_ZM,
  GEOARROW_TYPE_MULTIPOINT_ZM,
  GEOARROW_TYPE_MULTILINESTRING_ZM,
  GEOARROW_TYPE_MULTIPOLYGON_ZM
};

enum GeoArrowGeometryType {
  GEOARROW_GEOMETRY_TYPE_GEOMETRY = 0,
  GEOARROW_GEOMETRY_TYPE_POINT = 1,
  GEOARROW_GEOMETRY_TYPE_LINESTRING = 2,
  GEOARROW_GEOMETRY_TYPE_POLYGON = 3,
  GEOARROW_GEOMETRY_TYPE_MULTIPOINT = 4,
  GEOARROW_GEOMETRY_TYPE_MULTILINESTRING = 5,
  GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON = 6,
  GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION = 7
};

enum GeoArrowDimensions {
  GEOARROW_DIMENSIONS_UNKNOWN,
  GEOARROW_DIMENSIONS_XY,
  GEOARROW_DIMENSIONS_XYZ,
  GEOARROW_DIMENSIONS_XYM,
  GEOARROW_DIMENSIONS_XYZM
};

enum GeoArrowCoordType { GEOARROW_COORD_TYPE_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE };

enum GeoArrowEdgeType { GEOARROW_EDGE_TYPE_PLANAR, GEOARROW_EDGE_TYPE_SPHERICAL };

enum GeoArrowCrsType {
  GEOARROW_CRS_TYPE_NONE,
  GEOARROW_CRS_TYPE_UNKNOWN,
  GEOARROW_CRS_TYPE_PROJJSON
};

struct GeoArrowVisitor {
  int (*reserve_coord)(struct GeoArrowVisitor* v, int64_t n);
  int (*reserve_feat)(struct GeoArrowVisitor* v, int64_t n);

  int (*feat_start)(struct GeoArrowVisitor* v);
  int (*null_feat)(struct GeoArrowVisitor* v);
  int (*geom_start)(struct GeoArrowVisitor* v, enum GeoArrowGeometryType geometry_type,
                    enum GeoArrowDimensions dimensions);
  int (*ring_start)(struct GeoArrowVisitor* v);
  int (*coords)(struct GeoArrowVisitor* v, const double** values, int64_t n_coords,
                int32_t n_dims);
  int (*ring_end)(struct GeoArrowVisitor* v);
  int (*geom_end)(struct GeoArrowVisitor* v);
  int (*feat_end)(struct GeoArrowVisitor* v);

  void* private_data;
};

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

#ifdef __cplusplus
}
#endif

#endif
