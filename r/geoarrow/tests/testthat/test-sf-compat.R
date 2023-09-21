
test_that("infer_nanoarrow_schema() works for mixed sfc objects", {
  skip_if_not_installed("sf")

  sfc <- sf::st_sfc(
    sf::st_point(),
    sf::st_linestring()
  )
  sf::st_crs(sfc) <- "OGC:CRS84"

  schema <- infer_nanoarrow_schema(sfc)
  parsed <- geoarrow_schema_parse(schema)
  expect_identical(parsed$id, enum$Type$WKB)
  expect_identical(parsed$crs_type, enum$CrsType$PROJJSON)
})

test_that("infer_nanoarrow_schema() works for single-geometry type sfc objects", {
  skip_if_not_installed("sf")

  sfc <- sf::st_sfc(
    sf::st_point(),
    sf::st_point()
  )
  sf::st_crs(sfc) <- "OGC:CRS84"

  schema <- infer_nanoarrow_schema(sfc)
  parsed <- geoarrow_schema_parse(schema)
  expect_identical(parsed$geometry_type, enum$GeometryType$POINT)
  expect_identical(parsed$dimensions, enum$Dimensions$XY)
  expect_identical(parsed$crs_type, enum$CrsType$PROJJSON)
})

test_that("infer_nanoarrow_schema() works for single-geometry type sfc objects (Z)", {
  skip_if_not_installed("sf")

  sfc <- sf::st_sfc(
    sf::st_point(c(1, 2, 3)),
    sf::st_point(c(1, 2, 3))
  )
  sf::st_crs(sfc) <- "OGC:CRS84"

  schema <- infer_nanoarrow_schema(sfc)
  parsed <- geoarrow_schema_parse(schema)
  expect_identical(parsed$geometry_type, enum$GeometryType$POINT)
  expect_identical(parsed$dimensions, enum$Dimensions$XYZ)
  expect_identical(parsed$crs_type, enum$CrsType$PROJJSON)
})

test_that("infer_nanoarrow_schema() works for single-geometry type sfc objects (M)", {
  skip_if_not_installed("sf")

  sfc <- sf::st_sfc(
    sf::st_point(c(1, 2, 3), dim = "XYM"),
    sf::st_point(c(1, 2, 3), dim = "XYM")
  )
  sf::st_crs(sfc) <- "OGC:CRS84"

  schema <- infer_nanoarrow_schema(sfc)
  parsed <- geoarrow_schema_parse(schema)
  expect_identical(parsed$geometry_type, enum$GeometryType$POINT)
  expect_identical(parsed$dimensions, enum$Dimensions$XYM)
  expect_identical(parsed$crs_type, enum$CrsType$PROJJSON)
})

test_that("infer_nanoarrow_schema() works for single-geometry type sfc objects (ZM)", {
  skip_if_not_installed("sf")

  sfc <- sf::st_sfc(
    sf::st_point(c(1, 2, 3, 4)),
    sf::st_point(c(1, 2, 3, 4))
  )
  sf::st_crs(sfc) <- "OGC:CRS84"

  schema <- infer_nanoarrow_schema(sfc)
  parsed <- geoarrow_schema_parse(schema)
  expect_identical(parsed$geometry_type, enum$GeometryType$POINT)
  expect_identical(parsed$dimensions, enum$Dimensions$XYZM)
  expect_identical(parsed$crs_type, enum$CrsType$PROJJSON)
})
