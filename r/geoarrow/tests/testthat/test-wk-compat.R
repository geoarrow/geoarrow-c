
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

test_that("as_geoarrow_array() for wkt() respects schema", {
  skip_if_not_installed("arrow")

  array <- as_geoarrow_array(
    wk::wkt(c("POINT (0 1)")),
    schema = na_extension_large_wkt()
  )
  schema <- infer_nanoarrow_schema(array)
  expect_identical(schema$format, "U")
})

test_that("as_geoarrow_array() for wkb() generates the correct buffers", {
  array <- as_geoarrow_array(wk::as_wkb(c("POINT (0 1)", NA)))
  schema <- infer_nanoarrow_schema(array)

  expect_identical(schema$format, "z")
  expect_identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.wkb")

  expect_identical(
    as.raw(array$buffers[[1]]),
    as.raw(nanoarrow::as_nanoarrow_array(c(TRUE, rep(FALSE, 7)))$buffers[[2]])
  )

  expect_identical(
    as.raw(array$buffers[[2]]),
    as.raw(nanoarrow::as_nanoarrow_buffer(c(0L, 21L, 21L)))
  )
})

test_that("as_geoarrow_array() for wkb() respects schema", {
  skip_if_not_installed("arrow")

  array <- as_geoarrow_array(
    wk::as_wkb(c("POINT (0 1)")),
    schema = na_extension_large_wkb()
  )
  schema <- infer_nanoarrow_schema(array)
  expect_identical(schema$format, "Z")
})

test_that("as_geoarrow_array() for xy() generates the correct buffers", {
  array <- as_geoarrow_array(wk::xy(1:5, 6:10))
  schema <- infer_nanoarrow_schema(array)

  expect_identical(schema$format, "+s")
  expect_identical(schema$metadata[["ARROW:extension:name"]], "geoarrow.point")

  expect_identical(
    as.raw(array$children[[1]]$buffers[[2]]),
    as.raw(nanoarrow::as_nanoarrow_buffer(as.double(1:5)))
  )

  expect_identical(
    as.raw(array$children[[2]]$buffers[[2]]),
    as.raw(nanoarrow::as_nanoarrow_buffer(as.double(6:10)))
  )
})

test_that("as_geoarrow_array() for wk generates the correct metadata", {
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
