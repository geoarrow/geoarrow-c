#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

struct ParseSource {
  const char* data;
  int64_t n_bytes;
  const char* data0;
};

int from_chars_internal(const char* first, const char* last, double* out) {
  char* end_ptr;
  *out = strtod(first, &end_ptr);
  if (end_ptr != last) {
    return EINVAL;
  }

  return GEOARROW_OK;
}

static inline void AdvanceUnsafe(struct ParseSource* s, int64_t n) {
  s->data += n;
  s->n_bytes -= n;
}

static inline void SkipWhitespace(struct ParseSource* s) {
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

static inline int SkipUntil(struct ParseSource* s, const char* items) {
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

static inline void SkipUntilSep(struct ParseSource* s) { SkipUntil(s, " \n\t\r,()"); }

static inline char PeekChar(struct ParseSource* s) {
  if (s->n_bytes > 0) {
    return s->data[0];
  } else {
    return '\0';
  }
}

static inline struct ArrowStringView PeekUntilSep(struct ParseSource* s, int max_chars) {
  struct ParseSource tmp = *s;
  if (tmp.n_bytes > max_chars) {
    tmp.n_bytes = max_chars;
  }

  SkipUntilSep(&tmp);
  struct ArrowStringView out = {s->data, tmp.data - s->data};
  return out;
}

static inline void SetParseErrorAuto(const char* expected, struct ParseSource* s,
                                     struct GeoArrowError* error) {
  long pos = s->data - s->data0;
  // TODO: "but found ..." from s
  ArrowErrorSet((struct ArrowError*)error, "Expected %s at byte %ld", expected, pos);
}

static inline int AssertChar(struct ParseSource* s, char c, struct GeoArrowError* error) {
  if (s->n_bytes > 0 && s->data[0] == c) {
    return GEOARROW_OK;
  } else {
    char expected[4] = {'\'', c, '\'', '\0'};
    SetParseErrorAuto(expected, s, error);
    return EINVAL;
  }
}

static inline int AssertWhitespace(struct ParseSource* s, struct GeoArrowError* error) {
  if (s->n_bytes > 0 && (s->data[0] == ' ' || s->data[0] == '\t' || s->data[0] == '\r' ||
                         s->data[0] == '\n')) {
    return GEOARROW_OK;
  } else {
    SetParseErrorAuto("whitespace", s, error);
    return EINVAL;
  }
}

static inline int ReadOrdinate(struct ParseSource* s, double* out,
                               struct GeoArrowError* error) {
  const char* start;
  SkipUntilSep(s);
  int result = from_chars_internal(start, s->data, out);
  if (result != GEOARROW_OK) {
    s->data = start;
    SetParseErrorAuto("number", s, error);
  }

  return result;
}

static inline int ReadCoordinate(struct ParseSource* s, struct GeoArrowVisitor* v,
                                 int n_dims) {
  double coords[4];
  NANOARROW_RETURN_NOT_OK(ReadOrdinate(s, coords + 0, v->error));
  for (int i = 1; i < n_dims; i++) {
    NANOARROW_RETURN_NOT_OK(AssertWhitespace(s, v->error));
    NANOARROW_RETURN_NOT_OK(ReadOrdinate(s, coords + 0, v->error));
  }

  double* coord_ptr[] = {coords + 0, coords + 1, coords + 2, coords + 3};
  return v->coords(v, (const double**)coord_ptr, 1, n_dims);
}

static inline int ReadEmptyOrCoordinates(struct ParseSource* s, struct GeoArrowVisitor* v,
                                         int n_dims) {
  SkipWhitespace(s);
  if (PeekChar(s) == '(') {
    AdvanceUnsafe(s, 1);
    SkipWhitespace(s);

    // Read the first coordinate (there must always be one)
    NANOARROW_RETURN_NOT_OK(ReadCoordinate(s, v, n_dims));
    SkipWhitespace(s);

    // Read the rest of the coordinates
    while (PeekChar(s) != ')') {
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(AssertChar(s, ',', v->error));
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(ReadCoordinate(s, v, n_dims));
      SkipWhitespace(s);
    }

    AdvanceUnsafe(s, 1);
    return GEOARROW_OK;
  }

  struct ArrowStringView word = PeekUntilSep(s, 6);
  if (word.n_bytes == 5 && strncmp(word.data, "EMPTY", 5) == 0) {
    AdvanceUnsafe(s, 5);
    return GEOARROW_OK;
  }

  SetParseErrorAuto("'(' or 'EMPTY'", s, v->error);
  return EINVAL;
}

static inline int ReadEmptyOrPointCoordinate(struct ParseSource* s,
                                             struct GeoArrowVisitor* v, int n_dims) {
  SkipWhitespace(s);
  if (PeekChar(s) == '(') {
    AdvanceUnsafe(s, 1);

    SkipWhitespace(s);
    NANOARROW_RETURN_NOT_OK(ReadCoordinate(s, v, n_dims));
    SkipWhitespace(s);
    NANOARROW_RETURN_NOT_OK(AssertChar(s, ')', v->error));
    return GEOARROW_OK;
  }

  struct ArrowStringView word = PeekUntilSep(s, 6);
  if (word.n_bytes == 5 && strncmp(word.data, "EMPTY", 5) == 0) {
    AdvanceUnsafe(s, 5);
    return GEOARROW_OK;
  }

  SetParseErrorAuto("'(' or 'EMPTY'", s, v->error);
  return EINVAL;
}

static inline int ReadPolygon(struct ParseSource* s, struct GeoArrowVisitor* v, int n_dims) {
  SkipWhitespace(s);
  if (PeekChar(s) == '(') {
    AdvanceUnsafe(s, 1);
    SkipWhitespace(s);

    // Read the first ring (there must always be one)
    NANOARROW_RETURN_NOT_OK(v->ring_start(v));
    NANOARROW_RETURN_NOT_OK(ReadEmptyOrCoordinates(s, v, n_dims));
    NANOARROW_RETURN_NOT_OK(v->ring_end(v));
    SkipWhitespace(s);

    // Read the rest of the rings
    while (PeekChar(s) != ')') {
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(AssertChar(s, ',', v->error));
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(v->ring_start(v));
      NANOARROW_RETURN_NOT_OK(ReadEmptyOrCoordinates(s, v, n_dims));
      NANOARROW_RETURN_NOT_OK(v->ring_end(v));
      SkipWhitespace(s);
    }

    AdvanceUnsafe(s, 1);
    return GEOARROW_OK;
  }

  struct ArrowStringView word = PeekUntilSep(s, 6);
  if (word.n_bytes == 5 && strncmp(word.data, "EMPTY", 5) == 0) {
    AdvanceUnsafe(s, 5);
    return GEOARROW_OK;
  }

  SetParseErrorAuto("'(' or 'EMPTY'", s, v->error);
  return EINVAL;
}

static inline int ReadMultipoint(struct ParseSource* s, struct GeoArrowVisitor* v,
                             enum GeoArrowDimensions dimensions, int n_dims) {
  SkipWhitespace(s);
  if (PeekChar(s) == '(') {
    AdvanceUnsafe(s, 1);
    SkipWhitespace(s);

    // Read the first geometry (there must always be one)
    NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POINT, dimensions));
    NANOARROW_RETURN_NOT_OK(ReadEmptyOrPointCoordinate(s, v, n_dims));
    NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    SkipWhitespace(s);

    // Read the rest of the geometries
    while (PeekChar(s) != ')') {
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(AssertChar(s, ',', v->error));
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POINT, dimensions));
      NANOARROW_RETURN_NOT_OK(ReadEmptyOrPointCoordinate(s, v, n_dims));
      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
      SkipWhitespace(s);
    }

    AdvanceUnsafe(s, 1);
    return GEOARROW_OK;
  }

  struct ArrowStringView word = PeekUntilSep(s, 6);
  if (word.n_bytes == 5 && strncmp(word.data, "EMPTY", 5) == 0) {
    AdvanceUnsafe(s, 5);
    return GEOARROW_OK;
  }

  SetParseErrorAuto("'(' or 'EMPTY'", s, v->error);
  return EINVAL;
}

