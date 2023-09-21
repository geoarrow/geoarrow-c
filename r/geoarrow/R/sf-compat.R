
#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.sfc <- function(x, ...) {
  geoarrow_infer_schema_default(x)
}
