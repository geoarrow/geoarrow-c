
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
`[.geoarrow_vctr` <- function(x, i) {
  attrs <- attributes(x)
  x <- NextMethod()
  # Assert slice?
  attributes(x) <- attrs
  x
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
  first_index <- slice[1] - 1L
  end_index <- first_index + slice[2]
  last_index <- end_index - 1L
  first_chunk_index <- vctr_resolve_chunk(first_index, offsets)
  last_chunk_index <- vctr_resolve_chunk(last_index, offsets)

  first_chunk_offset <- first_index - offsets[first_chunk_index + 1L]
  first_chunk_length <- offsets[first_chunk_index + 2L] - first_index
  last_chunk_offset <- 0L
  last_chunk_length <- end_index - offsets[last_chunk_index + 1L]

  # Calculate first and last slices
  if (first_chunk_index == last_chunk_index) {
    batch <- nanoarrow::nanoarrow_array_modify(
      batches[[first_chunk_index + 1L]],
      list(
        offset = first_chunk_offset,
        length = last_chunk_length - first_chunk_offset
      )
    )

    return(
      nanoarrow::basic_array_stream(
        list(batch),
        schema = x_schema,
        validate = FALSE
      )
    )
  }

  batch1 <- nanoarrow::nanoarrow_array_modify(
    batches[[first_chunk_index + 1L]],
    list(
      offset = first_chunk_offset,
      length = first_chunk_length
    )
  )

  batchn <- nanoarrow::nanoarrow_array_modify(
    batches[[last_chunk_index + 1L]],
    list(
      offset = last_chunk_offset,
      length = last_chunk_length
    )
  )

  seq_mid <- seq_len(last_chunk_index - first_chunk_index - 1)
  batch_mid <- batches[first_chunk_index + seq_mid]

  nanoarrow::basic_array_stream(
    c(
      list(batch1),
      batch_mid,
      list(batchn)
    ),
    schema = x_schema,
    validate = FALSE
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
