
geoarrow_kernel <- function(kernel_name, input_types, options = NULL) {
  if (!is.null(options)) {
    stop("options not yet supported")
  }

  kernel_name <- as.character(kernel_name)[1]
  input_types <- lapply(input_types, nanoarrow::as_nanoarrow_schema)
  options_raw <- serialize_kernel_options(options)
  schema_out <- nanoarrow::nanoarrow_allocate_schema()

  kernel <- .Call(geoarrow_c_kernel, kernel_name, input_types, options_raw, schema_out)

  structure(
    kernel,
    class = c(paste0("geoarrow_kernel_", kernel_name), "geoarrow_kernel"),
    input_types = input_types,
    options = options,
    output_type = schema_out,
    is_agg = endsWith(kernel_name, "_agg")
  )
}

geoarrow_kernel_push <- function(x) {

}



serialize_kernel_options <- function(vals) {
  vals <- vals[!vapply(vals, is.null, logical(1))]
  vals <- vapply(vals, as.character, logical(1))

  if (length(vals) == 0) {
    return(as.raw(c(0x00, 0x00, 0x00, 0x00)))
  }

  # When this matters we can wire up nanoarrow's serializer
  tmp <- tempfile()
  con <- file(tmp, open = "w+b")
  on.exit({close(con); unlink(tmp)})

  writeBin(length(vals), con, size = 4L)
  for (i in seq_along(vals)) {
    key <- charToRaw(enc2utf8(names(vals)[i]))
    value <- charToRaw(enc2utf8(vals[[i]]))

    writeBin(length(key), con, size = 4L)
    writeBin(key, con)
    writeBin(length(value), con, size = 4L)
    writeBin(value, con)
  }

  n_bytes <- seek(con)
  seek(con, 0L)
  readBin(con, what = raw(), n = n_bytes)
}
