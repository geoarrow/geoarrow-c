
#include <errno.h>

#include "nanoarrow.h"

#include "geoarrow.h"

// A early draft implementation used something like the Arrow C Data interface
// metadata specification instead of JSON. To help with the transition, this
// bit of code parses the original metadata format.
static GeoArrowErrorCode GeoArrowMetadataViewInitDeprecated(
    struct GeoArrowMetadataView* metadata_view, struct GeoArrowError* error) {
  const char* metadata = metadata_view->metadata.data;
  int32_t pos = 0;
  int32_t name_len;
  int32_t value_len;
  int32_t m;

  // TODO: check metadata + pos against the known size of the metadata string
  // to avoid buffer overrun

  memcpy(&m, metadata + pos, sizeof(int32_t));
  pos += sizeof(int32_t);

  for (int j = 0; j < m; j++) {
    memcpy(&name_len, metadata + pos, sizeof(int32_t));
    pos += sizeof(int32_t);

    const char* name = metadata + pos;
    pos += name_len;

    memcpy(&value_len, metadata + pos, sizeof(int32_t));
    pos += sizeof(int32_t);

    const char* value = metadata + pos;
    pos += value_len;

    if (name_len == 0 || value_len == 0) {
      continue;
    }

    if (name_len >= 3 && strncmp(name, "crs", 3) == 0) {
      metadata_view->crs.n_bytes = value_len;
      metadata_view->crs.data = value;
      metadata_view->crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
    } else if (name_len >= 4 && strncmp(name, "edges", 4) == 0) {
      if (value_len >= 9 && strncmp(value, "spherical", 9) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_SPHERICAL;
      } else {
        // unuspported value for 'edges' key
      }
    } else {
      // unsupported metadata key
    }
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowMetadataViewInitJSON(
    struct GeoArrowMetadataView* metadata_view, struct GeoArrowError* error) {
  ArrowErrorSet((struct ArrowError*)error, "JSON format not yet supported");
  return ENOTSUP;
}

GeoArrowErrorCode GeoArrowMetadataViewInit(struct GeoArrowMetadataView* metadata_view,
                                           struct GeoArrowStringView metadata,
                                           struct GeoArrowError* error) {
  metadata_view->metadata = metadata;
  metadata_view->edge_type = GEOARROW_EDGE_TYPE_PLANAR;
  metadata_view->crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
  metadata_view->crs.data = NULL;
  metadata_view->crs.n_bytes = 0;

  if (metadata.n_bytes == 0) {
    return GEOARROW_OK;
  }

  if (metadata.n_bytes >= 4 && metadata.data[0] != '{') {
    if (GeoArrowMetadataViewInitDeprecated(metadata_view, error) == GEOARROW_OK) {
      return GEOARROW_OK;
    }
  }

  return GeoArrowMetadataViewInitJSON(metadata_view, error);
}
