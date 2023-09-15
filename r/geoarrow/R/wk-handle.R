
geoarrow_handle_wk <- function(x, handler, size = NA_integer_) {
  if (inherits(x, "nanoarrow_array")) {
    size <- x$length
    stream <- nanoarrow::basic_array_stream(
      list(x),
      schema = nanoarrow::infer_nanoarrow_schema(x),
      validate = FALSE
    )

  } else if (!inherits(x, "nanoarrow_array_stream")) {
    stream <- nanoarrow::as_nanoarrow_array_stream(x)
  }

  data <- list(
    stream,
    stream$get_schema(),
    nanoarrow::nanoarrow_allocate_array(),
    size
  )

  .Call(geoarrow_c_handle_stream, data, wk::as_wk_handler(handler))
}
