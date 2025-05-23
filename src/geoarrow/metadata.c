
#include <errno.h>
#include <stdio.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

#define CHECK_POS(n)                               \
  if ((pos + (int32_t)(n)) > ((int32_t)pos_max)) { \
    return EINVAL;                                 \
  }

static int ParseChar(struct ArrowStringView* s, char c) {
  if (s->size_bytes > 0 && s->data[0] == c) {
    s->size_bytes--;
    s->data++;
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static void SkipWhitespace(struct ArrowStringView* s) {
  while (s->size_bytes > 0) {
    char c = *(s->data);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      s->size_bytes--;
      s->data++;
    } else {
      break;
    }
  }
}

static int SkipUntil(struct ArrowStringView* s, const char* items) {
  int64_t n_items = strlen(items);
  while (s->size_bytes > 0) {
    char c = *(s->data);
    if (c == '\0') {
      return 0;
    }

    for (int64_t i = 0; i < n_items; i++) {
      if (c == items[i]) {
        return 1;
      }
    }

    s->size_bytes--;
    s->data++;
  }

  return 0;
}

static GeoArrowErrorCode FindNull(struct ArrowStringView* s,
                                  struct ArrowStringView* out) {
  if (s->size_bytes < 4) {
    return EINVAL;
  }

  if (strncmp(s->data, "null", 4) != 0) {
    return EINVAL;
  }

  out->data = s->data;
  out->size_bytes = 4;
  s->size_bytes -= 4;
  s->data += 4;
  return GEOARROW_OK;
}

static GeoArrowErrorCode FindString(struct ArrowStringView* s,
                                    struct ArrowStringView* out) {
  out->data = s->data;
  if (s->data[0] != '\"') {
    return EINVAL;
  }

  s->size_bytes--;
  s->data++;

  int is_escape = 0;
  while (s->size_bytes > 0) {
    char c = *(s->data);
    if (!is_escape && c == '\\') {
      is_escape = 1;
      s->size_bytes--;
      s->data++;
      continue;
    }

    if (!is_escape && c == '\"') {
      s->size_bytes--;
      s->data++;
      out->size_bytes = s->data - out->data;
      return GEOARROW_OK;
    }

    s->size_bytes--;
    s->data++;
    is_escape = 0;
  }

  return EINVAL;
}

static GeoArrowErrorCode FindObject(struct ArrowStringView* s,
                                    struct ArrowStringView* out);

static GeoArrowErrorCode FindList(struct ArrowStringView* s,
                                  struct ArrowStringView* out) {
  out->data = s->data;
  if (s->data[0] != '[') {
    return EINVAL;
  }

  s->size_bytes--;
  s->data++;
  struct ArrowStringView tmp_value;
  while (s->size_bytes > 0) {
    if (SkipUntil(s, "[{\"]")) {
      char c = *(s->data);
      switch (c) {
        case '\"':
          NANOARROW_RETURN_NOT_OK(FindString(s, &tmp_value));
          break;
        case '[':
          NANOARROW_RETURN_NOT_OK(FindList(s, &tmp_value));
          break;
        case '{':
          NANOARROW_RETURN_NOT_OK(FindObject(s, &tmp_value));
          break;
        case ']':
          s->size_bytes--;
          s->data++;
          out->size_bytes = s->data - out->data;
          return GEOARROW_OK;
        default:
          break;
      }
    }
  }

  return EINVAL;
}

static GeoArrowErrorCode FindObject(struct ArrowStringView* s,
                                    struct ArrowStringView* out) {
  out->data = s->data;
  if (s->data[0] != '{') {
    return EINVAL;
  }

  s->size_bytes--;
  s->data++;
  struct ArrowStringView tmp_value;
  while (s->size_bytes > 0) {
    if (SkipUntil(s, "{[\"}")) {
      char c = *(s->data);
      switch (c) {
        case '\"':
          NANOARROW_RETURN_NOT_OK(FindString(s, &tmp_value));
          break;
        case '[':
          NANOARROW_RETURN_NOT_OK(FindList(s, &tmp_value));
          break;
        case '{':
          NANOARROW_RETURN_NOT_OK(FindObject(s, &tmp_value));
          break;
        case '}':
          s->size_bytes--;
          s->data++;
          out->size_bytes = s->data - out->data;
          return GEOARROW_OK;
        default:
          break;
      }
    }
  }

  return EINVAL;
}

static GeoArrowErrorCode ParseJSONMetadata(struct GeoArrowMetadataView* metadata_view,
                                           struct ArrowStringView* s) {
  NANOARROW_RETURN_NOT_OK(ParseChar(s, '{'));
  SkipWhitespace(s);
  struct ArrowStringView k;
  struct ArrowStringView v;

  while (s->size_bytes > 0 && s->data[0] != '}') {
    SkipWhitespace(s);
    NANOARROW_RETURN_NOT_OK(FindString(s, &k));
    SkipWhitespace(s);
    NANOARROW_RETURN_NOT_OK(ParseChar(s, ':'));
    SkipWhitespace(s);

    switch (s->data[0]) {
      case '[':
        NANOARROW_RETURN_NOT_OK(FindList(s, &v));
        break;
      case '{':
        NANOARROW_RETURN_NOT_OK(FindObject(s, &v));
        break;
      case '\"':
        NANOARROW_RETURN_NOT_OK(FindString(s, &v));
        break;
      case 'n':
        NANOARROW_RETURN_NOT_OK(FindNull(s, &v));
        break;
      default:
        // e.g., a number or boolean
        return EINVAL;
    }

    if (k.size_bytes == 7 && strncmp(k.data, "\"edges\"", 7) == 0) {
      if (v.size_bytes == 11 && strncmp(v.data, "\"spherical\"", 11) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_SPHERICAL;
      } else if (v.size_bytes == 8 && strncmp(v.data, "\"planar\"", 8) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_PLANAR;
      } else if (v.size_bytes == 10 && strncmp(v.data, "\"vincenty\"", 10) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_VINCENTY;
      } else if (v.size_bytes == 8 && strncmp(v.data, "\"thomas\"", 8) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_THOMAS;
      } else if (v.size_bytes == 9 && strncmp(v.data, "\"andoyer\"", 9) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_ANDOYER;
      } else if (v.size_bytes == 8 && strncmp(v.data, "\"karney\"", 8) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_KARNEY;
      } else if (v.data[0] == 'n') {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_PLANAR;
      } else {
        return EINVAL;
      }
    } else if (k.size_bytes == 5 && strncmp(k.data, "\"crs\"", 5) == 0) {
      if (v.data[0] == '{') {
        metadata_view->crs.data = v.data;
        metadata_view->crs.size_bytes = v.size_bytes;
        if (metadata_view->crs_type == GEOARROW_CRS_TYPE_NONE) {
          metadata_view->crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
        }
      } else if (v.data[0] == '\"') {
        metadata_view->crs.data = v.data;
        metadata_view->crs.size_bytes = v.size_bytes;
        if (metadata_view->crs_type == GEOARROW_CRS_TYPE_NONE) {
          metadata_view->crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
        }
      } else if (v.data[0] == 'n') {
        // A null explicitly un-sets the CRS
        metadata_view->crs_type = GEOARROW_CRS_TYPE_NONE;
      } else {
        // Reject an unknown JSON type
        return EINVAL;
      }
    } else if (k.size_bytes == 10 && strncmp(k.data, "\"crs_type\"", 10) == 0) {
      if (v.data[0] == '\"') {
        if (v.size_bytes == 10 && strncmp(k.data, "\"projjson\"", 10)) {
          metadata_view->crs_type = GEOARROW_CRS_TYPE_PROJJSON;
        } else if (v.size_bytes == 11 && strncmp(k.data, "\"wkt2:2019\"", 11)) {
          metadata_view->crs_type = GEOARROW_CRS_TYPE_WKT2_2019;
        } else if (v.size_bytes == 16 && strncmp(k.data, "\"authority_code\"", 16)) {
          metadata_view->crs_type = GEOARROW_CRS_TYPE_AUTHORITY_CODE;
        } else if (v.size_bytes == 6 && strncmp(k.data, "\"srid\"", 6)) {
          metadata_view->crs_type = GEOARROW_CRS_TYPE_SRID;
        } else {
          // Accept unrecognized string values but ignore them
          metadata_view->crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
        }
      } else {
        // Reject values that are not a string
        return EINVAL;
      }
    }

    SkipUntil(s, ",}");
    if (s->data[0] == ',') {
      s->size_bytes--;
      s->data++;
    }
  }

  if (s->size_bytes > 0 && s->data[0] == '}') {
    s->size_bytes--;
    s->data++;
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static GeoArrowErrorCode GeoArrowMetadataViewInitJSON(
    struct GeoArrowMetadataView* metadata_view, struct GeoArrowError* error) {
  struct ArrowStringView metadata;
  metadata.data = metadata_view->metadata.data;
  metadata.size_bytes = metadata_view->metadata.size_bytes;

  struct ArrowStringView s = metadata;
  SkipWhitespace(&s);

  if (ParseJSONMetadata(metadata_view, &s) != GEOARROW_OK) {
    GeoArrowErrorSet(error, "Expected valid GeoArrow JSON metadata but got '%.*s'",
                     (int)metadata.size_bytes, metadata.data);
    return EINVAL;
  }

  SkipWhitespace(&s);
  if (s.data != (metadata.data + metadata.size_bytes)) {
    ArrowErrorSet(
        (struct ArrowError*)error,
        "Expected JSON object with no trailing characters but found trailing '%.*s'",
        (int)s.size_bytes, s.data);
    return EINVAL;
  }

  // Do one final canonicalization: it is possible that the crs_type was set
  // but the crs was not. If this is the case, we need to unset the crs_type to
  // NONE.
  if (metadata_view->crs.size_bytes == 0) {
    metadata_view->crs_type = GEOARROW_CRS_TYPE_NONE;
  }

  return GEOARROW_OK;
}

GeoArrowErrorCode GeoArrowMetadataViewInit(struct GeoArrowMetadataView* metadata_view,
                                           struct GeoArrowStringView metadata,
                                           struct GeoArrowError* error) {
  metadata_view->metadata = metadata;
  metadata_view->edge_type = GEOARROW_EDGE_TYPE_PLANAR;
  metadata_view->crs_type = GEOARROW_CRS_TYPE_NONE;
  metadata_view->crs.data = NULL;
  metadata_view->crs.size_bytes = 0;

  if (metadata.size_bytes == 0) {
    return GEOARROW_OK;
  }

  return GeoArrowMetadataViewInitJSON(metadata_view, error);
}

static int GeoArrowMetadataCrsNeedsEscape(struct GeoArrowStringView crs) {
  return (crs.size_bytes == 0) || (*crs.data != '{' && *crs.data != '"');
}

static int64_t GeoArrowMetadataCalculateSerializedSize(
    const struct GeoArrowMetadataView* metadata_view) {
  const int64_t kSizeOuterBraces = 2;
  const int64_t kSizeQuotes = 2;
  const int64_t kSizeColon = 1;
  const int64_t kSizeComma = 1;
  const int64_t kSizeEdgesKey = 5 + kSizeQuotes + kSizeColon;
  const int64_t kSizeCrsTypeKey = 8 + kSizeQuotes + kSizeColon;
  const int64_t kSizeCrsKey = 3 + kSizeQuotes + kSizeColon;

  int n_keys = 0;
  int64_t size_out = 0;
  size_out += kSizeOuterBraces;

  if (metadata_view->edge_type != GEOARROW_EDGE_TYPE_PLANAR) {
    n_keys += 1;
    size_out += kSizeEdgesKey + kSizeQuotes +
                strlen(GeoArrowEdgeTypeString(metadata_view->edge_type));
  }

  if (metadata_view->crs_type != GEOARROW_CRS_TYPE_UNKNOWN &&
      metadata_view->crs_type != GEOARROW_CRS_TYPE_NONE) {
    n_keys += 1;
    size_out += kSizeCrsTypeKey + kSizeQuotes +
                strlen(GeoArrowCrsTypeString(metadata_view->crs_type));
  }

  if (metadata_view->crs_type != GEOARROW_CRS_TYPE_NONE) {
    n_keys += 1;
    size_out += kSizeCrsKey;

    if (GeoArrowMetadataCrsNeedsEscape(metadata_view->crs)) {
      size_out += kSizeQuotes + metadata_view->crs.size_bytes;
      for (int64_t i = 0; i < metadata_view->crs.size_bytes; i++) {
        char val = metadata_view->crs.data[i];
        size_out += val == '\\' || val == '"';
      }
    } else {
      size_out += metadata_view->crs.size_bytes;
    }
  }

  if (n_keys > 1) {
    size_out += kSizeComma * (n_keys - 1);
  }

  return size_out;
}

static void GeoArrowWriteStringView(struct ArrowStringView sv, char** out) {
  if (sv.size_bytes == 0) {
    return;
  }

  memcpy(*out, sv.data, sv.size_bytes);
  (*out) += sv.size_bytes;
}

static void GeoArrowWriteString(const char* value, char** out) {
  GeoArrowWriteStringView(ArrowCharView(value), out);
}

static int64_t GeoArrowMetadataSerializeInternal(
    const struct GeoArrowMetadataView* metadata_view, char* out) {
  const struct ArrowStringView kEdgesKey = ArrowCharView("\"edges\":");
  const struct ArrowStringView kCrsTypeKey = ArrowCharView("\"crs_type\":");
  const struct ArrowStringView kCrsKey = ArrowCharView("\"crs\":");

  char* out_initial = out;
  int n_keys = 0;

  *out++ = '{';

  if (metadata_view->edge_type != GEOARROW_EDGE_TYPE_PLANAR) {
    n_keys += 1;
    GeoArrowWriteStringView(kEdgesKey, &out);
    *out++ = '"';
    GeoArrowWriteString(GeoArrowEdgeTypeString(metadata_view->edge_type), &out);
    *out++ = '"';
  }

  if (metadata_view->crs_type != GEOARROW_CRS_TYPE_UNKNOWN &&
      metadata_view->crs_type != GEOARROW_CRS_TYPE_NONE) {
    if (n_keys > 0) {
      *out++ = ',';
    }

    n_keys += 1;
    GeoArrowWriteStringView(kCrsTypeKey, &out);
    *out++ = '"';
    GeoArrowWriteString(GeoArrowCrsTypeString(metadata_view->crs_type), &out);
    *out++ = '"';
  }

  if (metadata_view->crs_type != GEOARROW_CRS_TYPE_NONE) {
    if (n_keys > 0) {
      *out++ = ',';
    }

    n_keys += 1;
    GeoArrowWriteStringView(kCrsKey, &out);

    if (GeoArrowMetadataCrsNeedsEscape(metadata_view->crs)) {
      *out++ = '"';
      for (int64_t i = 0; i < metadata_view->crs.size_bytes; i++) {
        char val = metadata_view->crs.data[i];
        if (val == '"') {
          *out++ = '\\';
        }

        *out++ = val;
      }
      *out++ = '"';
    } else {
      struct ArrowStringView sv;
      sv.data = metadata_view->crs.data;
      sv.size_bytes = metadata_view->crs.size_bytes;
      GeoArrowWriteStringView(sv, &out);
    }
  }

  *out++ = '}';
  return out - out_initial;
}

static GeoArrowErrorCode GeoArrowSchemaSetMetadataInternal(
    struct ArrowSchema* schema, const struct GeoArrowMetadataView* metadata_view) {
  int64_t metadata_size = GeoArrowMetadataCalculateSerializedSize(metadata_view);
  char* metadata = (char*)ArrowMalloc(metadata_size);
  if (metadata == NULL) {
    return ENOMEM;
  }

  int64_t chars_written = GeoArrowMetadataSerializeInternal(metadata_view, metadata);
  NANOARROW_DCHECK(chars_written == metadata_size);
  NANOARROW_UNUSED(chars_written);

  struct ArrowBuffer existing_buffer;
  int result = ArrowMetadataBuilderInit(&existing_buffer, schema->metadata);
  if (result != GEOARROW_OK) {
    ArrowFree(metadata);
    return result;
  }

  struct ArrowStringView value;
  value.data = metadata;
  value.size_bytes = metadata_size;
  result = ArrowMetadataBuilderSet(&existing_buffer,
                                   ArrowCharView("ARROW:extension:metadata"), value);
  ArrowFree(metadata);
  if (result != GEOARROW_OK) {
    ArrowBufferReset(&existing_buffer);
    return result;
  }

  result = ArrowSchemaSetMetadata(schema, (const char*)existing_buffer.data);
  ArrowBufferReset(&existing_buffer);
  return result;
}

int64_t GeoArrowMetadataSerialize(const struct GeoArrowMetadataView* metadata_view,
                                  char* out, int64_t n) {
  int64_t metadata_size = GeoArrowMetadataCalculateSerializedSize(metadata_view);
  if (metadata_size <= n) {
    int64_t chars_written = GeoArrowMetadataSerializeInternal(metadata_view, out);
    NANOARROW_DCHECK(chars_written == metadata_size);
  }

  // If there is room, write the null terminator
  if (metadata_size < n) {
    out[metadata_size] = '\0';
  }

  return metadata_size;
}

GeoArrowErrorCode GeoArrowSchemaSetMetadata(
    struct ArrowSchema* schema, const struct GeoArrowMetadataView* metadata_view) {
  return GeoArrowSchemaSetMetadataInternal(schema, metadata_view);
}

GeoArrowErrorCode GeoArrowSchemaSetMetadataFrom(struct ArrowSchema* schema,
                                                const struct ArrowSchema* schema_src) {
  struct ArrowSchemaView schema_view;
  NANOARROW_RETURN_NOT_OK(ArrowSchemaViewInit(&schema_view, schema_src, NULL));

  struct ArrowBuffer buffer;
  NANOARROW_RETURN_NOT_OK(ArrowMetadataBuilderInit(&buffer, schema->metadata));
  int result = ArrowMetadataBuilderSet(&buffer, ArrowCharView("ARROW:extension:metadata"),
                                       schema_view.extension_metadata);
  if (result != GEOARROW_OK) {
    ArrowBufferReset(&buffer);
    return result;
  }

  result = ArrowSchemaSetMetadata(schema, (const char*)buffer.data);
  ArrowBufferReset(&buffer);
  return result;
}

int64_t GeoArrowUnescapeCrs(struct GeoArrowStringView crs, char* out, int64_t n) {
  if (crs.size_bytes == 0) {
    if (n > 0) {
      out[0] = '\0';
    }
    return 0;
  }

  if (crs.data[0] != '\"') {
    if (n > crs.size_bytes) {
      memcpy(out, crs.data, crs.size_bytes);
      out[crs.size_bytes] = '\0';
    } else if (out != NULL) {
      memcpy(out, crs.data, n);
    }

    return crs.size_bytes;
  }

  int64_t out_i = 0;
  int is_escape = 0;
  for (int64_t i = 1; i < (crs.size_bytes - 1); i++) {
    if (!is_escape && crs.data[i] == '\\') {
      is_escape = 1;
      continue;
    } else {
      is_escape = 0;
    }

    if (out_i < n) {
      out[out_i] = crs.data[i];
    }

    out_i++;
  }

  if (out_i < n) {
    out[out_i] = '\0';
  }

  return out_i;
}

static const char* kCrsWgs84 =
    "{\"type\":\"GeographicCRS\",\"name\":\"WGS 84 "
    "(CRS84)\",\"datum_ensemble\":{\"name\":\"World Geodetic System 1984 "
    "ensemble\",\"members\":[{\"name\":\"World Geodetic System 1984 "
    "(Transit)\",\"id\":{\"authority\":\"EPSG\",\"code\":1166}},{\"name\":\"World "
    "Geodetic System 1984 "
    "(G730)\",\"id\":{\"authority\":\"EPSG\",\"code\":1152}},{\"name\":\"World Geodetic "
    "System 1984 "
    "(G873)\",\"id\":{\"authority\":\"EPSG\",\"code\":1153}},{\"name\":\"World Geodetic "
    "System 1984 "
    "(G1150)\",\"id\":{\"authority\":\"EPSG\",\"code\":1154}},{\"name\":\"World Geodetic "
    "System 1984 "
    "(G1674)\",\"id\":{\"authority\":\"EPSG\",\"code\":1155}},{\"name\":\"World Geodetic "
    "System 1984 "
    "(G1762)\",\"id\":{\"authority\":\"EPSG\",\"code\":1156}},{\"name\":\"World Geodetic "
    "System 1984 "
    "(G2139)\",\"id\":{\"authority\":\"EPSG\",\"code\":1309}}],\"ellipsoid\":{\"name\":"
    "\"WGS "
    "84\",\"semi_major_axis\":6378137,\"inverse_flattening\":298.257223563},\"accuracy\":"
    "\"2.0\",\"id\":{\"authority\":\"EPSG\",\"code\":6326}},\"coordinate_system\":{"
    "\"subtype\":\"ellipsoidal\",\"axis\":[{\"name\":\"Geodetic "
    "longitude\",\"abbreviation\":\"Lon\",\"direction\":\"east\",\"unit\":\"degree\"},{"
    "\"name\":\"Geodetic "
    "latitude\",\"abbreviation\":\"Lat\",\"direction\":\"north\",\"unit\":\"degree\"}]},"
    "\"scope\":\"Not "
    "known.\",\"area\":\"World.\",\"bbox\":{\"south_latitude\":-90,\"west_longitude\":-"
    "180,\"north_latitude\":90,\"east_longitude\":180},\"id\":{\"authority\":\"OGC\","
    "\"code\":\"CRS84\"}}";

void GeoArrowMetadataSetLonLat(struct GeoArrowMetadataView* metadata_view) {
  metadata_view->crs.data = kCrsWgs84;
  metadata_view->crs.size_bytes = strlen(kCrsWgs84);
  metadata_view->crs_type = GEOARROW_CRS_TYPE_PROJJSON;
}
