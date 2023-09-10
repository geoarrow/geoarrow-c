
enum_value <- function(x, enum_name) {
  values <- unlist(enum[[enum_name]])
  if (is.character(x)) {
    unname(values[x])
  } else {
    as.integer(ifelse(x %in% values, x, NA_integer_))
  }
}

enum_label <- function(x, enum_name) {
  values <- unlist(enum[[enum_name]])
  if (is.character(x)) {
    as.character(ifelse(x %in% names(values), x, NA_character_))
  } else {
    ids <- match(x, values)
    names(values)[ids]
  }
}

enum <- list(
  GeometryType = list(
    GEOMETRY = 0L,
    POINT = 1L,
    LINESTRING = 2L,
    POLYGON = 3L,
    MULTIPOINT = 4L,
    MULTILINESTRING = 5L,
    MULTIPOLYGON = 6L,
    GEOMETRYCOLLECTION = 7L
  ),
  Dimensions = list(
    UNKNOWN = 0L,
    XY = 1L,
    XYZ = 2L,
    XYM = 3L,
    XYZM = 4L
  ),
  CoordType = list(
    UNKNOWN = 0L,
    SEPARATE = 1L,
    INTERLEAVED = 2L
  ),
  CrsType = list(
    NONE = 0L,
    UNKNOWN = 1L,
    PROJJSON = 2L
  )
)

stop_invalid_enum <- function(x, enum_name) {
  stop(sprintf("%s is not a valid enum label or value for %s", x, enum_name))
}
