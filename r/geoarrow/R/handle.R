
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
