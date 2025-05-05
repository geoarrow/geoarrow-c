
#include <errno.h>
#include <stdint.h>

#include "geoarrow/geoarrow.h"

#include "nanoarrow/nanoarrow.h"

#define EWKB_Z_BIT 0x80000000
#define EWKB_M_BIT 0x40000000
#define EWKB_SRID_BIT 0x20000000

#ifndef GEOARROW_NATIVE_ENDIAN
#define GEOARROW_NATIVE_ENDIAN 0x01
#endif

#ifndef GEOARROW_BSWAP32
static inline uint32_t bswap_32(uint32_t x) {
  return (((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) |
          ((x & 0xFF000000) >> 24));
}
#define GEOARROW_BSWAP32(x) bswap_32(x)
#endif

struct WKBReaderPrivate {
  const uint8_t* data;
  int64_t size_bytes;
  const uint8_t* data0;
  int need_swapping;
  struct GeoArrowGeometry geom;
};

static inline int WKBReaderReadEndian(struct WKBReaderPrivate* s,
                                      struct GeoArrowError* error) {
  if (s->size_bytes > 0) {
    s->need_swapping = s->data[0] != GEOARROW_NATIVE_ENDIAN;
    s->data++;
    s->size_bytes--;
    return GEOARROW_OK;
  } else {
    GeoArrowErrorSet(error, "Expected endian byte but found end of buffer at byte %ld",
                     (long)(s->data - s->data0));
    return EINVAL;
  }
}

static inline int WKBReaderReadUInt32(struct WKBReaderPrivate* s, uint32_t* out,
                                      struct GeoArrowError* error) {
  if (s->size_bytes >= 4) {
    memcpy(out, s->data, sizeof(uint32_t));
    s->data += sizeof(uint32_t);
    s->size_bytes -= sizeof(uint32_t);
    if (s->need_swapping) {
      *out = GEOARROW_BSWAP32(*out);
    }
    return GEOARROW_OK;
  } else {
    GeoArrowErrorSet(error, "Expected uint32 but found end of buffer at byte %ld",
                     (long)(s->data - s->data0));
    return EINVAL;
  }
}

static inline GeoArrowErrorCode WKBReaderReadNodeCoordinates(
    struct WKBReaderPrivate* s, uint32_t n_coords, uint32_t coord_size_elements,
    struct GeoArrowGeometryNode* node, struct GeoArrowError* error) {
  int64_t bytes_needed = n_coords * coord_size_elements * sizeof(double);
  if (s->size_bytes < bytes_needed) {
    GeoArrowErrorSet(
        error,
        "Expected coordinate sequence of %ld coords (%ld bytes) but found %ld bytes "
        "remaining at byte %ld",
        (long)n_coords, (long)bytes_needed, (long)s->size_bytes,
        (long)(s->data - s->data0));
    return EINVAL;
  }

  if (n_coords > 0) {
    for (uint32_t i = 0; i < coord_size_elements; i++) {
      node->coord_stride[i] = (int32_t)coord_size_elements * sizeof(double);
      node->coords[i] = s->data + (i * sizeof(double));
    }
  }

  s->data += bytes_needed;
  return GEOARROW_OK;
}

static inline GeoArrowErrorCode WKBReaderReadNodeGeometry(
    struct WKBReaderPrivate* s, struct GeoArrowGeometryNode* node,
    struct GeoArrowError* error) {
  NANOARROW_RETURN_NOT_OK(WKBReaderReadEndian(s, error));
  uint32_t geometry_type;
  const uint8_t* data_at_geom_type = s->data;
  NANOARROW_RETURN_NOT_OK(WKBReaderReadUInt32(s, &geometry_type, error));

  int has_z = 0;
  int has_m = 0;

  // Handle EWKB high bits
  if (geometry_type & EWKB_Z_BIT) {
    has_z = 1;
  }

  if (geometry_type & EWKB_M_BIT) {
    has_m = 1;
  }

  if (geometry_type & EWKB_SRID_BIT) {
    // We ignore this because it's hard to work around if a user somehow
    // has embedded srid but still wants the data and doesn't have another way
    // to convert
    uint32_t embedded_srid;
    NANOARROW_RETURN_NOT_OK(WKBReaderReadUInt32(s, &embedded_srid, error));
  }

  geometry_type = geometry_type & 0x0000ffff;

  // Handle ISO X000 geometry types
  if (geometry_type >= 3000) {
    geometry_type = geometry_type - 3000;
    has_z = 1;
    has_m = 1;
  } else if (geometry_type >= 2000) {
    geometry_type = geometry_type - 2000;
    has_m = 1;
  } else if (geometry_type >= 1000) {
    geometry_type = geometry_type - 1000;
    has_z = 1;
  }

  // Read the number of coordinates/rings/parts
  uint32_t size;
  if (geometry_type != GEOARROW_GEOMETRY_TYPE_POINT) {
    NANOARROW_RETURN_NOT_OK(WKBReaderReadUInt32(s, &size, error));
  } else {
    size = 1;
  }

  // Set coord size
  uint32_t coord_size_elements = 2 + has_z + has_m;

  // Resolve dimensions
  enum GeoArrowDimensions dimensions;
  if (has_z && has_m) {
    dimensions = GEOARROW_DIMENSIONS_XYZM;
  } else if (has_z) {
    dimensions = GEOARROW_DIMENSIONS_XYZ;
  } else if (has_m) {
    dimensions = GEOARROW_DIMENSIONS_XYM;
  } else {
    dimensions = GEOARROW_DIMENSIONS_XY;
  }

  // Populate the node
  node->geometry_type = (uint8_t)geometry_type;
  node->dimensions = (uint8_t)dimensions;
  node->size = size;
  if (s->need_swapping) {
    node->flags = GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN;
  }

  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      NANOARROW_RETURN_NOT_OK(
          WKBReaderReadNodeCoordinates(s, size, coord_size_elements, node, error));
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      if (node->level == 255) {
        GeoArrowErrorSet(error, "WKBReader exceeded maximum recursion");
        return ENOTSUP;
      }

      struct GeoArrowGeometryNode ring_template = *node;
      ring_template.geometry_type = (uint8_t)GEOARROW_GEOMETRY_TYPE_LINESTRING;
      ring_template.level++;

      struct GeoArrowGeometryNode* ring;
      uint32_t ring_size;
      for (uint32_t i = 0; i < size; i++) {
        GEOARROW_RETURN_NOT_OK(WKBReaderReadUInt32(s, &ring_size, error));
        GEOARROW_RETURN_NOT_OK(GeoArrowGeometryAppendNodeInline(&s->geom, &ring));
        *ring = ring_template;
        ring->size = ring_size;

        GEOARROW_RETURN_NOT_OK(
            WKBReaderReadNodeCoordinates(s, ring_size, coord_size_elements, ring, error));
      }
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
    case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION:
      if (node->level == 255) {
        GeoArrowErrorSet(error, "WKBReader exceeded maximum recursion");
        return ENOTSUP;
      }

      uint8_t child_level = node->level + 1;
      struct GeoArrowGeometryNode* child;
      for (uint32_t i = 0; i < size; i++) {
        GEOARROW_RETURN_NOT_OK(GeoArrowGeometryAppendNodeInline(&s->geom, &child));
        child->level = child_level;
        GEOARROW_RETURN_NOT_OK(WKBReaderReadNodeGeometry(s, child, error));
      }
      break;
    default:
      GeoArrowErrorSet(error,
                       "Expected valid geometry type code but found %u at byte %ld",
                       (unsigned int)geometry_type, (long)(data_at_geom_type - s->data0));
      return EINVAL;
  }

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowWKBReaderInit(struct GeoArrowWKBReader* reader) {
  struct WKBReaderPrivate* s =
      (struct WKBReaderPrivate*)ArrowMalloc(sizeof(struct WKBReaderPrivate));

  if (s == NULL) {
    return ENOMEM;
  }

  s->data0 = NULL;
  s->data = NULL;
  s->size_bytes = 0;
  s->need_swapping = 0;

  GeoArrowErrorCode result = GeoArrowGeometryInit(&s->geom);
  if (result != GEOARROW_OK) {
    ArrowFree(s);
    return result;
  }

  reader->private_data = s;
  return GEOARROW_OK;
}

void GeoArrowWKBReaderReset(struct GeoArrowWKBReader* reader) {
  struct WKBReaderPrivate* s = (struct WKBReaderPrivate*)reader->private_data;
  GeoArrowGeometryReset(&s->geom);
  ArrowFree(reader->private_data);
}

GeoArrowErrorCode GeoArrowWKBReaderVisit(struct GeoArrowWKBReader* reader,
                                         struct GeoArrowBufferView src,
                                         struct GeoArrowVisitor* v) {
  struct GeoArrowGeometryView geometry;
  GEOARROW_RETURN_NOT_OK(GeoArrowWKBReaderRead(reader, src, &geometry, v->error));
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryViewVisit(geometry, v));
  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowWKBReaderRead(struct GeoArrowWKBReader* reader,
                                        struct GeoArrowBufferView src,
                                        struct GeoArrowGeometryView* out,
                                        struct GeoArrowError* error) {
  struct WKBReaderPrivate* s = (struct WKBReaderPrivate*)reader->private_data;
  s->data0 = src.data;
  s->data = src.data;
  s->size_bytes = src.size_bytes;

  // Reset the nodes list
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryResizeNodesInline(&s->geom, 0));

  // Make a root node at level 0
  struct GeoArrowGeometryNode* node;
  GEOARROW_RETURN_NOT_OK(GeoArrowGeometryAppendNodeInline(&s->geom, &node));
  node->level = 0;

  // Read
  GEOARROW_RETURN_NOT_OK(WKBReaderReadNodeGeometry(s, node, error));

  // Populate output on success
  *out = GeoArrowGeometryAsView(&s->geom);
  return GEOARROW_OK;
}
