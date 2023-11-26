
# exported in zzz.R
st_as_sfc.geoarrow_vctr <- function(x, ...) {
  sfc <- wk::wk_handle(x, wk::sfc_writer())
  sf::st_set_crs(sfc, sf::st_crs(wk::wk_crs(x)))
}

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

  meta <- wk::wk_vector_meta(x)

  # Let the default method handle M values (the optimized path doesn't
  # handle mixed XYZ/XYZM/XYM but can deal with mixed XY and XYZ)
  if (meta$has_m) {
    return(NextMethod())
  }

  if (meta$geometry_type %in% 1:6) {
    schema <- infer_geoarrow_schema(x)
    array <- nanoarrow::nanoarrow_allocate_array()
    .Call(geoarrow_c_as_nanoarrow_array_sfc, x, schema, array)
    nanoarrow::nanoarrow_array_set_schema(array, schema)
    array
  } else {
    NextMethod()
  }
}

#' @importFrom nanoarrow as_nanoarrow_array
#' @export
as_nanoarrow_array.sfc <- function(x, ..., schema = NULL) {
  as_geoarrow_array(x, ..., schema = schema)
}
