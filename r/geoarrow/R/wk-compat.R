
#' @export
as_geoarrow_array.wk_wkt <- function(x, ..., schema = NULL) {
  if (!is.null(schema)) {
    # TODO: Check if this is geoarrow.wkt
    return(NextMethod())
  }

  schema <- infer_nanoarrow_schema(x)
  schema_storage <- nanoarrow::nanoarrow_schema_modify(
    schema,
    list(metadata = NULL)
  )
  storage <- nanoarrow::as_nanoarrow_array(unclass(x), schema = schema_storage)
  storage_shallow_copy <- nanoarrow::nanoarrow_allocate_array()
  nanoarrow::nanoarrow_pointer_export(storage, storage_shallow_copy)
  nanoarrow::nanoarrow_array_set_schema(storage_shallow_copy, schema)

  storage_shallow_copy
}

#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.wk_wkt <- function(x, ...) {
  data <- unclass(x)
  schema <- nanoarrow::infer_nanoarrow_schema(data)

  crs <- wk::wk_crs(x)
  edge_type <- if (wk::wk_is_geodesic(x)) "SPHERICAL" else "PLANAR"

  schema_for_metadata <- na_extension_wkt(crs, edge_type)
  schema$metadata <- schema_for_metadata$metadata
  schema
}
