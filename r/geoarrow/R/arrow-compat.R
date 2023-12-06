
as_arrow_array.geoarrow_vctr <- function(x, ..., type = NULL) {
  chunked <- as_chunked_array.geoarrow_vctr(x, ..., type = type)
  if (chunked$num_chunks == 1) {
    chunked$chunk(0)
  } else {
    arrow::as_arrow_array(chunked)
  }
}

as_chunked_array.geoarrow_vctr <- function(x, ..., type = NULL) {
  if (is.null(type)) {
    type <- arrow::as_data_type(attr(x, "schema", exact = TRUE))
    chunks <- attr(x, "chunks", exact = TRUE)
  } else {
    stream <- as_nanoarrow_array_stream(x, schema = as_nanoarrow_schema(type))
    chunks <- nanoarrow::collect_array_stream(stream, validate = FALSE)
    type <- arrow::as_data_type(type)
  }

  schema <- as_nanoarrow_schema(type)
  arrays <- vector("list", length(chunks))
  for (i in seq_along(arrays)) {
    tmp_schema <- nanoarrow::nanoarrow_allocate_schema()
    nanoarrow::nanoarrow_pointer_export(schema, tmp_schema)
    tmp_array <- nanoarrow::nanoarrow_allocate_array()
    nanoarrow::nanoarrow_pointer_export(chunks[[i]], tmp_array)
    arrays[[i]] <- arrow::Array$import_from_c(tmp_array, tmp_schema)
  }

  arrow::ChunkedArray$create(!!!arrays, type = type)
}

#' @export
as_geoarrow_array_stream.ChunkedArray <- function(x, ..., schema = NULL) {
  stream <- nanoarrow::basic_array_stream(
    lapply(x$chunks, as_nanoarrow_array),
    schema = as_nanoarrow_schema(x$type),
    validate = FALSE
  )

  as_geoarrow_array_stream(stream, schema = schema)
}

#' @export
as_geoarrow_array_stream.Array <- function(x, ..., schema = NULL) {
  stream <- nanoarrow::basic_array_stream(
    list(as_nanoarrow_array(x)),
    schema = as_nanoarrow_schema(x$type),
    validate = FALSE
  )

  as_geoarrow_array_stream(stream, schema = schema)
}
