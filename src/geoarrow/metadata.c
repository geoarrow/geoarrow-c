
#include <errno.h>

#include "nanoarrow.h"

#include "geoarrow.h"

#define CHECK_POS(n)         \
  if ((pos + n) > pos_max) { \
    return EINVAL;           \
  }

// A early draft implementation used something like the Arrow C Data interface
// metadata specification instead of JSON. To help with the transition, this
// bit of code parses the original metadata format.
static GeoArrowErrorCode GeoArrowMetadataViewInitDeprecated(
    struct GeoArrowMetadataView* metadata_view, struct GeoArrowError* error) {
  const char* metadata = metadata_view->metadata.data;
  int32_t pos_max = metadata_view->metadata.n_bytes;
  int32_t pos = 0;
  int32_t name_len;
  int32_t value_len;
  int32_t m;

  CHECK_POS(sizeof(int32_t));
  memcpy(&m, metadata + pos, sizeof(int32_t));
  pos += sizeof(int32_t);

  for (int j = 0; j < m; j++) {
    CHECK_POS(sizeof(int32_t));
    memcpy(&name_len, metadata + pos, sizeof(int32_t));
    pos += sizeof(int32_t);

    CHECK_POS(name_len)
    const char* name = metadata + pos;
    pos += name_len;

    CHECK_POS(sizeof(int32_t))
    memcpy(&value_len, metadata + pos, sizeof(int32_t));
    pos += sizeof(int32_t);

    CHECK_POS(value_len)
    const char* value = metadata + pos;
    pos += value_len;

    if (name_len == 0 || value_len == 0) {
      continue;
    }

    if (name_len == 3 && strncmp(name, "crs", 3) == 0) {
      metadata_view->crs.n_bytes = value_len;
      metadata_view->crs.data = value;
      metadata_view->crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
    } else if (name_len == 5 && strncmp(name, "edges", 5) == 0) {
      if (value_len == 9 && strncmp(value, "spherical", 9) == 0) {
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

static int AssertChar(struct ArrowStringView* s, char c) {
  if (s->n_bytes > 0 && s->data[0] == c) {
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static int ParseChar(struct ArrowStringView* s, char c) {
  if (s->n_bytes > 0 && s->data[0] == c) {
    s->n_bytes--;
    s->data++;
    return GEOARROW_OK;
  } else {
    return EINVAL;
  }
}

static void SkipWhitespace(struct ArrowStringView* s) {
  while (s->n_bytes > 0) {
    char c = *(s->data);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      s->n_bytes--;
      s->data++;
    } else {
      break;
    }
  }
}

static int SkipUntil(struct ArrowStringView* s, const char* items) {
  int n_items = strlen(items);
  while (s->n_bytes > 0) {
    char c = *(s->data);
    if (c == '\0') {
      return 0;
    }

    for (int i = 0; i < n_items; i++) {
      if (c == items[i]) {
        return 1;
      }
    }

    s->n_bytes--;
    s->data++;
  }

  return 0;
}

static GeoArrowErrorCode FindString(struct ArrowStringView* s,
                                    struct ArrowStringView* out) {
  out->data = s->data;
  if (s->data[0] != '\"') {
    return EINVAL;
  }

  s->n_bytes--;
  s->data++;

  int is_escape = 0;
  while (s->n_bytes > 0) {
    char c = *(s->data);
    if (!is_escape && c == '\\') {
      is_escape = 1;
      s->n_bytes--;
      s->data++;
      continue;
    }

    if (!is_escape && c == '\"') {
      s->n_bytes--;
      s->data++;
      out->n_bytes = s->data - out->data;
      return GEOARROW_OK;
    }

    s->n_bytes--;
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

  s->n_bytes--;
  s->data++;
  struct ArrowStringView tmp_value;
  while (s->n_bytes > 0) {
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
          s->n_bytes--;
          s->data++;
          out->n_bytes = s->data - out->data;
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

  s->n_bytes--;
  s->data++;
  struct ArrowStringView tmp_value;
  while (s->n_bytes > 0) {
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
          s->n_bytes--;
          s->data++;
          out->n_bytes = s->data - out->data;
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
  struct ArrowStringView k;
  struct ArrowStringView v;

  while (s->n_bytes > 0 && s->data[0] != '}') {
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
      default:
        break;
    }

    if (k.n_bytes == 7 && strncmp(k.data, "\"edges\"", 7) == 0) {
      if (v.n_bytes == 11 && strncmp(v.data, "\"spherical\"", 11) == 0) {
        metadata_view->edge_type = GEOARROW_EDGE_TYPE_SPHERICAL;
      }
    } else if (k.n_bytes == 5 && strncmp(k.data, "\"crs\"", 5) == 0) {
      if (v.data[0] == '{') {
        metadata_view->crs_type = GEOARROW_CRS_TYPE_PROJJSON;
      } else if (v.data[0] == '\"') {
        metadata_view->crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
      } else {
        return EINVAL;
      }

      metadata_view->crs.data = v.data;
      metadata_view->crs.n_bytes = v.n_bytes;
    }

    SkipUntil(s, ",}");
  }

  if (s->n_bytes > 0 && s->data[0] == '}') {
    s->n_bytes--;
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
  metadata.n_bytes = metadata_view->metadata.n_bytes;

  struct ArrowStringView s = metadata;
  SkipWhitespace(&s);

  if (ParseJSONMetadata(metadata_view, &s) != GEOARROW_OK) {
    ArrowErrorSet((struct ArrowError*)error,
                  "Expected valid GeoArrow JSON metadata but got '%.*s'",
                  (int)metadata.n_bytes, metadata.data);
    return EINVAL;
  }

  SkipWhitespace(&s);
  if (s.data != (metadata.data + metadata.n_bytes)) {
    ArrowErrorSet(
        (struct ArrowError*)error,
        "Expected JSON object with no trailing characters but found trailing '%.*s'",
        (int)s.n_bytes, s.data);
    return EINVAL;
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

static GeoArrowErrorCode GeoArrowMetadataSerializeDeprecated(
    struct GeoArrowMetadataView* metadata_view, struct ArrowBuffer* buffer) {
  switch (metadata_view->edge_type) {
    case GEOARROW_EDGE_TYPE_SPHERICAL:
      NANOARROW_RETURN_NOT_OK(ArrowMetadataBuilderAppend(buffer, ArrowCharView("edges"),
                                                         ArrowCharView("spherical")));
      break;
    default:
      break;
  }

  struct ArrowStringView crs_value;
  if (metadata_view->crs.n_bytes > 0) {
    crs_value.data = metadata_view->crs.data;
    crs_value.n_bytes = metadata_view->crs.n_bytes;
    NANOARROW_RETURN_NOT_OK(
        ArrowMetadataBuilderAppend(buffer, ArrowCharView("crs"), crs_value));
  }

  return NANOARROW_OK;
}

static GeoArrowErrorCode GeoArrowMetadataSerialize(
    struct GeoArrowMetadataView* metadata_view, struct ArrowBuffer* buffer) {
  NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "{", 1));

  int needs_leading_comma = 0;
  const char* spherical_edges_json = "\"edges\":\"spherical\"";
  switch (metadata_view->edge_type) {
    case GEOARROW_EDGE_TYPE_SPHERICAL:
      NANOARROW_RETURN_NOT_OK(
          ArrowBufferAppend(buffer, spherical_edges_json, strlen(spherical_edges_json)));
      needs_leading_comma = 1;
      break;
    default:
      break;
  }

  if (metadata_view->crs_type != GEOARROW_CRS_TYPE_NONE && needs_leading_comma) {
    NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, ",", 1));
  }

  if (metadata_view->crs_type != GEOARROW_CRS_TYPE_NONE) {
    const char* crs_json_prefix = "\"crs\":";
    NANOARROW_RETURN_NOT_OK(
        ArrowBufferAppend(buffer, crs_json_prefix, strlen(crs_json_prefix)));
  }

  if (metadata_view->crs_type == GEOARROW_CRS_TYPE_PROJJSON) {
    NANOARROW_RETURN_NOT_OK(
        ArrowBufferAppend(buffer, metadata_view->crs.data, metadata_view->crs.n_bytes));
  } else if (metadata_view->crs_type == GEOARROW_CRS_TYPE_UNKNOWN) {
    NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "\"", 1));

    // Escape quotes in the string!
    for (int64_t i = 0; i < metadata_view->crs.n_bytes; i++) {
      char c = metadata_view->crs.data[i];
      if (c == '\"') {
        NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "\\", 1));
      }

      NANOARROW_RETURN_NOT_OK(ArrowBufferAppendInt8(buffer, c));
    }

    NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "\"", 1));
  }

  NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "}", 1));
  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowSchemaSetMetadataInternal(
    struct ArrowSchema* schema, struct GeoArrowMetadataView* metadata_view,
    int use_deprecated) {
  struct ArrowBuffer buffer;
  ArrowBufferInit(&buffer);

  int result = 0;
  if (use_deprecated) {
    result = GeoArrowMetadataSerializeDeprecated(metadata_view, &buffer);
  } else {
    result = GeoArrowMetadataSerialize(metadata_view, &buffer);
  }

  if (result != GEOARROW_OK) {
    ArrowBufferReset(&buffer);
    return result;
  }

  struct ArrowBuffer existing_buffer;
  result = ArrowMetadataBuilderInit(&existing_buffer, schema->metadata);
  if (result != GEOARROW_OK) {
    ArrowBufferReset(&buffer);
    return result;
  }

  struct ArrowStringView value;
  value.data = (const char*)buffer.data;
  value.n_bytes = buffer.size_bytes;
  result = ArrowMetadataBuilderSet(&existing_buffer,
                                   ArrowCharView("ARROW:extension:metadata"), value);
  ArrowBufferReset(&buffer);
  if (result != GEOARROW_OK) {
    ArrowBufferReset(&existing_buffer);
    return result;
  }

  result = ArrowSchemaSetMetadata(schema, (const char*)existing_buffer.data);
  ArrowBufferReset(&existing_buffer);
  return result;
}

GeoArrowErrorCode GeoArrowSchemaSetMetadata(struct ArrowSchema* schema,
                                            struct GeoArrowMetadataView* metadata_view) {
  return GeoArrowSchemaSetMetadataInternal(schema, metadata_view, 0);
}

GeoArrowErrorCode GeoArrowSchemaSetMetadataDeprecated(
    struct ArrowSchema* schema, struct GeoArrowMetadataView* metadata_view) {
  return GeoArrowSchemaSetMetadataInternal(schema, metadata_view, 1);
}
