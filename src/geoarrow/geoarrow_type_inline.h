
#ifndef GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED
#define GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED

#include <stddef.h>
#include <string.h>

#include "geoarrow_type.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline enum GeoArrowGeometryType GeoArrowGeometryTypeFromType(
    enum GeoArrowType type) {
  switch (type) {
    case GEOARROW_TYPE_UNINITIALIZED:
    case GEOARROW_TYPE_WKB:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKT:
    case GEOARROW_TYPE_LARGE_WKT:
      return GEOARROW_GEOMETRY_TYPE_GEOMETRY;

    default:
      break;
  }

  int type_int = type;

  if (type_int >= GEOARROW_TYPE_INTERLEAVED_POINT) {
    type_int -= 10000;
  }

  if (type_int >= 4000) {
    type_int -= 4000;
  } else if (type_int >= 3000) {
    type_int -= 3000;
  } else if (type_int >= 2000) {
    type_int -= 2000;
  } else if (type_int >= 1000) {
    type_int -= 1000;
  }

  if (type_int > 6 || type_int < 1) {
    return GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  } else {
    return (enum GeoArrowGeometryType)type_int;
  }
}

static inline const char* GeoArrowExtensionNameFromType(enum GeoArrowType type) {
  switch (type) {
    case GEOARROW_TYPE_WKB:
    case GEOARROW_TYPE_LARGE_WKB:
      return "geoarrow.wkb";
    case GEOARROW_TYPE_WKT:
    case GEOARROW_TYPE_LARGE_WKT:
      return "geoarrow.wkt";

    default:
      break;
  }

  int geometry_type = GeoArrowGeometryTypeFromType(type);
  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      return "geoarrow.point";
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      return "geoarrow.linestring";
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      return "geoarrow.polygon";
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      return "geoarrow.multipoint";
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      return "geoarrow.multilinestring";
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      return "geoarrow.multipolygon";
    default:
      return NULL;
  }
}

static inline enum GeoArrowDimensions GeoArrowDimensionsFromType(enum GeoArrowType type) {
  switch (type) {
    case GEOARROW_TYPE_UNINITIALIZED:
    case GEOARROW_TYPE_WKB:
    case GEOARROW_TYPE_LARGE_WKB:
    case GEOARROW_TYPE_WKT:
    case GEOARROW_TYPE_LARGE_WKT:
      return GEOARROW_DIMENSIONS_UNKNOWN;

    default:
      break;
  }

  int geometry_type = GeoArrowGeometryTypeFromType(type);
  int type_int = type;
  type_int -= geometry_type;
  if (type_int > 5000) {
    type_int -= 10000;
  }

  switch (type_int) {
    case 0:
      return GEOARROW_DIMENSIONS_XY;
    case 1000:
      return GEOARROW_DIMENSIONS_XYZ;
    case 2000:
      return GEOARROW_DIMENSIONS_XYM;
    case 3000:
      return GEOARROW_DIMENSIONS_XYZM;
    default:
      return GEOARROW_DIMENSIONS_UNKNOWN;
  }
}

static inline enum GeoArrowCoordType GeoArrowCoordTypeFromType(enum GeoArrowType type) {
  if (type >= GEOARROW_TYPE_WKB) {
    return GEOARROW_COORD_TYPE_UNKNOWN;
  } else if (type >= GEOARROW_TYPE_INTERLEAVED_POINT) {
    return GEOARROW_COORD_TYPE_INTERLEAVED;
  } else if (type >= GEOARROW_TYPE_POINT) {
    return GEOARROW_COORD_TYPE_SEPARATE;
  } else {
    return GEOARROW_COORD_TYPE_UNKNOWN;
  }
}

static inline enum GeoArrowType GeoArrowMakeType(enum GeoArrowGeometryType geometry_type,
                                                 enum GeoArrowDimensions dimensions,
                                                 enum GeoArrowCoordType coord_type) {
  if (geometry_type == GEOARROW_GEOMETRY_TYPE_GEOMETRY) {
    return GEOARROW_TYPE_UNINITIALIZED;
  } else if (dimensions == GEOARROW_DIMENSIONS_UNKNOWN) {
    return GEOARROW_TYPE_UNINITIALIZED;
  } else if (coord_type == GEOARROW_COORD_TYPE_UNKNOWN) {
    return GEOARROW_TYPE_UNINITIALIZED;
  }

  int type_int = (dimensions - 1) * 1000 + (coord_type - 1) * 10000 + geometry_type;
  return (enum GeoArrowType)type_int;
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
  memcpy(buffer->data.as_uint8 + buffer->size_bytes, value.data, value.size_bytes);
  buffer->size_bytes += value.size_bytes;
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
static inline void GeoArrowCoordViewCopy(const struct GeoArrowCoordView* src,
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

static inline int GeoArrowBuilderCoordsCheck(struct GeoArrowBuilder* builder,
                                             int64_t additional_size_coords) {
  return builder->view.coords.capacity_coords >=
         (builder->view.coords.size_coords + additional_size_coords);
}

static inline void GeoArrowBuilderCoordsAppendUnsafe(
    struct GeoArrowBuilder* builder, const struct GeoArrowCoordView* coords,
    enum GeoArrowDimensions dimensions, int64_t offset, int64_t n) {
  GeoArrowCoordViewCopy(coords, dimensions, offset, &builder->view.coords,
                        builder->view.schema_view.dimensions,
                        builder->view.coords.size_coords, n);
  builder->view.coords.size_coords += n;
}

static inline int GeoArrowBuilderOffsetCheck(struct GeoArrowBuilder* builder, int32_t i,
                                             int64_t additional_size_elements) {
  return (builder->view.buffers[i + 1].capacity_bytes / sizeof(int32_t)) >=
         ((builder->view.buffers[i + 1].size_bytes / sizeof(int32_t)) +
          additional_size_elements);
}

static inline void GeoArrowBuilderOffsetAppendUnsafe(struct GeoArrowBuilder* builder,
                                                     int32_t i, int32_t* data,
                                                     int64_t additional_size_elements) {
  struct GeoArrowWritableBufferView* buf = &builder->view.buffers[i + 1];
  memcpy(buf->data.as_uint8 + buf->size_bytes, data,
         additional_size_elements * sizeof(int32_t));
  buf->size_bytes += additional_size_elements * sizeof(int32_t);
}

struct _GeoArrowFindBufferResult {
  struct ArrowArray* array;
  int level;
  int64_t i;
};

static inline int64_t _GeoArrowArrayFindBuffer(struct ArrowArray* array,
                                               struct _GeoArrowFindBufferResult* res,
                                               int64_t i, int level, int skip_first) {
  int64_t total_buffers = (array->n_buffers - skip_first);
  if (i < total_buffers) {
    res->array = array;
    res->i = i + skip_first;
    res->level = level;
    return total_buffers;
  }

  i -= total_buffers;

  for (int64_t child_id = 0; child_id < array->n_children; child_id++) {
    int64_t child_buffers =
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
