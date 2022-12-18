
#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

#include "wkx_testing.hpp"

class TypeParameterizedTestFixture : public ::testing::TestWithParam<enum GeoArrowType> {
 protected:
  enum GeoArrowType type;
};

TEST_P(TypeParameterizedTestFixture, BuilderTestInit) {
  struct GeoArrowBuilder array;
  struct ArrowSchema schema;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowBuilderInitFromType(&array, type), GEOARROW_OK);
  EXPECT_EQ(array.view.schema_view.type, type);
  GeoArrowBuilderReset(&array);

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&array);
  schema.release(&schema);
}

TEST_P(TypeParameterizedTestFixture, BuilderTestEmpty) {
  struct GeoArrowBuilder array;
  struct ArrowArray array_out;
  array_out.release = nullptr;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowBuilderInitFromType(&array, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderFinish(&array, &array_out, nullptr), GEOARROW_OK);
  EXPECT_NE(array_out.release, nullptr);
  GeoArrowBuilderReset(&array);

  // Make sure this is a valid zero-length array
  struct ArrowArrayView array_view;
  struct ArrowSchema schema;
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  EXPECT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema, nullptr), GEOARROW_OK);

  ArrowArrayViewReset(&array_view);
  array_out.release(&array_out);
  schema.release(&schema);
}

INSTANTIATE_TEST_SUITE_P(
    BuilderTest, TypeParameterizedTestFixture,
    ::testing::Values(GEOARROW_TYPE_POINT, GEOARROW_TYPE_LINESTRING,
                      GEOARROW_TYPE_POLYGON, GEOARROW_TYPE_MULTIPOINT,
                      GEOARROW_TYPE_MULTILINESTRING, GEOARROW_TYPE_MULTIPOLYGON,

                      GEOARROW_TYPE_POINT_Z, GEOARROW_TYPE_LINESTRING_Z,
                      GEOARROW_TYPE_POLYGON_Z, GEOARROW_TYPE_MULTIPOINT_Z,
                      GEOARROW_TYPE_MULTILINESTRING_Z, GEOARROW_TYPE_MULTIPOLYGON_Z,

                      GEOARROW_TYPE_POINT_M, GEOARROW_TYPE_LINESTRING_M,
                      GEOARROW_TYPE_POLYGON_M, GEOARROW_TYPE_MULTIPOINT_M,
                      GEOARROW_TYPE_MULTILINESTRING_M, GEOARROW_TYPE_MULTIPOLYGON_M,

                      GEOARROW_TYPE_POINT_ZM, GEOARROW_TYPE_LINESTRING_ZM,
                      GEOARROW_TYPE_POLYGON_ZM, GEOARROW_TYPE_MULTIPOINT_ZM,
                      GEOARROW_TYPE_MULTILINESTRING_ZM, GEOARROW_TYPE_MULTIPOLYGON_ZM));
