
#include <errno.h>
#include <stdio.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow.h"

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

static GeoArrowErrorCode GeoArrowMetadataSerializeInternal(
    const struct GeoArrowMetadataView* metadata_view, struct ArrowBuffer* buffer) {
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
    NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, metadata_view->crs.data,
                                              metadata_view->crs.size_bytes));
  } else if (metadata_view->crs_type == GEOARROW_CRS_TYPE_UNKNOWN) {
    // Escape quotes in the string if the string does not start with '"'
    if (metadata_view->crs.size_bytes > 0 && metadata_view->crs.data[0] == '\"') {
      NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, metadata_view->crs.data,
                                                metadata_view->crs.size_bytes));
    } else {
      NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "\"", 1));
      for (int64_t i = 0; i < metadata_view->crs.size_bytes; i++) {
        char c = metadata_view->crs.data[i];
        if (c == '\"') {
          NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "\\", 1));
        }
        NANOARROW_RETURN_NOT_OK(ArrowBufferAppendInt8(buffer, c));
      }
      NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "\"", 1));
    }
  }

  NANOARROW_RETURN_NOT_OK(ArrowBufferAppend(buffer, "}", 1));
  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowSchemaSetMetadataInternal(
    struct ArrowSchema* schema, const struct GeoArrowMetadataView* metadata_view) {
  struct ArrowBuffer buffer;
  ArrowBufferInit(&buffer);

  int result = GeoArrowMetadataSerializeInternal(metadata_view, &buffer);
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
  value.size_bytes = buffer.size_bytes;
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

int64_t GeoArrowMetadataSerialize(const struct GeoArrowMetadataView* metadata_view,
                                  char* out, int64_t n) {
  struct ArrowBuffer buffer;
  ArrowBufferInit(&buffer);
  int result = ArrowBufferReserve(&buffer, n);
  if (result != GEOARROW_OK) {
    ArrowBufferReset(&buffer);
    return -1;
  }

  result = GeoArrowMetadataSerializeInternal(metadata_view, &buffer);
  if (result != GEOARROW_OK) {
    ArrowBufferReset(&buffer);
    return -1;
  }

  int64_t size_needed = buffer.size_bytes;
  int64_t n_copy;
  if (n >= size_needed) {
    n_copy = size_needed;
  } else {
    n_copy = n;
  }

  if (n_copy > 0) {
    memcpy(out, buffer.data, n_copy);
  }

  if (n > size_needed) {
    out[size_needed] = '\0';
  }

  ArrowBufferReset(&buffer);
  return size_needed;
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
