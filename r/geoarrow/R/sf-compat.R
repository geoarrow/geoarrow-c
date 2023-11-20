
#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.sfc <- function(x, ...) {
  infer_geoarrow_schema(x)
}

#' @export
as_geoarrow_array.sfc <- function(x, ..., schema = NULL) {
  # Let the default method handle custom output schemas
  if (!is.null(schema)) {
    return(NextMethod())
  }

  # Let the default method handle M values (the optimized path doesn't
  # handle mixed XYZ/XYZM/XYM but can deal with mixed XY and XYZ)
  if (!is.null(attr(x, "m_range"))) {
    return(NextMethod())
  }

  if (class(x)[1] %in% c("sfc_POINT",
                         "sfc_LINESTRING",
                         "sfc_POLYGON",
                         "sfc_MULTIPOINT",
                         "sfc_MULTILINESTRING",
                         "sfc_MULTIPOLYGON")) {
    schema <- infer_geoarrow_schema(x)
    array <- nanoarrow::nanoarrow_allocate_array()
    .Call(geoarrow_c_as_nanoarrow_array_sfc, x, schema, array)
    nanoarrow::nanoarrow_array_set_schema(array, schema)
    array
  } else {
    NextMethod()
  }
}

#' @export
as_nanoarrow_array.sfc <- function(x, ..., schema = NULL) {
  as_geoarrow_array.sfc(x, ..., schema = schema)
}