static inline int ReadMultilinestring(struct ParseSource* s, struct GeoArrowVisitor* v,
                                  enum GeoArrowDimensions dimensions, int n_dims) {
  SkipWhitespace(s);
  if (PeekChar(s) == '(') {
    AdvanceUnsafe(s, 1);
    SkipWhitespace(s);

    // Read the first geometry (there must always be one)
    NANOARROW_RETURN_NOT_OK(
        v->geom_start(v, GEOARROW_GEOMETRY_TYPE_LINESTRING, dimensions));
    NANOARROW_RETURN_NOT_OK(ReadEmptyOrCoordinates(s, v, n_dims));
    NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    SkipWhitespace(s);

    // Read the rest of the geometries
    while (PeekChar(s) != ')') {
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(AssertChar(s, ',', v->error));
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(
          v->geom_start(v, GEOARROW_GEOMETRY_TYPE_LINESTRING, dimensions));
      NANOARROW_RETURN_NOT_OK(ReadEmptyOrCoordinates(s, v, n_dims));
      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
      SkipWhitespace(s);
    }

    AdvanceUnsafe(s, 1);
    return GEOARROW_OK;
  }

  struct ArrowStringView word = PeekUntilSep(s, 6);
  if (word.n_bytes == 5 && strncmp(word.data, "EMPTY", 5) == 0) {
    AdvanceUnsafe(s, 5);
    return GEOARROW_OK;
  }

  SetParseErrorAuto("'(' or 'EMPTY'", s, v->error);
  return EINVAL;
}

