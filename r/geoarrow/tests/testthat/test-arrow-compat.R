
test_that("as_chunked_array() works for geoarrow_vctr", {
  skip_if_not_installed("arrow")

  vctr <- as_geoarrow_vctr("POINT (0 1)")
  chunked <- arrow::as_chunked_array(vctr)
  expect_s3_class(chunked, "ChunkedArray")
  expect_equal(chunked$length(), 1)

  # Check with a requested type
  chunked <- arrow::as_chunked_array(vctr, type = na_extension_wkb())
  expect_equal(chunked$length(), 1)
})

test_that("as_arrow_array() works for geoarrow_vctr", {
  skip_if_not_installed("arrow")

  vctr <- as_geoarrow_vctr("POINT (0 1)")
  array <- arrow::as_arrow_array(vctr)
  expect_s3_class(array, "Array")
  expect_equal(array$length(), 1)

  vctr2 <- new_geoarrow_vctr(
    list(
      as_geoarrow_array("POINT (0 1)"),
      as_geoarrow_array("POINT (1 2)")
    ),
    schema = na_extension_wkt()
  )

  array <- arrow::as_arrow_array(vctr2)
  expect_s3_class(array, "Array")
  expect_equal(array$length(), 2)

  # Check with a requested type
  chunked <- arrow::as_arrow_array(vctr2, type = na_extension_wkb())
})
