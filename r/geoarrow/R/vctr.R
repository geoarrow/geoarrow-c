
as_geoarrow_vctr <- function(x, ..., schema = NULL) {
  if (inherits(x, "geoarrow_vctr") && is.null(schema)) {
    return(x)
  }

  stream <- as_geoarrow_array_stream(x, ..., schema = schema)
  chunks <- nanoarrow::collect_array_stream(stream, validate = FALSE)
  new_geoarrow_vctr(chunks, stream$get_schema())
}

new_geoarrow_vctr <- function(chunks, schema) {
  offsets <- .Call(geoarrow_c_vctr_chunk_offsets, chunks)
  structure(
    seq_len(offsets[length(offsets)]),
    schema = schema,
    chunks = chunks,
    chunk_offsets = offsets,
    class = c("geoarrow_vctr", "wk_vctr")
  )
}

#' @export
infer_nanoarrow_schema.geoarrow_vctr <- function(x, ...) {
  attr(x, "schema")
}

#' @export
as_geoarrow_array_stream.geoarrow_vctr <- function(x, ..., schema = NULL) {
  as_nanoarrow_array_stream.geoarrow_vctr(x, ..., schema = NULL)
}

#' @export
as_nanoarrow_array_stream.geoarrow_vctr <- function(x, ..., schema = NULL) {
  slice <- vctr_as_slice(x)
  if (is.null(slice)) {
    stop("Can't resolve non-slice geoarrow_vctr to nanoarrow_array_stream")
  }

  chunk1

  nanoarrow::basic_array_stream(
    attr(x, "chunks")
  )
}


# Utilities for vctr methods

vctr_resolve_chunk <- function(x, offsets) {
  .Call(geoarrow_c_vctr_chunk_resolve, x, offsets)
}

vctr_as_slice <- function(x) {
  .Call(geoarrow_c_vctr_as_slice, x)
}

vctr_array_slice <- function(x, offset, length) {
  new_offset <- x$offset + offset
  new_length <- length
  nanoarrow::nanoarrow_array_modify(
    x,
    list(offset = new_offset, length = new_length),
    validate = FALSE
  )
}
