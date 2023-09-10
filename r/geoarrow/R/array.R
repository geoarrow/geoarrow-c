
geoarrow_array_from_buffers <- function(schema, buffers, validity = NULL) {
  extension_name <- schema$metadata[["ARROW:extension:name"]]
  if (is.null(extension_name)) {
    stop("Expected extension name")
  }

  switch(
    extension_name,
    "geoarrow.wkt",
    "geoarrow.wkb" = binary_array_from_buffers(
      schema,
      offsets = buffers[[1]],
      data = buffers[[2]],
      validity = validity
    )
  )
}

binary_array_from_buffers <- function(schema, offsets, data, validity = NULL) {
  is_large <- schema$format %in% c("U", "Z")

  validity <- as_validity_buffer(validity)
  buffers <- list(
    validity$buffer,
    if (is_large) as_large_offset_buffer(offsets) else as_offset_buffer(offsets),
    as_binary_buffer(data)
  )

  array <- nanoarrow::nanoarrow_array_init(schema)
  if (buffers[[2]]$size_bytes == 0) {
    return(array)
  }

  offset_element_size <- if (is_large) 8L else 4L

  nanoarrow::nanoarrow_array_modify(
    array,
    list(
      length = (buffers[[2]]$size_bytes %/% offset_element_size) - 1L,
      null_count = validity$null_count,
      buffers = buffers
    )
  )
}

point_array_from_buffers <- function(schema, x, y = NULL, z_or_m = NULL, m = NULL,
                                     validity = NULL) {
  validity <- as_validity_buffer(validity)
  ordinate_buffers <- list(
    as_coord_buffer(x),
    if (!is.null(y)) as_coord_buffer(y),
    if (!is.null(z_or_m)) as_coord_buffer(z_or_m),
    if (!is.null(m)) as_coord_buffer(m)
  )
  ordinate_buffers <- ordinate_buffers[!vapply(ordinate_buffers, is.null, logical(1))]

  array <- nanoarrow::nanoarrow_array_init(schema)
  children <- Map(
    function(child, buffer) {
      nanoarrow::nanoarrow_array_modify(
        child,
        list(
          length = buffer$size_bytes %/% 8L,
          buffers = list(NULL, buffer)
        )
      )
    },
    array$children,
    ordinate_buffers
  )

  if (schema$format == "+s") {
    len_factor <- 1L
  } else {
    parsed <- nanoarrow::nanoarrow_schema_parse(schema)
    len_factor <- parsed$fixed_size
  }

  nanoarrow::nanoarrow_array_modify(
    array,
    list(
      length = ordinate_buffers[[1]]$size_bytes %/% 8L %/% len_factor,
      null_count = validity$null_count,
      buffers = list(validity$buffer),
      children = children
    )
  )
}

as_binary_buffer <- function(x) {
  if (inherits(x, "nanoarrow_buffer")) {
    x
  } else if (is.raw(x)) {
    nanoarrow::as_nanoarrow_buffer(x)
  } else if (is.character(x)) {
    nanoarrow::as_nanoarrow_buffer(paste(x, collapse = ""))
  } else if (is.list(x)) {
    as_binary_buffer(unlist(x))
  } else {
    stop(
      sprintf(
        "Don't know how to create binary data buffer from object of class %s",
        class(x)[1]
      )
    )
  }
}

as_coord_buffer <- function(x) {
  if (inherits(x, "nanoarrow_buffer")) {
    return(x)
  }

  nanoarrow::as_nanoarrow_buffer(as.double(x))
}

as_offset_buffer <- function(x) {
  if (inherits(x, "nanoarrow_buffer")) {
    return(x)
  }

  nanoarrow::as_nanoarrow_buffer(as.integer(x))
}

as_large_offset_buffer <- function(x) {
  if (inherits(x, "nanoarrow_buffer")) {
    return(x)
  }

  array <- nanoarrow::as_nanoarrow_array(x, schema = nanoarrow::na_int64())
  array$buffers[[2]]
}

as_validity_buffer <- function(x) {
  if (is.null(x)) {
    return(list(null_count = 0, buffer = NULL))
  }

  if (inherits(x, "nanoarrow_buffer")) {
    return(list(null_count = -1, buffer = x))
  }

  if (is.logical(x)) {
    null_count <- sum(!x)
  } else {
    null_count <- sum(x == 0)
  }

  array <- nanoarrow::as_nanoarrow_array(x, schema = nanoarrow::na_bool())
  if (array$null_count > 0) {
    stop("NA values are not allowed in validity buffer")
  }

  list(null_count == null_count, buffer = x$buffers[[2]])
}
