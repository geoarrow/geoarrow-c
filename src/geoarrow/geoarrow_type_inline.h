
#ifndef GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED
#define GEOARROW_GEOARROW_TYPES_INLINE_H_INCLUDED

#include <stddef.h>
#include <string.h>

#include "geoarrow/geoarrow_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Extract GeometryType from a GeoArrowType
/// \ingroup geoarrow-schema
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

  int geometry_type = (int)type;
  if (geometry_type >= GEOARROW_TYPE_INTERLEAVED_POINT) {
    geometry_type -= 10000;
  }

  geometry_type = (int)geometry_type % 1000;
  if (geometry_type == GEOARROW_GEOMETRY_TYPE_BOX) {
    return GEOARROW_GEOMETRY_TYPE_BOX;
  } else if (geometry_type <= 6 && geometry_type >= 1) {
    return (enum GeoArrowGeometryType)geometry_type;
  } else {
    return GEOARROW_GEOMETRY_TYPE_GEOMETRY;
  }
}

/// \brief Returns the Arrow extension name for a given GeoArrowType
/// \ingroup geoarrow-schema
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
    case GEOARROW_GEOMETRY_TYPE_BOX:
      return "geoarrow.box";
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

/// \brief Returns a string representation of a GeoArrowDimensions
/// \ingroup geoarrow-schema
static inline const char* GeoArrowDimensionsString(enum GeoArrowDimensions dimensions) {
  switch (dimensions) {
    case GEOARROW_DIMENSIONS_UNKNOWN:
      return "unknown";
    case GEOARROW_DIMENSIONS_XY:
      return "xy";
    case GEOARROW_DIMENSIONS_XYZ:
      return "xyz";
    case GEOARROW_DIMENSIONS_XYM:
      return "xym";
    case GEOARROW_DIMENSIONS_XYZM:
      return "xyzm";
    default:
      return "<not valid>";
  }
}

/// \brief Returns a string representation of a GeoArrowCoordType
/// \ingroup geoarrow-schema
static inline const char* GeoArrowCoordTypeString(enum GeoArrowCoordType dimensions) {
  switch (dimensions) {
    case GEOARROW_COORD_TYPE_UNKNOWN:
      return "unknown";
    case GEOARROW_COORD_TYPE_SEPARATE:
      return "separate";
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      return "interleaved";
    default:
      return "<not valid>";
  }
}

/// \brief Returns a string representation of a GeoArrowEdgeType
/// \ingroup geoarrow-schema
static inline const char* GeoArrowEdgeTypeString(enum GeoArrowEdgeType edge_type) {
  switch (edge_type) {
    case GEOARROW_EDGE_TYPE_PLANAR:
      return "planar";
    case GEOARROW_EDGE_TYPE_SPHERICAL:
      return "spherical";
    case GEOARROW_EDGE_TYPE_VINCENTY:
      return "vincenty";
    case GEOARROW_EDGE_TYPE_THOMAS:
      return "thomas";
    case GEOARROW_EDGE_TYPE_ANDOYER:
      return "andoyer";
    case GEOARROW_EDGE_TYPE_KARNEY:
      return "karney";
    default:
      return "<not valid>";
  }
}

/// \brief Returns a string representation of a GeoArrowCrsType
/// \ingroup geoarrow-schema
static inline const char* GeoArrowCrsTypeString(enum GeoArrowCrsType crs_type) {
  switch (crs_type) {
    case GEOARROW_CRS_TYPE_NONE:
      return "none";
    case GEOARROW_CRS_TYPE_UNKNOWN:
      return "unknown";
    case GEOARROW_CRS_TYPE_PROJJSON:
      return "projjson";
    case GEOARROW_CRS_TYPE_WKT2_2019:
      return "wkt2:2019";
    case GEOARROW_CRS_TYPE_AUTHORITY_CODE:
      return "authority_code";
    case GEOARROW_CRS_TYPE_SRID:
      return "srid";
    default:
      return "<not valid>";
  }
}

/// \brief Extract GeoArrowDimensions from a GeoArrowType
/// \ingroup geoarrow-schema
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

  int type_int = (int)type;
  if (type_int >= GEOARROW_TYPE_INTERLEAVED_POINT) {
    type_int -= 10000;
  }

  switch (type_int / 1000) {
    case 0:
      return GEOARROW_DIMENSIONS_XY;
    case 1:
      return GEOARROW_DIMENSIONS_XYZ;
    case 2:
      return GEOARROW_DIMENSIONS_XYM;
    case 3:
      return GEOARROW_DIMENSIONS_XYZM;
    default:
      return GEOARROW_DIMENSIONS_UNKNOWN;
  }
}

/// \brief Extract GeoArrowCoordType from a GeoArrowType
/// \ingroup geoarrow-schema
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

