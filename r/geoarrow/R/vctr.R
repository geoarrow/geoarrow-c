
as_geoarrow_vctr <- function(x, ..., schema = NULL) {
  if (inherits(x, "geoarrow_vctr") && is.null(schema)) {
    return(x)
  }

  stream <- as_geoarrow_array_stream(x, ..., schema = schema)
  chunks <- nanoarrow::collect_array_stream(stream, validate = FALSE)
  new_geoarrow_vctr(chunks, stream$get_schema())
}

new_geoarrow_vctr <- function(chunks, schema, indices = NULL) {
  offsets <- .Call(geoarrow_c_vctr_chunk_offsets, chunks)
  if (is.null(indices)) {
    indices <- seq_len(offsets[length(offsets)])
  }

  structure(
    indices,
    schema = schema,
    chunks = chunks,
    offsets = offsets,
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

#' @importFrom nanoarrow as_nanoarrow_array_stream
#' @export
as_nanoarrow_array_stream.geoarrow_vctr <- function(x, ..., schema = NULL) {
  slice <- vctr_as_slice(x)
  if (is.null(slice)) {
    stop("Can't resolve non-slice geoarrow_vctr to nanoarrow_array_stream")
  }

  x_schema <- attr(x, "schema")

  # Zero-size slice can be an array stream with zero batches
  if (slice[2] == 0) {
    return(nanoarrow::basic_array_stream(list(), schema = x_schema))
  }

  # Full slice doesn't need slicing logic
  offsets <- attr(x, "offsets")
  batches <- attr(x, "chunks")
  if (slice[1] == 0 && slice[2] == max(offsets)) {
    return(
      nanoarrow::basic_array_stream(
        batches,
        schema = x_schema,
        validate = FALSE
      )
    )
  }

  # Calculate first and last slice information
  first_index <- slice[1]
  last_index <- slice[1] + slice[2]
  first_chunk_index <- vctr_resolve_chunk(first_index, offsets)
  last_chunk_index <- vctr_resolve_chunk(last_index, offsets)

  first_chunk_offset <- first_index - offsets[first_chunk_index]
  first_chunk_length <- offsets[first_chunk_index + 1] - first_index
  last_chunk_offset <- 0
  last_chunk_length <- last_index - offsets[last_chunk_index]

  # Calculate first and last slices


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
