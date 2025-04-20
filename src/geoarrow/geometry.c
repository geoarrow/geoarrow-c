
#include <errno.h>

#include "geoarrow/geoarrow.h"

#ifndef GEOARROW_BSWAP64
static inline uint64_t bswap_64(uint64_t x) {
  return (((x & 0xFFULL) << 56) | ((x & 0xFF00ULL) << 40) | ((x & 0xFF0000ULL) << 24) |
          ((x & 0xFF000000ULL) << 8) | ((x & 0xFF00000000ULL) >> 8) |
          ((x & 0xFF0000000000ULL) >> 24) | ((x & 0xFF000000000000ULL) >> 40) |
          ((x & 0xFF00000000000000ULL) >> 56));
}
#define GEOARROW_BSWAP64(x) bswap_64(x)
#endif

// This must be divisible by 2, 3, and 4
#define COORD_CACHE_SIZE_ELEMENTS 384

static inline void GeoArrowGeometryAlignCoords(const uint8_t** cursor,
                                               const int32_t* stride, double* coords,
                                               int n_values, uint32_t n_coords) {
  double* coords_cursor = coords;
  for (uint32_t i = 0; i < n_coords; i++) {
    for (int i = 0; i < n_values; i++) {
      memcpy(coords_cursor++, cursor[i], sizeof(double));
      cursor[i] += stride[i];
    }
  }
}

static inline void GeoArrowGeometryMaybeBswapCoords(struct GeoArrowGeometryNode* node,
                                                    double* values, int64_t n) {
  if (node->flags & GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN) {
    uint64_t* data64 = (uint64_t*)values;
    for (int i = 0; i < n; i++) {
      data64[i] = GEOARROW_BSWAP64(data64[i]);
    }
  }
}

static GeoArrowErrorCode GeoArrowGeometryVisitSequence(struct GeoArrowGeometryNode* node,
                                                       struct GeoArrowVisitor* v) {
  double coords[COORD_CACHE_SIZE_ELEMENTS];
  struct GeoArrowCoordView coord_view;
  switch (node->dimensions) {
    case GEOARROW_DIMENSIONS_XY:
      coord_view.n_values = 2;
      break;
    case GEOARROW_DIMENSIONS_XYZ:
    case GEOARROW_DIMENSIONS_XYM:
      coord_view.n_values = 3;
      break;
    case GEOARROW_DIMENSIONS_XYZM:
      coord_view.n_values = 4;
      break;
    default:
      GeoArrowErrorSet(v->error, "Invalid dimensions: %d", (int)node->dimensions);
      return EINVAL;
  }

  for (int i = 0; i < coord_view.n_values; i++) {
    coord_view.values[i] = coords + i;
    coord_view.coords_stride = coord_view.n_values;
  }

  int32_t chunk_size = COORD_CACHE_SIZE_ELEMENTS / coord_view.n_values;
  coord_view.n_coords = chunk_size;

  // Process full chunks
  int64_t n_coords = node->size;
  const uint8_t* cursor[4];
  memcpy(cursor, node->coords, sizeof(cursor));

  while (n_coords > chunk_size) {
    GeoArrowGeometryAlignCoords(cursor, node->coord_stride, coords, coord_view.n_values,
                                chunk_size);
    GeoArrowGeometryMaybeBswapCoords(node, coords, COORD_CACHE_SIZE_ELEMENTS);
    coord_view.n_coords = chunk_size;
    GEOARROW_RETURN_NOT_OK(v->coords(v, &coord_view));
    n_coords -= chunk_size;
  }

  GeoArrowGeometryAlignCoords(cursor, node->coord_stride, coords, coord_view.n_values,
                              (uint32_t)n_coords);
  GeoArrowGeometryMaybeBswapCoords(node, coords, COORD_CACHE_SIZE_ELEMENTS);
  coord_view.n_coords = n_coords;
  GEOARROW_RETURN_NOT_OK(v->coords(v, &coord_view));

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowGeometryVisit(struct GeoArrowGeometry geometry,
                                        struct GeoArrowVisitor* v) {
  GEOARROW_RETURN_NOT_OK(
      v->geom_start(v, (enum GeoArrowGeometryType)geometry.root->geometry_type,
                    (enum GeoArrowDimensions)geometry.root->dimensions));
  switch (geometry.root->geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisitSequence(geometry.root, v));
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      if (geometry.n_nodes < ((int64_t)geometry.root->size + 1)) {
        return EINVAL;
      }

      for (uint32_t i = 0; i < geometry.root->size; i++) {
        GEOARROW_RETURN_NOT_OK(v->ring_start(v));
        GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisitSequence(geometry.root + i + 1, v));
        GEOARROW_RETURN_NOT_OK(v->ring_end(v));
      }
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
    case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION: {
      if (geometry.n_nodes < ((int64_t)geometry.root->size + 1)) {
        return EINVAL;
      }

      struct GeoArrowGeometry child = geometry;
      for (uint32_t i = 0; i < geometry.root->size; i++) {
        child.root++;
        child.n_nodes--;
        GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisit(geometry, v));
      }
      break;
    }
    default:
      GeoArrowErrorSet(v->error, "Invalid geometry_type: %d",
                       (int)geometry.root->geometry_type);
      return EINVAL;
  }

  GEOARROW_RETURN_NOT_OK(v->geom_end(v));
  return GEOARROW_OK;
}