/// \brief Construct a GeometryType from a GeoArrowGeometryType, GeoArrowDimensions,
/// and GeoArrowCoordType.
/// \ingroup geoarrow-schema
static inline enum GeoArrowType GeoArrowMakeType(enum GeoArrowGeometryType geometry_type,
                                                 enum GeoArrowDimensions dimensions,
                                                 enum GeoArrowCoordType coord_type) {
  if (geometry_type == GEOARROW_GEOMETRY_TYPE_GEOMETRY) {
    return GEOARROW_TYPE_UNINITIALIZED;
  } else if (dimensions == GEOARROW_DIMENSIONS_UNKNOWN) {
    return GEOARROW_TYPE_UNINITIALIZED;
  } else if (coord_type == GEOARROW_COORD_TYPE_UNKNOWN) {
    return GEOARROW_TYPE_UNINITIALIZED;
  } else if (geometry_type == GEOARROW_GEOMETRY_TYPE_BOX &&
             coord_type != GEOARROW_COORD_TYPE_SEPARATE) {
    return GEOARROW_TYPE_UNINITIALIZED;
  }

  int type_int = (dimensions - 1) * 1000 + (coord_type - 1) * 10000 + geometry_type;
  return (enum GeoArrowType)type_int;
}

/// \brief The all-caps string associated with a given GeometryType (e.g., POINT)
/// \ingroup geoarrow-schema
static inline const char* GeoArrowGeometryTypeString(
    enum GeoArrowGeometryType geometry_type) {
  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_GEOMETRY:
      return "GEOMETRY";
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
    case GEOARROW_GEOMETRY_TYPE_BOX:
      return "BOX";
    default:
      return "<not valid>";
  }
}

// Such that kNumOffsets[geometry_type] gives the right answer
static const int _GeoArrowkNumOffsets[] = {-1, 0, 1, 2, 1, 2, 3, -1};

// Such that kNumDimensions[dimensions] gives the right answer
static const int _GeoArrowkNumDimensions[] = {-1, 2, 3, 3, 4};

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
#if defined(GEOARROW_NATIVE_ENDIAN) && GEOARROW_NATIVE_ENDIAN == 0x00
static uint8_t _GeoArrowkEmptyPointCoords[] = {
    0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else
static uint8_t _GeoArrowkEmptyPointCoords[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xf8, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf8, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};
#endif

/// \brief View a GeoArrowGeometry
/// \ingroup geoarrow-geometry
///
/// The geometry_type must be a POINT or LINESTRING.
static inline struct GeoArrowGeometryView GeoArrowGeometryAsView(
    const struct GeoArrowGeometry* geom) {
  struct GeoArrowGeometryView out;
  out.root = geom->root;
  out.size_nodes = geom->size_nodes;
  return out;
}

/// \brief Set a node where coordinates are stored in a row-major (C) array
/// \ingroup geoarrow-geometry
///
/// The geometry_type must be a POINT or LINESTRING.
static inline void GeoArrowGeometryNodeSetInterleaved(
    struct GeoArrowGeometryNode* node, enum GeoArrowGeometryType geometry_type,
    enum GeoArrowDimensions dimensions, struct GeoArrowBufferView coords) {
  node->geometry_type = (uint8_t)geometry_type;
  node->dimensions = (uint8_t)dimensions;

  int32_t coord_stride_bytes = _GeoArrowkNumDimensions[dimensions] * sizeof(double);
  node->size = (uint32_t)(coords.size_bytes / coord_stride_bytes);
  for (int i = 0; i < 4; i++) {
    node->coord_stride[i] = coord_stride_bytes;
    node->coords[i] = coords.data + (i * sizeof(double));
  }
}

/// \brief Set a node where coordinates are stored in a column-major (Fortran) array
/// \ingroup geoarrow-geometry
///
/// The geometry_type must be a POINT or LINESTRING.
static inline void GeoArrowGeometryNodeSetSeparated(
    struct GeoArrowGeometryNode* node, enum GeoArrowGeometryType geometry_type,
    enum GeoArrowDimensions dimensions, struct GeoArrowBufferView coords) {
  node->geometry_type = (uint8_t)geometry_type;
  node->dimensions = (uint8_t)dimensions;

  int64_t dimension_size_bytes = coords.size_bytes / _GeoArrowkNumDimensions[dimensions];
  node->size = (uint32_t)(dimension_size_bytes / sizeof(double));
  for (int i = 0; i < 4; i++) {
    node->coord_stride[i] = sizeof(double);
    node->coords[i] = coords.data + (i * dimension_size_bytes);
  }
}

/// \brief Inline version of GeoArrowGeometryResizeNodes
/// \ingroup geoarrow-geometry
static inline GeoArrowErrorCode GeoArrowGeometryResizeNodesInline(
    struct GeoArrowGeometry* geom, int64_t size_nodes) {
  if (size_nodes < geom->capacity_nodes) {
    geom->size_nodes = size_nodes;
    return GEOARROW_OK;
  } else {
    return GeoArrowGeometryResizeNodes(geom, size_nodes);
  }
}

/// \brief Inline version of GeoArrowGeometryAppendNode
/// \ingroup geoarrow-geometry
static inline GeoArrowErrorCode GeoArrowGeometryAppendNodeInline(
    struct GeoArrowGeometry* geom, struct GeoArrowGeometryNode** out) {
  if (geom->size_nodes < geom->capacity_nodes) {
    *out = geom->root + (geom->size_nodes++);
    memset(*out, 0, sizeof(struct GeoArrowGeometryNode));
    for (uint32_t i = 0; i < 4; i++) {
      (*out)->coords[i] = _GeoArrowkEmptyPointCoords;
    }
    return GEOARROW_OK;
  } else {
    return GeoArrowGeometryAppendNode(geom, out);
  }
}

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
                                                     int32_t i, const int32_t* data,
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

static inline GeoArrowErrorCode GeoArrowBuilderAppendBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value) {
  if (!GeoArrowBuilderBufferCheck(builder, i, value.size_bytes)) {
    int result = GeoArrowBuilderReserveBuffer(builder, i, value.size_bytes);
    if (result != GEOARROW_OK) {
      return result;
    }
  }

  GeoArrowBuilderAppendBufferUnsafe(builder, i, value);
  return GEOARROW_OK;
}

