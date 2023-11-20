
#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.sfc <- function(x, ...) {
  infer_geoarrow_schema(x)
}

#' @export
as_geoarrow_array.sfc <- function(x, ..., schema = NULL) {
  if (!is.null(schema)) {
    schema_inferred <- infer_geoarrow_schema(x)
    parsed <- geoarrow_schema_parse(schema)
    parsed_inferred <- geoarrow_schema_parse(schema_inferred)
    stop("Check")
  } else {
    schema <- infer_geoarrow_schema(x)
  }

  if (class(x)[1] %in% c("sfc_POINT",
                         "sfc_LINESTRING",
                         "sfc_POLYGON",
                         "sfc_MULTIPOINT",
                         "sfc_MULTILINESTRING",
                         "sfc_MULTIPOLYGON")) {
    array <- nanoarrow::nanoarrow_allocate_array()
    .Call(geoarrow_c_as_nanoarrow_array_sfc, x, schema, array)
    nanoarrow::nanoarrow_array_set_schema(array, schema)
    array
  } else {
    NextMethod()
  }
}

# Eventually we can add specializations for as_geoarrow_array() based on
# st_as_grob(), which is very fast and generates lengths + a column-major
# matrix full of buffers we can provide views into.

coord_matrix <- function(x) {
  switch(
    class(x)[1],
    "sfc_POINT" = matrix(unlist(x, use.names = FALSE), ncol = length(x)),
    "sfc_LINESTRING" = ,
    "sfc_POLYGON" = ,
    "sfc_MULTIPOINT" = ,
    "sfc_MULTILINESTRING" = ,
    "sfc_MULTIPOLYGON" = do.call(rbind, unclass(x)),
    NULL
  )
}

