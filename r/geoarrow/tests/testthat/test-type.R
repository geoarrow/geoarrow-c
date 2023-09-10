
test_that("serialized extensions work", {

})

test_that("enum matcher works", {
  expect_identical(
    enum_value(c("GEOMETRY", "MULTIPOINT", "NOT VALID"), "GeometryType"),
    c(0L, 4L, NA_integer_)
  )

  expect_identical(
    enum_value(c(0L, 4L, 9L), "GeometryType"),
    c(0L, 4L, NA_integer_)
  )
})

test_that("enum labeller works", {
  expect_identical(
    enum_label(c("GEOMETRY", "MULTIPOINT", "NOT VALID"), "GeometryType"),
    c("GEOMETRY", "MULTIPOINT", NA_character_)
  )

  expect_identical(
    enum_label(c(0L, 4L, 9L), "GeometryType"),
    c("GEOMETRY", "MULTIPOINT", NA_character_)
  )
})

test_that("enum_scalar matcher errors for bad values", {
  expect_error(
    enum_value_scalar("NOT VALID", "GeometryType"),
    "NOT VALID is not a valid enum label or value for GeometryType"
  )

  expect_error(
    enum_value_scalar(10, "GeometryType"),
    "10 is not a valid enum label or value for GeometryType"
  )
})
