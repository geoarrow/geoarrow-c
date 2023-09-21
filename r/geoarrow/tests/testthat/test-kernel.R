
test_that("void kernel can be created", {
  kernel <- geoarrow_kernel("void", list(nanoarrow::na_na()))
  expect_s3_class(kernel, "geoarrow_kernel_void")
  expect_identical(attr(kernel, "is_agg"), FALSE)
  expect_identical(attr(kernel, "output_type")$format, "n")
})


