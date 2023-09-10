
na_extension_wkb <- function(crs = NULL, edge_type = "PLANAR") {
  na_extension_geoarrow_internal(
    enum$Type$WKB,
    crs = crs,
    edge_type = edge_type
  )
}

na_extension_wkt <- function(crs = NULL, edge_type = "PLANAR") {
  na_extension_geoarrow_internal(
    enum$Type$WKT,
    crs = crs,
    edge_type = edge_type
  )
}

na_extension_large_wkb <- function(crs = NULL, edge_type = "PLANAR") {
  na_extension_geoarrow_internal(
    enum$Type$LARGE_WKB,
    crs = crs,
    edge_type = edge_type
  )
}

na_extension_large_wkt <- function(crs = NULL, edge_type = "PLANAR") {
  na_extension_geoarrow_internal(
    enum$Type$LARGE_WKT,
    crs = crs,
    edge_type = edge_type
  )
}

na_extension_geoarrow <- function(geometry_type, dimensions = "XY",
                                  coord_type = "SEPARATE",
                                  crs = NULL, edge_type = "PLANAR") {
  geometry_type <- enum_value_scalar(geometry_type, "GeometryType")
  dimensions <- enum_value_scalar(dimensions, "Dimensions")
  coord_type <- enum_value_scalar(coord_type, "CoordType")

  type_id <- .Call(geoarrow_c_make_type, geometry_type, dimensions, coord_type)
  na_extension_geoarrow_internal(type_id, crs = crs, edge_type = edge_type)
}

na_extension_geoarrow_internal <- function(type_id, crs = NULL, edge_type = "PLANAR") {
  crs <- sanitize_crs(crs)
  edge_type <- enum_value_scalar(edge_type, "EdgeType")
  schema <- nanoarrow::nanoarrow_allocate_schema()
  .Call(
    geoarrow_c_schema_init_extension,
    type_id,
    crs$type,
    crs$crs,edge_type
  )

  schema
}

sanitize_crs <- function(crs = NULL) {
  if (is.null(crs)) {
    return(list(crs_type = enum$CrsType$NONE, crs = NA_character_))
  }

  crs_projjson <- wk::wk_crs_projjson(crs)
  if (identical(crs_projjson, NA_character_)) {
    crs_type <- enum$CrsType$UNKNOWN
    return(list(crs_type = enum$CrsType$UNKNOWN, crs = crs))
  }

  list(crs_type = enum$CrsType$UNKNOWN, crs = crs_projjson)
}

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

enum_value_scalar <- function(x, enum_name) {
  x_clean <- enum_value(x, enum_name)
  if (length(x_clean) != 1 || is.na(x_clean)) {
    stop(sprintf("%s is not a valid enum label or value for %s", x, enum_name))
  }

  x_clean
}

enum <- list(
  Type = list(
    UNINITIALIZED = 0L,
    WKB = 100001L,
    LARGE_WKB = 100002L,
    WKT = 100003L,
    LARGE_WKT = 100004L
  ),
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
  ),
  EdgeType = list(
    PLANAR = 0L,
    SPHERICAL = 1L
  )
)
