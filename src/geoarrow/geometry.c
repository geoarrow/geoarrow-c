
#include <errno.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

struct GeoArrowGeometryPrivate {
  // Note that the GeoArrowGeometry has cached data/size/capacity that needs
  // to be kept in sync
  struct ArrowBuffer nodes;
  struct ArrowBuffer coords;
};

GeoArrowErrorCode GeoArrowGeometryInit(struct GeoArrowGeometry* geom) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)ArrowMalloc(
          sizeof(struct GeoArrowGeometryPrivate));
  if (private_data == NULL) {
    return ENOMEM;
  }

  geom->root = NULL;
  geom->size_nodes = 0;
  geom->capacity_nodes = 0;
  ArrowBufferInit(&private_data->nodes);
  ArrowBufferInit(&private_data->coords);
  geom->private_data = private_data;

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowGeometryShallowCopy(struct GeoArrowGeometryView src,
                                              struct GeoArrowGeometry* dst) {
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryResizeNodes(dst, src.size_nodes));
  memcpy(dst->root, src.root, src.size_nodes * sizeof(struct GeoArrowGeometryNode));
  return GEOARROW_OK;
}

void GeoArrowGeometryReset(struct GeoArrowGeometry* geom) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;
  ArrowBufferReset(&private_data->coords);
  ArrowBufferReset(&private_data->nodes);
  ArrowFree(geom->private_data);
  geom->private_data = NULL;
}

static inline void GeoArrowGeometrySyncGeomToNodes(struct GeoArrowGeometry* geom) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;
  // Inline methods should never modify the capacity or data pointer
  NANOARROW_DCHECK(private_data->nodes.capacity_bytes ==
                   (geom->capacity_nodes * (int64_t)sizeof(struct GeoArrowGeometryNode)));
  NANOARROW_DCHECK(private_data->nodes.data == (uint8_t*)geom->root);

  // But may have updated the size
  private_data->nodes.size_bytes = geom->size_nodes * sizeof(struct GeoArrowGeometryNode);
}

static inline void GeoArrowGeometrySyncNodesToGeom(struct GeoArrowGeometry* geom) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;
  geom->root = (struct GeoArrowGeometryNode*)private_data->nodes.data;
  geom->size_nodes = private_data->nodes.size_bytes / sizeof(struct GeoArrowGeometryNode);
  geom->capacity_nodes =
      private_data->nodes.capacity_bytes / sizeof(struct GeoArrowGeometryNode);
}

GeoArrowErrorCode GeoArrowGeometryResizeNodes(struct GeoArrowGeometry* geom,
                                              int64_t size_nodes) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;
  GEOARROW_RETURN_NOT_OK(ArrowBufferResize(
      &private_data->nodes, size_nodes * sizeof(struct GeoArrowGeometryNode), 0));
  GeoArrowGeometrySyncNodesToGeom(geom);
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowGeometryAppendNode(struct GeoArrowGeometry* geom,
                                             struct GeoArrowGeometryNode** out) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;
  GeoArrowGeometrySyncGeomToNodes(geom);
  GEOARROW_RETURN_NOT_OK(
      ArrowBufferReserve(&private_data->nodes, sizeof(struct GeoArrowGeometryNode)));
  GeoArrowGeometrySyncNodesToGeom(geom);
  return GeoArrowGeometryAppendNodeInline(geom, out);
}

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

static inline void GeoArrowGeometryMaybeBswapCoords(
    const struct GeoArrowGeometryNode* node, double* values, int64_t n) {
  if (node->flags & GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN) {
    uint64_t* data64 = (uint64_t*)values;
    for (int i = 0; i < n; i++) {
      data64[i] = GEOARROW_BSWAP64(data64[i]);
    }
  }
}

static GeoArrowErrorCode GeoArrowGeometryVisitSequence(
    const struct GeoArrowGeometryNode* node, struct GeoArrowVisitor* v) {
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

static GeoArrowErrorCode GeoArrowGeometryVisitNode(
    const struct GeoArrowGeometryNode* node, int64_t* n_nodes,
    struct GeoArrowVisitor* v) {
  if ((*n_nodes)-- <= 0) {
    GeoArrowErrorSet(v->error, "Too few nodes provided to GeoArrowGeometryVisit()");
  }

  GEOARROW_RETURN_NOT_OK(v->geom_start(v, (enum GeoArrowGeometryType)node->geometry_type,
                                       (enum GeoArrowDimensions)node->dimensions));
  switch (node->geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisitSequence(node, v));
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      if (*n_nodes < ((int64_t)node->size)) {
        return EINVAL;
      }

      for (uint32_t i = 0; i < node->size; i++) {
        GEOARROW_RETURN_NOT_OK(v->ring_start(v));
        GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisitSequence(node + i + 1, v));
        GEOARROW_RETURN_NOT_OK(v->ring_end(v));
      }

      (*n_nodes) -= node->size;
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
    case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION: {
      const struct GeoArrowGeometryNode* child = node + 1;
      for (uint32_t i = 0; i < node->size; i++) {
        int64_t n_nodes_before_child = *n_nodes;
        GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisitNode(child, n_nodes, v));
        int64_t nodes_consumed = n_nodes_before_child - *n_nodes;
        child += nodes_consumed;
      }
      break;
    }
    default:
      GeoArrowErrorSet(v->error, "Invalid geometry_type: %d", (int)node->geometry_type);
      return EINVAL;
  }

  GEOARROW_RETURN_NOT_OK(v->geom_end(v));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowGeometryViewVisit(struct GeoArrowGeometryView geometry,
                                            struct GeoArrowVisitor* v) {
  int64_t n_nodes = geometry.size_nodes;
  GEOARROW_RETURN_NOT_OK(v->feat_start(v));
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisitNode(geometry.root, &n_nodes, v));
  if (n_nodes != 0) {
    GeoArrowErrorSet(
        v->error, "Too many nodes provided to GeoArrowGeometryVisit() for root geometry");
    return EINVAL;
  }

  GEOARROW_RETURN_NOT_OK(v->feat_end(v));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowGeometryVisit(struct GeoArrowGeometry* geometry,
                                        struct GeoArrowVisitor* v) {
  int64_t n_nodes = geometry->size_nodes;
  GEOARROW_RETURN_NOT_OK(v->feat_start(v));
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryVisitNode(geometry->root, &n_nodes, v));
  if (n_nodes != 0) {
    GeoArrowErrorSet(
        v->error, "Too many nodes provided to GeoArrowGeometryVisit() for root geometry");
    return EINVAL;
  }

  GEOARROW_RETURN_NOT_OK(v->feat_end(v));
  return GEOARROW_OK;
}
