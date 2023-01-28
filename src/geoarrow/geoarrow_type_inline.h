
#ifndef GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED
#define GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED

#include <string.h>

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

// Such that kNumOffsets[geometry_type] gives the right answer
static int _GeoArrowkNumOffsets[] = {-1, 0, 1, 2, 1, 2, 3, -1};

// Such that kNumDimensions[dimensions] gives the right answer
static int _GeoArrowkNumDimensions[] = {-1, 2, 3, 3, 4};

static inline int GeoArrowBuilderBufferCheck(struct GeoArrowBuilder* builder, int64_t i,
                                             int64_t additional_size_bytes) {
  return builder->view.buffers[i].capacity_bytes >=
         (builder->view.buffers[i].size_bytes + additional_size_bytes);
}

static inline void GeoArrowBuilderAppendBufferUnsafe(struct GeoArrowBuilder* builder,
                                                     int64_t i,
                                                     struct GeoArrowBufferView value) {
  struct GeoArrowWritableBufferView* buffer = builder->view.buffers + i;
  memcpy(buffer->data.as_uint8 + buffer->size_bytes, value.data, value.n_bytes);
  buffer->size_bytes += value.n_bytes;
}

// This could probably be or use a lookup table at some point
static inline void GeoArrowMapDimensions(enum GeoArrowDimensions src_dim,
                                    enum GeoArrowDimensions dst_dim, int* dim_map) {
  dim_map[0] = 0;
  dim_map[1] = 1;
  dim_map[2] = -1;
  dim_map[3] = -1;

  switch (dst_dim) {
    case GEOARROW_DIMENSIONS_XYM:
      switch (src_dim) {
        case GEOARROW_DIMENSIONS_XYM:
          dim_map[2] = 2;
          break;
        case GEOARROW_DIMENSIONS_XYZM:
          dim_map[2] = 3;
          break;
        default:
          break;
      }
      break;

    case GEOARROW_DIMENSIONS_XYZ:
      switch (src_dim) {
        case GEOARROW_DIMENSIONS_XYZ:
        case GEOARROW_DIMENSIONS_XYZM:
          dim_map[2] = 2;
          break;
        default:
          break;
      }
      break;

    case GEOARROW_DIMENSIONS_XYZM:
      switch (src_dim) {
        case GEOARROW_DIMENSIONS_XYZ:
          dim_map[2] = 2;
          break;
        case GEOARROW_DIMENSIONS_XYM:
          dim_map[3] = 2;
          break;
        case GEOARROW_DIMENSIONS_XYZM:
          dim_map[2] = 2;
          dim_map[3] = 3;
          break;
        default:
          break;
      }
      break;

    default:
      break;
  }
}

// Four little-endian NANs
static uint8_t _GeoArrowkEmptyPointCoords[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xf8, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf8, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};

// Copies coordinates from one view to another keeping dimensions the same.
// This function fills dimensions in dst but not in src with NAN; dimensions
// in src but not in dst are dropped. This is useful for generic copying of
// small sequences (e.g., the builder) but shouldn't be used when there is some
// prior knowledge of the coordinate type.
static inline void GeoArrowCoordViewCopy(struct GeoArrowCoordView* src,
                                         enum GeoArrowDimensions src_dim,
                                         int64_t src_offset,
                                         struct GeoArrowWritableCoordView* dst,
                                         enum GeoArrowDimensions dst_dim,
                                         int64_t dst_offset, int64_t n) {
  // Copy the XYs
  for (int64_t i = 0; i < n; i++) {
    GEOARROW_COORD_VIEW_VALUE(dst, dst_offset + i, 0) =
        GEOARROW_COORD_VIEW_VALUE(src, src_offset + i, 0);
    GEOARROW_COORD_VIEW_VALUE(dst, dst_offset + i, 1) =
        GEOARROW_COORD_VIEW_VALUE(src, src_offset + i, 1);
  }

  if (dst->n_values == 2) {
    return;
  }

  int dst_dim_map[4];
  GeoArrowMapDimensions(src_dim, dst_dim, dst_dim_map);

  if (dst_dim_map[2] == -1) {
    for (int64_t i = 0; i < n; i++) {
      memcpy(&(GEOARROW_COORD_VIEW_VALUE(dst, dst_offset + i, 2)),
             _GeoArrowkEmptyPointCoords, sizeof(double));
    }
  } else {
    for (int64_t i = 0; i < n; i++) {
      GEOARROW_COORD_VIEW_VALUE(dst, dst_offset + i, 2) =
          GEOARROW_COORD_VIEW_VALUE(src, src_offset + i, dst_dim_map[2]);
    }
  }

  if (dst->n_values == 3) {
    return;
  }

  if (dst_dim_map[3] == -1) {
    for (int64_t i = 0; i < n; i++) {
      memcpy(&(GEOARROW_COORD_VIEW_VALUE(dst, dst_offset + i, 3)),
             _GeoArrowkEmptyPointCoords, sizeof(double));
    }
  } else {
    for (int64_t i = 0; i < n; i++) {
      GEOARROW_COORD_VIEW_VALUE(dst, dst_offset + i, 3) =
          GEOARROW_COORD_VIEW_VALUE(src, src_offset + i, dst_dim_map[3]);
    }
  }
}

struct _GeoArrowFindBufferResult {
  struct ArrowArray* array;
  int level;
  int i;
};

static inline int _GeoArrowArrayFindBuffer(struct ArrowArray* array,
                                           struct _GeoArrowFindBufferResult* res, int i,
                                           int level, int skip_first) {
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
