
#include <gtest/gtest.h>

#include "nanoarrow.h"
#include "geoarrow.h"

// Such that kNumOffsets[geometry_type] gives the right answer
static int kNumOffsets[] = {-1, 0, 1, 2, 1, 2, 3, -1};

// Such that kNumDimensions[dimensions] gives the right answer
static int kNumDimensions[] = {-1, 2, 3, 3, 4};

class TypeParameterizedTestFixture : public ::testing::TestWithParam<enum GeoArrowType> {
 protected:
  enum GeoArrowType type;
};

TEST_P(TypeParameterizedTestFixture, ArrayViewTestInitType) {
  struct GeoArrowArrayView array_view;
  enum GeoArrowType type = GetParam();

  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(array_view.schema_view.type, type);
  EXPECT_EQ(array_view.length, 0);
  EXPECT_EQ(array_view.validity_bitmap, nullptr);
  EXPECT_EQ(array_view.n_offsets, kNumOffsets[array_view.schema_view.geometry_type]);
  EXPECT_EQ(array_view.coords.n_coords, 0);
  EXPECT_EQ(array_view.coords.n_values,
            kNumDimensions[array_view.schema_view.dimensions]);

  if (array_view.schema_view.coord_type == GEOARROW_COORD_TYPE_SEPARATE) {
    EXPECT_EQ(array_view.coords.coords_stride, 1);
  } else {
    EXPECT_EQ(array_view.coords.coords_stride,
              kNumDimensions[array_view.schema_view.dimensions]);
  }
}

TEST_P(TypeParameterizedTestFixture, ArrayViewTestInitSchema) {
  struct GeoArrowArrayView array_view;
  struct ArrowSchema schema;
  enum GeoArrowType type = GetParam();

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewInitFromSchema(&array_view, &schema, nullptr), GEOARROW_OK);
  EXPECT_EQ(array_view.schema_view.type, type);

  schema.release(&schema);
}

TEST_P(TypeParameterizedTestFixture, ArrayViewTestInitEmptyArray) {
  struct GeoArrowArrayView array_view;
  struct ArrowSchema schema;
  struct ArrowArray array;
  enum GeoArrowType type = GetParam();

  ASSERT_EQ(GeoArrowSchemaInit(&schema, type), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayInitFromSchema(&array, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayStartAppending(&array), GEOARROW_OK);
  ASSERT_EQ(ArrowArrayFinishBuilding(&array, nullptr), GEOARROW_OK);

  EXPECT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

  schema.release(&schema);
  array.release(&array);
}

INSTANTIATE_TEST_SUITE_P(
    ArrayViewTest, TypeParameterizedTestFixture,
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
