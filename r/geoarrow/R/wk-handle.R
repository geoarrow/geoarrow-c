
geoarrow_handle_wk <- function(x, handler, i = NULL) {
  if (inherits(x, "nanoarrow_array")) {
    x <- nanoarrow::basic_array_stream(
      list(x),
      schema = nanoarrow::infer_nanoarrow_schema(x),
      validate = FALSE
    )
  } else if (!inherits(x, "nanoarrow_array_stream")) {
    x <- nanoarrow::as_nanoarrow_array_stream(x)
  }

  .Call(geoarrow_c_handle_stream, x, wk::as_wk_handler(handler))
}

