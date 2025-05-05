
#include <errno.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

struct GeoArrowGeometryPrivate {
  // Note that the GeoArrowGeometry has cached data/size/capacity that needs
  // to be kept in sync
  struct ArrowBuffer nodes;
  struct ArrowBuffer coords;
  // For the builder interface
  int current_level;
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
  private_data->current_level = 0;
  geom->private_data = private_data;

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowGeometryShallowCopy(struct GeoArrowGeometryView src,
                                              struct GeoArrowGeometry* dst) {
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryResizeNodes(dst, src.size_nodes));
  if (src.size_nodes > 0) {
    memcpy(dst->root, src.root, src.size_nodes * sizeof(struct GeoArrowGeometryNode));
  }

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

GeoArrowErrorCode GeoArrowGeometryDeepCopy(struct GeoArrowGeometryView src,
                                           struct GeoArrowGeometry* dst) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)dst->private_data;

  // Calculate the number of coordinates required
  int64_t coords_required = 0;
  const struct GeoArrowGeometryNode* src_node = src.root;
  for (int64_t i = 0; i < src.size_nodes; i++) {
    switch (src_node->geometry_type) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
        coords_required += _GeoArrowkNumDimensions[src_node->dimensions] * src_node->size;
        break;
      default:
        break;
    }

    ++src_node;
  }

  // Resize the destination coords array
  GEOARROW_RETURN_NOT_OK(
      ArrowBufferResize(&private_data->coords, coords_required * sizeof(double), 0));

  // Copy the nodes
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryShallowCopy(src, dst));

  // Copy the data and update the nodes to point to the internal data
  double* coords = (double*)private_data->coords.data;
  struct GeoArrowGeometryNode* dst_node = dst->root;
  int n_values;
  for (int64_t i = 0; i < dst->size_nodes; i++) {
    switch (dst_node->geometry_type) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
        n_values = _GeoArrowkNumDimensions[dst_node->dimensions];
        GeoArrowGeometryAlignCoords(dst_node->coords, dst_node->coord_stride, coords,
                                    n_values, dst_node->size);
        for (int i = 0; i < 4; i++) {
          dst_node->coords[i] = (uint8_t*)(coords + i);
          dst_node->coord_stride[i] = n_values * sizeof(double);
        }
        coords += dst_node->size * n_values;
        break;
      default:
        break;
    }

    ++dst_node;
  }

  return GEOARROW_OK;
}

// We define a maximum nesting to simplify collecting sizes based on levels
#define GEOARROW_GEOMETRY_VISITOR_MAX_NESTING 31

static inline GeoArrowErrorCode GeoArrowGeometryReallocCoords(
    struct GeoArrowGeometry* geom, struct ArrowBuffer* new_coords,
    int64_t additional_size_bytes) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;

  int64_t new_size_bytes;
  if ((private_data->coords.size_bytes + additional_size_bytes) >
      private_data->coords.size_bytes * 2) {
    new_size_bytes = private_data->coords.size_bytes + additional_size_bytes;
  } else {
    new_size_bytes = private_data->coords.size_bytes * 2;
  }

  GEOARROW_RETURN_NOT_OK(ArrowBufferReserve(new_coords, new_size_bytes));
  GEOARROW_RETURN_NOT_OK(ArrowBufferAppend(new_coords, private_data->coords.data,
                                           private_data->coords.size_bytes));
  struct GeoArrowGeometryNode* node = geom->root;
  for (int64_t i = 0; i < geom->size_nodes; i++) {
    for (int i = 0; i < 4; i++) {
      if (node->coords[i] == _GeoArrowkEmptyPointCoords) {
        continue;
      }

      ptrdiff_t offset = node->coords[i] - private_data->coords.data;
      node->coords[i] = new_coords->data + offset;
    }

    ++node;
  }

  return GEOARROW_OK;
}

static inline GeoArrowErrorCode GeoArrowGeometryReserveCoords(
    struct GeoArrowGeometry* geom, int64_t additional_doubles, double** out) {
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;

  int64_t bytes_required =
      private_data->coords.size_bytes + (additional_doubles * sizeof(double));
  if (bytes_required > private_data->coords.capacity_bytes) {
    struct ArrowBuffer new_coords;
    ArrowBufferInit(&new_coords);
    GeoArrowErrorCode result = GeoArrowGeometryReallocCoords(
        geom, &new_coords, (additional_doubles * sizeof(double)));
    if (result != GEOARROW_OK) {
      ArrowBufferReset(&new_coords);
      return result;
    }

    ArrowBufferReset(&private_data->coords);
    ArrowBufferMove(&new_coords, &private_data->coords);
  }

  *out = (double*)(private_data->coords.data + private_data->coords.size_bytes);
  return GEOARROW_OK;
}

static int feat_start_geometry(struct GeoArrowVisitor* v) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;

  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryResizeNodes(geom, 0));
  GEOARROW_RETURN_NOT_OK(ArrowBufferResize(&private_data->coords, 0, 0));
  private_data->current_level = 0;
  return GEOARROW_OK;
}

static int null_feat_geometry(struct GeoArrowVisitor* v) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  struct GeoArrowGeometryNode* node;
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryAppendNodeInline(geom, &node));
  return GEOARROW_OK;
}