static inline int ReadMultipolygon(struct ParseSource* s, struct GeoArrowVisitor* v,
                               enum GeoArrowDimensions dimensions, int n_dims) {
  SkipWhitespace(s);
  if (PeekChar(s) == '(') {
    AdvanceUnsafe(s, 1);
    SkipWhitespace(s);

    // Read the first geometry (there must always be one)
    NANOARROW_RETURN_NOT_OK(v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POLYGON, dimensions));
    NANOARROW_RETURN_NOT_OK(ReadPolygon(s, v, n_dims));
    NANOARROW_RETURN_NOT_OK(v->geom_end(v));
    SkipWhitespace(s);

    // Read the rest of the geometries
    while (PeekChar(s) != ')') {
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(AssertChar(s, ',', v->error));
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(
          v->geom_start(v, GEOARROW_GEOMETRY_TYPE_POLYGON, dimensions));
      NANOARROW_RETURN_NOT_OK(ReadPolygon(s, v, n_dims));
      NANOARROW_RETURN_NOT_OK(v->geom_end(v));
      SkipWhitespace(s);
    }

    AdvanceUnsafe(s, 1);
    return GEOARROW_OK;
  }

  struct ArrowStringView word = PeekUntilSep(s, 6);
  if (word.n_bytes == 5 && strncmp(word.data, "EMPTY", 5) == 0) {
    AdvanceUnsafe(s, 5);
    return GEOARROW_OK;
  }

  SetParseErrorAuto("'(' or 'EMPTY'", s, v->error);
  return EINVAL;
}

static inline int ReadTaggedGeometry(struct ParseSource* s, struct GeoArrowVisitor* v);

