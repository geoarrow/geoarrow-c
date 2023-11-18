
test_that("as_nanoarrow_array_stream() generates an empty stream for empty slice", {
  vctr <- new_geoarrow_vctr(list(), na_extension_wkt())
  stream <- nanoarrow::as_nanoarrow_array_stream(vctr)
  schema_out <- stream$get_schema()
  expect_identical(schema_out$format, "u")
  expect_identical(nanoarrow::collect_array_stream(stream), list())
})

test_that("as_nanoarrow_array_stream() generates identical stream for identity slice", {
  array <- as_geoarrow_array("POINT (0 1)")
  vctr <- new_geoarrow_vctr(list(array), infer_nanoarrow_schema(array))

  stream <- nanoarrow::as_nanoarrow_array_stream(vctr)
  schema_out <- stream$get_schema()
  expect_identical(schema_out$format, "u")

  collected <- nanoarrow::collect_array_stream(stream)
  expect_length(collected, 1)
  expect_identical(
    nanoarrow::convert_buffer(array$buffers[[3]]),
    "POINT (0 1)"
  )
})



test_that("slice detector works", {
  expect_identical(
    vctr_as_slice(logical()),
    NULL
  )

  expect_identical(
    vctr_as_slice(2:1),
    NULL
  )

  expect_identical(
    vctr_as_slice(integer()),
    c(NA_integer_, 0L)
  )

  expect_identical(
    vctr_as_slice(2L),
    c(2L, 1L)
  )

  expect_identical(
    vctr_as_slice(1:10),
    c(1L, 10L)
  )

  expect_identical(
    vctr_as_slice(10:2048),
    c(10L, (2048L - 10L + 1L))
  )
})

test_that("chunk resolver works", {
  chunk_offset1 <- 0:10

  expect_identical(
    vctr_resolve_chunk(c(-1L, 10L), chunk_offset1),
    c(NA_integer_, NA_integer_)
  )

  expect_identical(
    vctr_resolve_chunk(9:0, chunk_offset1),
    9:0
  )
})
