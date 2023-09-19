
geoarrow_handle <- function(x, handler, size = NA_integer_) {
  stream <- as_geoarrow_array_stream(x)
  handler <- wk::as_wk_handler(handler)

  data <- list(
    stream,
    stream$get_schema(),
    nanoarrow::nanoarrow_allocate_array(),
    size
  )

  .Call(geoarrow_c_handle_stream, data, handler)
}

geoarrow_writer <- function(schema) {
  array_out <- nanoarrow::nanoarrow_allocate_array()
  schema <- nanoarrow::as_nanoarrow_schema(schema)
  nanoarrow::nanoarrow_array_set_schema(array_out, schema, validate = FALSE)
  wk::new_wk_handler(
    .Call(geoarrow_c_writer_new, schema, array_out),
    "geoarrow_writer"
  )
}