static int geom_start_geometry(struct GeoArrowVisitor* v,
                               enum GeoArrowGeometryType geometry_type,
                               enum GeoArrowDimensions dimensions) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;
  struct GeoArrowGeometryNode* node;
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryAppendNodeInline(geom, &node));
  node->geometry_type = (uint8_t)geometry_type;
  node->dimensions = (uint8_t)dimensions;
  node->level = (uint8_t)private_data->current_level;
  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING: {
      // Reserve coords and set pointers here, even when we're not sure if
      // there will be any coordinates. This lets us calculate sequences sizes
      // after the fact because we can always use the pointer differences between
      // the start of the x value in one sequence and the start of the x value
      // in the next sequence to calculate sequence size.
      int n_values = _GeoArrowkNumDimensions[dimensions];
      double* coords_start;
      GEOARROW_RETURN_NOT_OK(
          GeoArrowGeometryReserveCoords(geom, n_values, &coords_start));
      for (int i = 0; i < n_values; i++) {
        node->coords[i] = (uint8_t*)(coords_start + i);
        node->coord_stride[i] = n_values * sizeof(double);
      }
      break;
    }
    default:
      break;
  }

  if (private_data->current_level == GEOARROW_GEOMETRY_VISITOR_MAX_NESTING) {
    GeoArrowErrorSet(v->error, "Maximum recursion for GeoArrowGeometry visitor reached");
    return EINVAL;
  }

  private_data->current_level++;
  return GEOARROW_OK;
}

static int ring_start_geometry(struct GeoArrowVisitor* v) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  if (geom->size_nodes == 0) {
    GeoArrowErrorSet(v->error,
                     "Call to ring_start before geom_start in GeoArrowGeometry visitor");
    return EINVAL;
  }

  struct GeoArrowGeometryNode* last_node = geom->root + geom->size_nodes - 1;
  return geom_start_geometry(v, GEOARROW_GEOMETRY_TYPE_LINESTRING, last_node->dimensions);
}

static int coords_geometry(struct GeoArrowVisitor* v,
                           const struct GeoArrowCoordView* coords) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;

  if (geom->size_nodes == 0) {
    GeoArrowErrorSet(v->error,
                     "Call to coords before geom_start in GeoArrowGeometry visitor");
    return EINVAL;
  }

  double* values;
  GEOARROW_RETURN_NOT_OK(
      GeoArrowGeometryReserveCoords(geom, coords->n_coords * coords->n_values, &values));

  for (int64_t i = 0; i < coords->n_coords; i++) {
    for (int j = 0; j < coords->n_values; j++) {
      *values++ = GEOARROW_COORD_VIEW_VALUE(coords, i, j);
    }
  }

  private_data->coords.size_bytes += coords->n_coords * coords->n_values * sizeof(double);
  return GEOARROW_OK;
}

static int ring_end_geometry(struct GeoArrowVisitor* v) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;

  if (private_data->current_level == 0) {
    GeoArrowErrorSet(v->error,
                     "Incorrect nesting in GeoArrowGeometry visitor (level < 0)");
    return EINVAL;
  }

  private_data->current_level--;
  return GEOARROW_OK;
}

static int geom_end_geometry(struct GeoArrowVisitor* v) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;

  if (private_data->current_level == 0) {
    GeoArrowErrorSet(v->error,
                     "Incorrect nesting in GeoArrowGeometry visitor (level < 0)");
    return EINVAL;
  }

  private_data->current_level--;
  return GEOARROW_OK;
}

static int feat_end_geometry(struct GeoArrowVisitor* v) {
  struct GeoArrowGeometry* geom = (struct GeoArrowGeometry*)v->private_data;
  struct GeoArrowGeometryPrivate* private_data =
      (struct GeoArrowGeometryPrivate*)geom->private_data;

  if (geom->size_nodes == 0) {
    GeoArrowErrorSet(v->error,
                     "Call to feat_end before geom_start in GeoArrowGeometry visitor");
    return EINVAL;
  }

  if (private_data->coords.size_bytes == 0) {
    // Can happen for empty collections where there was never a sequence
    return GEOARROW_OK;
  }

  // Set sequence lengths by rolling backwards through the sequences. We recorded
  // the start position for each sequence in geom_start, so we can calculate the
  // size based on the distance between the pointers.
  const uint8_t* end = private_data->coords.data + private_data->coords.size_bytes;
  struct GeoArrowGeometryNode* node_begin = geom->root;
  struct GeoArrowGeometryNode* node_end = geom->root + geom->size_nodes;

  uint32_t sizes[GEOARROW_GEOMETRY_VISITOR_MAX_NESTING + 1];
  memset(sizes, 0, sizeof(sizes));
  ptrdiff_t sequence_bytes;
  for (struct GeoArrowGeometryNode* node = (node_end - 1); node >= node_begin; node--) {
    sizes[node->level]++;

    switch (node->geometry_type) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
        sequence_bytes = end - node->coords[0];
        node->size = (uint32_t)(sequence_bytes / node->coord_stride[0]);
        end = node->coords[0];
        break;
      case GEOARROW_GEOMETRY_TYPE_POLYGON:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION:
        node->size = sizes[node->level + 1];
        sizes[node->level + 1] = 0;
        break;
      default:
        break;
    }
  }

  return GEOARROW_OK;
}

void GeoArrowGeometryInitVisitor(struct GeoArrowGeometry* geom,
                                 struct GeoArrowVisitor* v) {
  v->feat_start = &feat_start_geometry;
  v->null_feat = &null_feat_geometry;
  v->geom_start = &geom_start_geometry;
  v->ring_start = &ring_start_geometry;
  v->coords = &coords_geometry;
  v->ring_end = &ring_end_geometry;
  v->geom_end = &geom_end_geometry;
  v->feat_end = &feat_end_geometry;
  v->private_data = geom;
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

GeoArrowErrorCode GeoArrowGeometryVisit(const struct GeoArrowGeometry* geometry,
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
