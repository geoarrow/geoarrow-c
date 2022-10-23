#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(SchemaViewTest, SchemaViewTestInitFromUninitialized) {
  struct GeoArrowSchemaView schema_view;

  ASSERT_EQ(GeoArrowSchemaViewInitFromType(&schema_view, GEOARROW_TYPE_UNINITIALIZED),
            GEOARROW_OK);
}

TEST(SchemaViewTest, SchemaViewTestInitFromNoExtension) {
  struct ArrowSchema schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POINT), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected extension type");

  schema.release(&schema);
}

TEST(SchemaViewTest, SchemaViewTestInitFromBadExtension) {
  struct ArrowSchema schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POINT), GEOARROW_OK);

  struct ArrowBuffer metadata;
  ASSERT_EQ(ArrowMetadataBuilderInit(&metadata, nullptr), GEOARROW_OK);
  ASSERT_EQ(ArrowMetadataBuilderAppend(&metadata, ArrowCharView("ARROW:extension:name"),
                                       ArrowCharView("geoarrow.bogus")),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&schema, (const char*)metadata.data), NANOARROW_OK);
  ArrowBufferReset(&metadata);

  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, &error), EINVAL);
  EXPECT_STREQ(error.message, "Unrecognized GeoArrow extension name: 'geoarrow.bogus'");

  schema.release(&schema);
}

class TypeParameterizedTestFixture : public ::testing::TestWithParam<enum GeoArrowType> {
 protected:
  enum GeoArrowType type;
};

TEST_P(TypeParameterizedTestFixture, SchemaViewTestInitFromType) {
  enum GeoArrowType type = GetParam();
  struct GeoArrowSchemaView schema_view;
  EXPECT_EQ(GeoArrowSchemaViewInitFromType(&schema_view, type), GEOARROW_OK);
  EXPECT_EQ(schema_view.type, type);
}

TEST_P(TypeParameterizedTestFixture, SchemaViewTestInitFromSchema) {
  enum GeoArrowType type = GetParam();
  struct ArrowSchema schema;
  struct GeoArrowSchemaView schema_view;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, type), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, nullptr), GEOARROW_OK);
  EXPECT_EQ(schema_view.type, type);

  schema.release(&schema);
}

INSTANTIATE_TEST_SUITE_P(
    SchemaViewTest, TypeParameterizedTestFixture,
    ::testing::Values(GEOARROW_TYPE_WKB, GEOARROW_TYPE_LARGE_WKB,

                      GEOARROW_TYPE_POINT, GEOARROW_TYPE_LINESTRING,
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

TEST(SchemaViewTest, SchemaViewTestInitInvalidPoint) {
  struct ArrowSchema good_schema;
  struct ArrowSchema bad_schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_POINT), GEOARROW_OK);

  // Bad storage type
  ASSERT_EQ(ArrowSchemaInit(&bad_schema, NANOARROW_TYPE_INT32), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Expected storage type struct for coord array for extension 'geoarrow.point'");
  bad_schema.release(&bad_schema);

  // Bad number of children
  ASSERT_EQ(ArrowSchemaInit(&bad_schema, NANOARROW_TYPE_STRUCT), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected 2, 3, or 4 children for coord array for extension "
               "'geoarrow.point' but got 0");
  bad_schema.release(&bad_schema);

  // Bad child name
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[1], "not_single_char"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected coordinate child 1 to have single character name for extension "
               "'geoarrow.point'");
  bad_schema.release(&bad_schema);

  // Bad child type
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  bad_schema.children[1]->release(bad_schema.children[1]);
  ASSERT_EQ(ArrowSchemaInit(bad_schema.children[1], NANOARROW_TYPE_INT32), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[1], "y"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected coordinate child 1 to have storage type of double for extension "
               "'geoarrow.point'");
  bad_schema.release(&bad_schema);

  // Bad name combination
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[1], "z"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected dimensions 'xy', 'xyz', 'xym', or 'xyzm' for extension "
               "'geoarrow.point' but found 'xz'");
  bad_schema.release(&bad_schema);

  good_schema.release(&good_schema);
}

TEST(SchemaViewTest, SchemaViewTestInitInvalidNested) {
  struct ArrowSchema good_schema;
  struct ArrowSchema bad_schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_LINESTRING), GEOARROW_OK);

  ASSERT_EQ(ArrowSchemaInit(&bad_schema, NANOARROW_TYPE_INT32), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Expected valid list type for coord parent 1 for extension 'geoarrow.linestring'");
  bad_schema.release(&bad_schema);

  good_schema.release(&good_schema);
}

TEST(SchemaViewTest, SchemaViewTestInitInvalidWKB) {
  struct ArrowSchema good_schema;
  struct ArrowSchema bad_schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_WKB), GEOARROW_OK);

  ASSERT_EQ(ArrowSchemaInit(&bad_schema, NANOARROW_TYPE_INT32), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Expected storage type of binary or large_binary for extension 'geoarrow.wkb'");
  bad_schema.release(&bad_schema);

  good_schema.release(&good_schema);
}
