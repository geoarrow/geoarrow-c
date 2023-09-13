
#' @export
as_geoarrow_array.wk_wkt <- function(x, ...) {
  schema <- infer_nanoarrow_schema(x)
  data <- unclass(x)
  data[is.na(data)] <- ""
  geoarrow_array_from_buffers(
    na_extension_wkt(),
    list(
      NULL,
      c(0L, cumsum(nchar(data))),
      data
    )
  )
}


#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.wk_wkt <- function(x) {
  data <- unclass(x)
  storage <- nanoarrow::infer_nanoarrow_schema(data)
  crs <- wk::wk_crs(x)
  edge_type <- if (wk::wk_is_geodesic(x)) "SPHERICAL" else "PLANAR"

  if (identical(storage$format, "u")) {
    na_extension_wkt(crs, edge_type)
  } else {
    na_extension_large_wkt(crs, edge_type)
  }
}
