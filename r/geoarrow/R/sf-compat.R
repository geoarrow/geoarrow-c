
#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.sfc <- function(x, ...) {
  geometry_type <- substr(class(x)[1], 5, nchar(class(x)[1]))

  # TODO: Infer using wk_vector_meta or wk_meta
  if (identical(geometry_type, "GEOMETRY") || identical(geometry_type, "GEOMETRYCOLLECTION")) {
    return(wk_geoarrow_schema(x, na_extension_wkb))
  }

  has_z <- !is.null(attr(x, "z_range"))
  has_m <- !is.null(attr(x, "m_range"))
  dims <- if (has_z && has_m) {
    "XYZM"
  } else if (has_z) {
    "XYZ"
  } else if (has_m) {
    "XYM"
  } else {
    "XY"
  }

  wk_geoarrow_schema(x, na_extension_geoarrow, geometry_type, dimensions = dims)
}
