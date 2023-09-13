
test_that("geoarrow_array_from_buffers() works for wkb", {
  wkb <- wk::as_wkb("POINT (0 1)")
  array <- geoarrow_array_from_buffers(
    na_extension_wkb(),
    list(
      NULL,
      c(0L, cumsum(lengths(unclass(wkb)))),
      wkb
    )
  )
  vctr <- suppressWarnings(nanoarrow::convert_array(array))
  expect_identical(unclass(wkb), as.list(vctr))
})

test_that("geoarrow_array_from_buffers() works for large wkb", {
  wkb <- wk::as_wkb("POINT (0 1)")
  array <- geoarrow_array_from_buffers(
    na_extension_large_wkb(),
    list(
      NULL,
      c(0L, cumsum(lengths(unclass(wkb)))),
      wkb
    )
  )
  vctr <- suppressWarnings(nanoarrow::convert_array(array))
  expect_identical(unclass(wkb), as.list(vctr))
})

test_that("geoarrow_array_from_buffers() works for wkt", {
  wkt <- "POINT (0 1)"
  array <- geoarrow_array_from_buffers(
    na_extension_wkt(),
    list(
      NULL,
      c(0L, cumsum(nchar(wkt))),
      wkt
    )
  )
  vctr <- suppressWarnings(nanoarrow::convert_array(array, character()))
  expect_identical(wkt, vctr)
})

test_that("geoarrow_array_from_buffers() works for large wkt", {
  wkt <- "POINT (0 1)"
  array <- geoarrow_array_from_buffers(
    na_extension_large_wkt(),
    list(
      NULL,
      c(0L, cumsum(nchar(wkt))),
      wkt
    )
  )
  vctr <- suppressWarnings(nanoarrow::convert_array(array, character()))
  expect_identical(wkt, vctr)
})
