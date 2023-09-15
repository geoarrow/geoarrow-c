
test_that("as_geoarrow_array() for wkt() generates the correct buffers", {
  array <- as_geoarrow_array(wk::wkt(c("POINT (0 1)", NA)))
  schema <- infer_nanoarrow_schema(array)

  expect_identical(schema$format, "u")
  expect_identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.wkt")

  expect_identical(
    as.raw(array$buffers[[1]]),
    as.raw(nanoarrow::as_nanoarrow_array(c(TRUE, rep(FALSE, 7)))$buffers[[2]])
  )

  expect_identical(
    as.raw(array$buffers[[2]]),
    as.raw(nanoarrow::as_nanoarrow_buffer(c(0L, 11L, 11L)))
  )

  expect_identical(
    as.raw(array$buffers[[3]]),
    as.raw(nanoarrow::as_nanoarrow_buffer("POINT (0 1)"))
  )
})

test_that("as_geoarrow_array() for wkt() generates the correct metadata", {
  array <- as_geoarrow_array(wk::wkt(c("POINT (0 1)", NA)))
  schema <- infer_nanoarrow_schema(array)
  expect_identical(schema$format, "u")
  expect_identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.wkt")
  expect_identical(schema$metadata[["ARROW:extension:metadata"]], "{}")

  array <- as_geoarrow_array(wk::wkt(c("POINT (0 1)", NA), crs = "OGC:CRS84"))
  schema <- infer_nanoarrow_schema(array)
  expect_identical(schema$format, "u")
  expect_identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.wkt")
  expect_identical(
    schema$metadata[["ARROW:extension:metadata"]],
    sprintf('{"crs":%s}', wk::wk_crs_projjson("OGC:CRS84"))
  )

  array <- as_geoarrow_array(wk::wkt(c("POINT (0 1)", NA), geodesic = TRUE))
  schema <- infer_nanoarrow_schema(array)
  expect_identical(schema$format, "u")
  expect_identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.wkt")
  expect_identical(
    schema$metadata[["ARROW:extension:metadata"]],
    sprintf('{"edges":"spherical"}')
  )
})
