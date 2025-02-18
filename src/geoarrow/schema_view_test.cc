#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

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

TEST(SchemaViewTest, SchemaViewTestInitFromStorage) {
  struct ArrowSchema schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POINT), GEOARROW_OK);
  struct GeoArrowStringView ext;
  ext.data = "geoarrow.point";
  ext.size_bytes = 14;
  EXPECT_EQ(GeoArrowSchemaViewInitFromStorage(&schema_view, &schema, ext, &error),
            GEOARROW_OK);
  EXPECT_EQ(schema_view.type, GEOARROW_TYPE_POINT);

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
    ::testing::Values(
        GEOARROW_TYPE_WKB, GEOARROW_TYPE_LARGE_WKB, GEOARROW_TYPE_WKT,
        GEOARROW_TYPE_LARGE_WKT,

        GEOARROW_TYPE_BOX, GEOARROW_TYPE_BOX_Z, GEOARROW_TYPE_BOX_M, GEOARROW_TYPE_BOX_ZM,

        GEOARROW_TYPE_POINT, GEOARROW_TYPE_LINESTRING, GEOARROW_TYPE_POLYGON,
        GEOARROW_TYPE_MULTIPOINT, GEOARROW_TYPE_MULTILINESTRING,
        GEOARROW_TYPE_MULTIPOLYGON,

        GEOARROW_TYPE_POINT_Z, GEOARROW_TYPE_LINESTRING_Z, GEOARROW_TYPE_POLYGON_Z,
        GEOARROW_TYPE_MULTIPOINT_Z, GEOARROW_TYPE_MULTILINESTRING_Z,
        GEOARROW_TYPE_MULTIPOLYGON_Z,

        GEOARROW_TYPE_POINT_M, GEOARROW_TYPE_LINESTRING_M, GEOARROW_TYPE_POLYGON_M,
        GEOARROW_TYPE_MULTIPOINT_M, GEOARROW_TYPE_MULTILINESTRING_M,
        GEOARROW_TYPE_MULTIPOLYGON_M,

        GEOARROW_TYPE_POINT_ZM, GEOARROW_TYPE_LINESTRING_ZM, GEOARROW_TYPE_POLYGON_ZM,
        GEOARROW_TYPE_MULTIPOINT_ZM, GEOARROW_TYPE_MULTILINESTRING_ZM,
        GEOARROW_TYPE_MULTIPOLYGON_ZM,

        GEOARROW_TYPE_INTERLEAVED_POINT, GEOARROW_TYPE_INTERLEAVED_LINESTRING,
        GEOARROW_TYPE_INTERLEAVED_POLYGON, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING, GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON,
        GEOARROW_TYPE_INTERLEAVED_POINT_Z, GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z,
        GEOARROW_TYPE_INTERLEAVED_POLYGON_Z, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_Z,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_Z,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_Z, GEOARROW_TYPE_INTERLEAVED_POINT_M,
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_M, GEOARROW_TYPE_INTERLEAVED_POLYGON_M,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_M,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_M,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_M, GEOARROW_TYPE_INTERLEAVED_POINT_ZM,
        GEOARROW_TYPE_INTERLEAVED_LINESTRING_ZM, GEOARROW_TYPE_INTERLEAVED_POLYGON_ZM,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_ZM,
        GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_ZM,
        GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_ZM));

TEST(SchemaViewTest, SchemaViewTestInitInterleavedGuessDims) {
  struct ArrowSchema good_schema;
  struct ArrowSchema good_schema2;
  struct GeoArrowSchemaView schema_view;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_INTERLEAVED_POINT),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &good_schema2), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(good_schema2.children[0], "item"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &good_schema2, nullptr), GEOARROW_OK);
  EXPECT_EQ(schema_view.dimensions, GEOARROW_DIMENSIONS_XY);
  good_schema2.release(&good_schema2);
  good_schema.release(&good_schema);

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_INTERLEAVED_POINT_Z),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &good_schema2), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(good_schema2.children[0], "item"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &good_schema2, nullptr), GEOARROW_OK);
  EXPECT_EQ(schema_view.dimensions, GEOARROW_DIMENSIONS_XYZ);
  good_schema2.release(&good_schema2);
  good_schema.release(&good_schema);

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_INTERLEAVED_POINT_ZM),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &good_schema2), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(good_schema2.children[0], "item"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &good_schema2, nullptr), GEOARROW_OK);
  EXPECT_EQ(schema_view.dimensions, GEOARROW_DIMENSIONS_XYZM);
  good_schema2.release(&good_schema2);
  good_schema.release(&good_schema);
}