static inline GeoArrowErrorCode GeoArrowBuilderCoordsReserve(
    struct GeoArrowBuilder* builder, int64_t additional_size_coords) {
  if (GeoArrowBuilderCoordsCheck(builder, additional_size_coords)) {
    return GEOARROW_OK;
  }

  struct GeoArrowWritableCoordView* writable_view = &builder->view.coords;
  int result;
  int64_t last_buffer = builder->view.n_buffers - 1;
  int n_values = writable_view->n_values;

  switch (builder->view.schema_view.coord_type) {
    case GEOARROW_COORD_TYPE_INTERLEAVED:
      // Sync the coord view size back to the buffer size
      builder->view.buffers[last_buffer].size_bytes =
          writable_view->size_coords * sizeof(double) * n_values;

      // Use the normal reserve
      result = GeoArrowBuilderReserveBuffer(
          builder, last_buffer, additional_size_coords * sizeof(double) * n_values);
      if (result != GEOARROW_OK) {
        return result;
      }

      // Sync the capacity and pointers back to the writable view
      writable_view->capacity_coords =
          builder->view.buffers[last_buffer].capacity_bytes / sizeof(double) / n_values;
      for (int i = 0; i < n_values; i++) {
        writable_view->values[i] = builder->view.buffers[last_buffer].data.as_double + i;
      }

      return GEOARROW_OK;

    case GEOARROW_COORD_TYPE_SEPARATE:
      for (int64_t i = last_buffer - n_values + 1; i <= last_buffer; i++) {
        // Sync the coord view size back to the buffer size
        builder->view.buffers[i].size_bytes = writable_view->size_coords * sizeof(double);

        // Use the normal reserve
        result = GeoArrowBuilderReserveBuffer(builder, i,
                                              additional_size_coords * sizeof(double));
        if (result != GEOARROW_OK) {
          return result;
        }
      }

      // Sync the capacity and pointers back to the writable view
      writable_view->capacity_coords =
          builder->view.buffers[last_buffer].capacity_bytes / sizeof(double);
      for (int i = 0; i < n_values; i++) {
        writable_view->values[i] =
            builder->view.buffers[last_buffer - n_values + 1 + i].data.as_double;
      }

      return GEOARROW_OK;
    default:
      // Because there is no include <errno.h> here yet
      return -1;
  }
}

static inline GeoArrowErrorCode GeoArrowBuilderCoordsAppend(
    struct GeoArrowBuilder* builder, const struct GeoArrowCoordView* coords,
    enum GeoArrowDimensions dimensions, int64_t offset, int64_t n) {
  if (!GeoArrowBuilderCoordsCheck(builder, n)) {
    int result = GeoArrowBuilderCoordsReserve(builder, n);
    if (result != GEOARROW_OK) {
      return result;
    }
  }

  GeoArrowBuilderCoordsAppendUnsafe(builder, coords, dimensions, offset, n);
  return GEOARROW_OK;
}

static inline GeoArrowErrorCode GeoArrowBuilderOffsetReserve(
    struct GeoArrowBuilder* builder, int32_t i, int64_t additional_size_elements) {
  if (GeoArrowBuilderOffsetCheck(builder, i, additional_size_elements)) {
    return GEOARROW_OK;
  }

  return GeoArrowBuilderReserveBuffer(builder, i + 1,
                                      additional_size_elements * sizeof(int32_t));
}

static inline GeoArrowErrorCode GeoArrowBuilderOffsetAppend(
    struct GeoArrowBuilder* builder, int32_t i, const int32_t* data,
    int64_t additional_size_elements) {
  if (!GeoArrowBuilderOffsetCheck(builder, i, additional_size_elements)) {
    int result = GeoArrowBuilderOffsetReserve(builder, i, additional_size_elements);
    if (result != GEOARROW_OK) {
      return result;
    }
  }

  GeoArrowBuilderOffsetAppendUnsafe(builder, i, data, additional_size_elements);
  return GEOARROW_OK;
}

#ifdef __cplusplus
}
#endif

#endif
