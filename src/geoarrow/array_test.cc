
#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

#include "wkx_testing.hpp"

class TypeParameterizedTestFixture : public ::testing::TestWithParam<enum GeoArrowType> {
 protected:
  enum GeoArrowType type;
};

TEST_P(TypeParameterizedTestFixture, ArrayTestInit) {
  struct GeoArrowArray array;
  struct ArrowSchema schema;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowArrayInitFromType(&array, type), GEOARROW_OK);
  EXPECT_EQ(array.schema_view.type, type);
  GeoArrowArrayReset(&array);

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  GeoArrowArrayReset(&array);
  schema.release(&schema);
}

TEST_P(TypeParameterizedTestFixture, ArrayTestEmpty) {
  struct GeoArrowArray array;
  struct ArrowArray array_out;
  array_out.release = nullptr;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowArrayInitFromType(&array, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayFinish(&array, &array_out, nullptr), GEOARROW_OK);
  EXPECT_NE(array_out.release, nullptr);
  EXPECT_EQ(array.array.release, nullptr);
  GeoArrowArrayReset(&array);

  // Make sure this is a valid zero-length array
  struct ArrowArrayView array_view;
  struct ArrowSchema schema;
  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  EXPECT_EQ(ArrowArrayViewInitFromSchema(&array_view, &schema, nullptr), GEOARROW_OK);

  ArrowArrayViewReset(&array_view);
  array_out.release(&array_out);
}

INSTANTIATE_TEST_SUITE_P(
    ArrayTest, TypeParameterizedTestFixture,
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

TEST(ArrayTest, ArrayTestSetBuffersPoint) {
  struct GeoArrowArray array;
  struct ArrowArray array_out;

  struct GeoArrowBufferView b;
  std::vector<uint8_t> is_valid = {0b00000001};
  std::vector<double> xs = {30, 0, 0};
  std::vector<double> ys = {10, 0, 0};

  ASSERT_EQ(GeoArrowArrayInitFromType(&array, GEOARROW_TYPE_POINT), GEOARROW_OK);
  b.data = is_valid.data();
  b.n_bytes = is_valid.size() * sizeof(uint8_t);
  EXPECT_EQ(GeoArrowArraySetBufferCopy(&array, 0, b), GEOARROW_OK);

  b.data = (const uint8_t*)xs.data();
  b.n_bytes = xs.size() * sizeof(double);
  EXPECT_EQ(GeoArrowArraySetBufferCopy(&array, 1, b), GEOARROW_OK);

  b.data = (const uint8_t*)ys.data();
  b.n_bytes = ys.size() * sizeof(double);
  EXPECT_EQ(GeoArrowArraySetBufferCopy(&array, 2, b), GEOARROW_OK);

  // Should find a better way to set these lengths
  array.array.length = 3;
  array.array.children[0]->length = 3;
  array.array.children[1]->length = 3;

  struct GeoArrowError error;
  EXPECT_EQ(GeoArrowArrayFinish(&array, &array_out, &error), GEOARROW_OK);
  GeoArrowArrayReset(&array);

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

  array_out.release(&array_out);
}
