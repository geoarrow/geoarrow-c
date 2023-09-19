
#' @export
as_geoarrow_array.wk_wkt <- function(x, ..., schema = NULL) {
  if (!is.null(schema)) {
    if (!identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.wkt")) {
      return(NextMethod())
    }
  } else {
    schema <- infer_nanoarrow_schema(x)
  }

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

#' @export
as_geoarrow_array.wk_wkb <- function(x, ..., schema = NULL) {
  if (!is.null(schema)) {
    if (!identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.wkb")) {
      return(NextMethod())
    }
  } else {
    schema <- infer_nanoarrow_schema(x)
  }

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

#' @export
as_geoarrow_array.wk_xy <- function(x, ..., schema = NULL) {
  if (!is.null(schema)) {
    if (!identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.point")) {
      return(NextMethod())
    }
  }

  schema <- infer_nanoarrow_schema(x)
  data <- unclass(x)
  geoarrow_array_from_buffers(
    schema,
    # Treating NULLs as EMPTY for now
    c(list(NULL), data)
  )
}

#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.wk_wkt <- function(x, ...) {
  data <- unclass(x)
  schema <- nanoarrow::infer_nanoarrow_schema(data)
  schema_for_metadata <- wk_geoarrow_schema(x, na_extension_wkt)
  schema$metadata <- schema_for_metadata$metadata
  schema
}

#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.wk_wkb <- function(x, ...) {
  data <- structure(x, class = "blob")
  schema <- nanoarrow::infer_nanoarrow_schema(data)
  schema_for_metadata <- wk_geoarrow_schema(x, na_extension_wkb)
  schema$metadata <- schema_for_metadata$metadata
  schema
}

#' @importFrom nanoarrow infer_nanoarrow_schema
#' @export
infer_nanoarrow_schema.wk_xy <- function(x, ...) {
  data <- unclass(x)
  dims <- paste0(toupper(names(data)), collapse = "")
  wk_geoarrow_schema(x, na_extension_geoarrow, "POINT", dimensions = dims)
}

wk_geoarrow_schema <- function(x, type_constructor, ...) {
  crs <- wk::wk_crs(x)
  edges <- if (wk::wk_is_geodesic(x)) "SPHERICAL" else "PLANAR"
  type_constructor(..., crs = crs, edges = edges)
}