TEST(SchemaViewTest, SchemaViewTestInitInvalidBox) {
  struct ArrowSchema good_schema;
  struct ArrowSchema bad_schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_BOX), GEOARROW_OK);

  // Bad storage type
  ASSERT_EQ(ArrowSchemaInitFromType(&bad_schema, NANOARROW_TYPE_INT32), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected struct storage for 'geoarrow.box'");
  ArrowSchemaRelease(&bad_schema);

  // Wrong number of children
  ASSERT_EQ(ArrowSchemaInitFromType(&bad_schema, NANOARROW_TYPE_STRUCT), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected 4, 6, or 8 children for extension 'geoarrow.box' but got 0");
  ArrowSchemaRelease(&bad_schema);

  // Column with incorrect child name length
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[0], "name too long"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected box child 0 to have exactly four characters");
  ArrowSchemaRelease(&bad_schema);

  // Column without 'min' suffix
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[0], "xkat"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected box child 0 to have suffix 'min' but got 'xkat'");
  ArrowSchemaRelease(&bad_schema);

  // Invalid dimensions
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[0], "jmin"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected dimensions 'xy', 'xyz', 'xym', or 'xyzm' for extension "
               "'geoarrow.box' but found 'jy'");
  ArrowSchemaRelease(&bad_schema);

  // Column without 'max' suffix
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[2], "xkat"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected box child 2 to have suffix 'max' but got 'xkat'");
  ArrowSchemaRelease(&bad_schema);

  // Non-matching dimensions for max
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[2], "jmax"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Expected box child 0 name to match name for dimension 'xmin' but got 'jmax'");
  ArrowSchemaRelease(&bad_schema);

  // Column with invalid storage type
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetFormat(bad_schema.children[0], "f"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected box child 0 to have storage type of double");
  ArrowSchemaRelease(&bad_schema);

  ArrowSchemaRelease(&good_schema);
}

TEST(SchemaViewTest, SchemaViewTestInitInvalidPoint) {
  struct ArrowSchema good_schema;
  struct ArrowSchema bad_schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_POINT), GEOARROW_OK);

  // Bad storage type
  ASSERT_EQ(ArrowSchemaInitFromType(&bad_schema, NANOARROW_TYPE_INT32), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected storage type fixed-size list or struct for coord array for "
               "extension 'geoarrow.point'");
  bad_schema.release(&bad_schema);

  // Bad number of children
  ASSERT_EQ(ArrowSchemaInitFromType(&bad_schema, NANOARROW_TYPE_STRUCT), GEOARROW_OK);
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
  ASSERT_EQ(ArrowSchemaInitFromType(bad_schema.children[1], NANOARROW_TYPE_INT32),
            GEOARROW_OK);
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

TEST(SchemaViewTest, SchemaViewTestInitInvalidInterleavedPoint) {
  struct ArrowSchema good_schema;
  struct ArrowSchema bad_schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_INTERLEAVED_POINT),
            GEOARROW_OK);

  // Bad fixed size for guessed dims
  ArrowSchemaInit(&bad_schema);
  ASSERT_EQ(ArrowSchemaSetTypeFixedSize(&bad_schema, NANOARROW_TYPE_FIXED_SIZE_LIST, 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetType(bad_schema.children[0], NANOARROW_TYPE_DOUBLE),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[0], nullptr), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Can't guess dimensions for fixed size list coord array with child name "
               "'<NULL>' and fixed size 1 for extension 'geoarrow.point'");
  bad_schema.release(&bad_schema);

  // Bad fixed size with explicit dims
  ArrowSchemaInit(&bad_schema);
  ASSERT_EQ(ArrowSchemaSetTypeFixedSize(&bad_schema, NANOARROW_TYPE_FIXED_SIZE_LIST, 1),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetType(bad_schema.children[0], NANOARROW_TYPE_DOUBLE),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[0], "xy"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected fixed size list coord array with child name 'xy' to have fixed "
               "size 2 but found fixed size 1 for extension 'geoarrow.point'");
  bad_schema.release(&bad_schema);

  // Bad child type
  ASSERT_EQ(ArrowSchemaDeepCopy(&good_schema, &bad_schema), GEOARROW_OK);
  bad_schema.children[0]->release(bad_schema.children[0]);
  ASSERT_EQ(ArrowSchemaInitFromType(bad_schema.children[0], NANOARROW_TYPE_INT32),
            GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetName(bad_schema.children[0], "xy"), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected fixed-size list coordinate child 0 to have storage type of "
               "double for extension 'geoarrow.point'");
  bad_schema.release(&bad_schema);

  good_schema.release(&good_schema);
}

TEST(SchemaViewTest, SchemaViewTestInitInvalidNested) {
  struct ArrowSchema good_schema;
  struct ArrowSchema bad_schema;
  struct GeoArrowSchemaView schema_view;
  struct GeoArrowError error;

  ASSERT_EQ(GeoArrowSchemaInitExtension(&good_schema, GEOARROW_TYPE_LINESTRING),
            GEOARROW_OK);

  ASSERT_EQ(ArrowSchemaInitFromType(&bad_schema, NANOARROW_TYPE_INT32), GEOARROW_OK);
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

  ASSERT_EQ(ArrowSchemaInitFromType(&bad_schema, NANOARROW_TYPE_INT32), GEOARROW_OK);
  ASSERT_EQ(ArrowSchemaSetMetadata(&bad_schema, good_schema.metadata), GEOARROW_OK);
  EXPECT_EQ(GeoArrowSchemaViewInit(&schema_view, &bad_schema, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Expected storage type of binary or large_binary for extension 'geoarrow.wkb'");
  bad_schema.release(&bad_schema);

  good_schema.release(&good_schema);
}
