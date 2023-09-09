
test_that("wk handler works", {
  expect_null(geoarrow_handle_wk(data.frame(), wk::wk_void_handler()))
})