static inline int ReadGeometryCollection(struct ParseSource* s, struct GeoArrowVisitor* v) {
  SkipWhitespace(s);
  if (PeekChar(s) == '(') {
    AdvanceUnsafe(s, 1);
    SkipWhitespace(s);

    // Read the first geometry (there must always be one)
    NANOARROW_RETURN_NOT_OK(v->ring_start(v));
    NANOARROW_RETURN_NOT_OK(ReadTaggedGeometry(s, v));
    NANOARROW_RETURN_NOT_OK(v->ring_end(v));
    SkipWhitespace(s);

    // Read the rest of the geometries
    while (PeekChar(s) != ')') {
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(AssertChar(s, ',', v->error));
      SkipWhitespace(s);
      NANOARROW_RETURN_NOT_OK(v->ring_start(v));
      NANOARROW_RETURN_NOT_OK(ReadTaggedGeometry(s, v));
      NANOARROW_RETURN_NOT_OK(v->ring_end(v));
      SkipWhitespace(s);
    }

    AdvanceUnsafe(s, 1);
    return GEOARROW_OK;
  }

  struct ArrowStringView word = PeekUntilSep(s, 6);
  if (word.n_bytes == 5 && strncmp(word.data, "EMPTY", 5) == 0) {
    AdvanceUnsafe(s, 5);
    return GEOARROW_OK;
  }

  SetParseErrorAuto("'(' or 'EMPTY'", s, v->error);
  return EINVAL;
}

static inline int ReadTaggedGeometry(struct ParseSource* s, struct GeoArrowVisitor* v) {
  SkipWhitespace(s);

  struct ArrowStringView word = PeekUntilSep(s, 19);
  enum GeoArrowGeometryType geometry_type;
  if (word.n_bytes == 5 && strncmp(word.data, "POINT", 5) == 0) {
    geometry_type = GEOARROW_GEOMETRY_TYPE_POINT;
  } else if (word.n_bytes == 11 && strncmp(word.data, "LINESTRING", 11) == 0) {
    geometry_type = GEOARROW_GEOMETRY_TYPE_LINESTRING;
  } else if (word.n_bytes == 7 && strncmp(word.data, "POLYGON", 7) == 0) {
    geometry_type = GEOARROW_GEOMETRY_TYPE_POLYGON;
  } else if (word.n_bytes == 10 && strncmp(word.data, "MULTIPOINT", 10) == 0) {
    geometry_type = GEOARROW_GEOMETRY_TYPE_MULTIPOINT;
  } else if (word.n_bytes == 15 && strncmp(word.data, "MULTILINESTRING", 15) == 0) {
    geometry_type = GEOARROW_GEOMETRY_TYPE_MULTILINESTRING;
  } else if (word.n_bytes == 12 && strncmp(word.data, "MULTIPOLYGON", 12) == 0) {
    geometry_type = GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON;
  } else if (word.n_bytes == 18 && strncmp(word.data, "GEOMETRYCOLLECTION", 18) == 0) {
    geometry_type = GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION;
  } else {
    SetParseErrorAuto("geometry type", s, v->error);
    return EINVAL;
  }

  AdvanceUnsafe(s, word.n_bytes);
  SkipWhitespace(s);

  enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY;
  int n_dims = 2;
  word = PeekUntilSep(s, 3);
  if (word.n_bytes == 1 && strncmp(word.data, "Z", 1) == 0) {
    dimensions = GEOARROW_DIMENSIONS_XYZ;
    n_dims = 3;
    AdvanceUnsafe(s, 1);
  } else if (word.n_bytes == 1 && strncmp(word.data, "M", 1) == 0) {
    dimensions = GEOARROW_DIMENSIONS_XYM;
    n_dims = 3;
    AdvanceUnsafe(s, 1);
  } else if (word.n_bytes == 2 && strncmp(word.data, "ZM", 2) == 0) {
    dimensions = GEOARROW_DIMENSIONS_XYZM;
    n_dims = 4;
    AdvanceUnsafe(s, 2);
  }

  NANOARROW_RETURN_NOT_OK(v->geom_start(v, geometry_type, dimensions));

  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_POINT:
      NANOARROW_RETURN_NOT_OK(ReadEmptyOrPointCoordinate(s, v, n_dims));
      break;
    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      NANOARROW_RETURN_NOT_OK(ReadEmptyOrCoordinates(s, v, n_dims));
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      NANOARROW_RETURN_NOT_OK(ReadPolygon(s, v, n_dims));
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
    case GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION:
    default:
      ArrowErrorSet((struct ArrowError*)v->error,
                    "Internal error: unrecognized geometry type id");
      return EINVAL;
  }

  return v->geom_end(v);
}
