
#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

#include "wkx_testing.hpp"

class TypeParameterizedTestFixture : public ::testing::TestWithParam<enum GeoArrowType> {
 protected:
  enum GeoArrowType type;
};

TEST_P(TypeParameterizedTestFixture, BuilderTestInit) {
  struct GeoArrowBuilder builder;
  struct ArrowSchema schema;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
  EXPECT_EQ(builder.view.schema_view.type, type);
  GeoArrowBuilderReset(&builder);

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderInitFromSchema(&builder, &schema, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);
  schema.release(&schema);
}

TEST_P(TypeParameterizedTestFixture, BuilderTestEmpty) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;
  array_out.release = nullptr;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowBuilderInitFromType(&builder, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  EXPECT_NE(array_out.release, nullptr);
  GeoArrowBuilderReset(&builder);

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

TEST(BuilderTest, BuilerTestSetBuffersPoint) {
  struct GeoArrowBuilder builder;
  struct ArrowArray array_out;

  // Build the array for [POINT (30 10), null, null]
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<double> xs = {30, 0, 0};
  std::vector<double> ys = {10, 0, 0};

  ASSERT_EQ(GeoArrowBuilderInitFromType(&builder, GEOARROW_TYPE_POINT), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 0, MakeBufferView(is_valid)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 1, MakeBufferView(xs)), GEOARROW_OK);
  EXPECT_EQ(GeoArrowBuilderAppendBuffer(&builder, 2, MakeBufferView(ys)), GEOARROW_OK);

  EXPECT_EQ(GeoArrowBuilderFinish(&builder, &array_out, nullptr), GEOARROW_OK);
  GeoArrowBuilderReset(&builder);

  EXPECT_EQ(array_out.length, 3);
  EXPECT_EQ(array_out.children[0]->length, 3);
  EXPECT_EQ(array_out.children[1]->length, 3);

  struct GeoArrowArrayView array_view;
  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);

  WKXTester tester;
  EXPECT_EQ(GeoArrowArrayViewVisit(&array_view, 0, array_out.length, tester.WKTVisitor()),
            GEOARROW_OK);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 3);
  EXPECT_EQ(values[0], "POINT (30 10)");
  EXPECT_EQ(values[1], "<null value>");
  EXPECT_EQ(values[2], "<null value>");

  array_out.release(&array_out);
}
