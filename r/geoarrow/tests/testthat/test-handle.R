
test_that("handler works on geoarrow.point", {
  expect_identical(
    geoarrow_handle(wk::xy(0:1999, 1:2000), wk::xy_writer()),
    wk::xy(0:1999, 1:2000)
  )

  expect_identical(
    geoarrow_handle(wk::xyz(0, 1, 2), wk::xy_writer()),
    wk::xyz(0, 1, 2)
  )

  expect_identical(
    geoarrow_handle(wk::xym(0, 1, 3), wk::xy_writer()),
    wk::xym(0, 1, 3)
  )

  expect_identical(
    geoarrow_handle(wk::xyzm(0, 1, 2, 3), wk::xy_writer()),
    wk::xyzm(0, 1, 2, 3)
  )
})
