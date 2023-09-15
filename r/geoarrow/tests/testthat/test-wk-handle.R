
test_that("wk handler works", {
  array <- as_geoarrow_array(wk::wkt("POINT (0 1)"))
  expect_null(geoarrow_handle_wk(data.frame(), wk::wk_void_handler()))
})
