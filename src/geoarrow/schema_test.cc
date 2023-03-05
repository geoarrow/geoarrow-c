#include <stdexcept>

#include <arrow/array.h>
#include <arrow/c/bridge.h>
#include <gtest/gtest.h>

#include "geoarrow.h"

using namespace arrow;

void ASSERT_ARROW_OK(Status status) {
  if (!status.ok()) {
    throw std::runtime_error(status.message());
  }
}

std::shared_ptr<DataType> coord_type(std::string dims) {
  if (dims == "xy") {
    return struct_({field("x", float64()), field("y", float64())});
  } else if (dims == "xyz") {
    return struct_({field("x", float64()), field("y", float64()), field("z", float64())});
  } else if (dims == "xym") {
    return struct_({field("x", float64()), field("y", float64()), field("m", float64())});
  } else if (dims == "xyzm") {
    return struct_({field("x", float64()), field("y", float64()), field("z", float64()),
                    field("m", float64())});
  } else {
    throw std::runtime_error("unsuppored dims in helper");
  }
}

std::shared_ptr<DataType> interleaved_coord_type(std::string dims) {
  return fixed_size_list(field(dims, float64()), dims.size());
}

TEST(SchemaTest, SchemaTestInitSchemaWKB) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_WKB), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(binary()));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LARGE_WKB), GEOARROW_OK);
  auto maybe_type_large = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_large.status());
  EXPECT_TRUE(maybe_type_large.ValueUnsafe()->Equals(large_binary()));
}

TEST(SchemaTest, SchemaTestInitSchemaWKT) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_WKT), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(utf8()));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LARGE_WKT), GEOARROW_OK);
  auto maybe_type_large = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_large.status());
  EXPECT_TRUE(maybe_type_large.ValueUnsafe()->Equals(large_utf8()));
}

TEST(SchemaTest, SchemaTestInitSchemaPoint) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POINT), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(coord_type("xy")));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POINT_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(coord_type("xyz")));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POINT_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(coord_type("xym")));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POINT_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(coord_type("xyzm")));
}

TEST(SchemaTest, SchemaTestInitSchemaInterleavedPoint) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_POINT), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(interleaved_coord_type("xy")));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_POINT_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(interleaved_coord_type("xyz")));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_POINT_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(interleaved_coord_type("xym")));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_POINT_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(interleaved_coord_type("xyzm")));
}

TEST(SchemaTest, SchemaTestInitSchemaLinestring) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(
      maybe_type.ValueUnsafe()->Equals(list(field("vertices", coord_type("xy")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(
      maybe_type_z.ValueUnsafe()->Equals(list(field("vertices", coord_type("xyz")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(
      maybe_type_m.ValueUnsafe()->Equals(list(field("vertices", coord_type("xym")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(
      maybe_type_zm.ValueUnsafe()->Equals(list(field("vertices", coord_type("xyzm")))));
}

TEST(SchemaTest, SchemaTestInitSchemaInterleavedLinestring) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_LINESTRING),
            GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(
      list(field("vertices", interleaved_coord_type("xy")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z),
            GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(
      list(field("vertices", interleaved_coord_type("xyz")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_LINESTRING_M),
            GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(
      list(field("vertices", interleaved_coord_type("xym")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_INTERLEAVED_LINESTRING_ZM),
            GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(
      list(field("vertices", interleaved_coord_type("xyzm")))));
}

TEST(SchemaTest, SchemaTestInitSchemaPolygon) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(
      list(field("rings", list(field("vertices", coord_type("xy")))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(
      list(field("rings", list(field("vertices", coord_type("xyz")))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(
      list(field("rings", list(field("vertices", coord_type("xym")))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(
      list(field("rings", list(field("vertices", coord_type("xyzm")))))));
}

TEST(SchemaTest, SchemaTestInitSchemaMultipoint) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(list(field("points", coord_type("xy")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(
      maybe_type_z.ValueUnsafe()->Equals(list(field("points", coord_type("xyz")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(
      maybe_type_m.ValueUnsafe()->Equals(list(field("points", coord_type("xym")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(
      maybe_type_zm.ValueUnsafe()->Equals(list(field("points", coord_type("xyzm")))));
}

TEST(SchemaTest, SchemaTestInitSchemaMultilinestring) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(
      list(field("linestrings", list(field("vertices", coord_type("xy")))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(
      list(field("linestrings", list(field("vertices", coord_type("xyz")))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(
      list(field("linestrings", list(field("vertices", coord_type("xym")))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(
      list(field("linestrings", list(field("vertices", coord_type("xyzm")))))));
}

TEST(SchemaTest, SchemaTestInitSchemaMultipolygon) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(list(field(
      "polygons", list(field("rings", list(field("vertices", coord_type("xy")))))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(list(field(
      "polygons", list(field("rings", list(field("vertices", coord_type("xyz")))))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(list(field(
      "polygons", list(field("rings", list(field("vertices", coord_type("xym")))))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(list(field(
      "polygons", list(field("rings", list(field("vertices", coord_type("xyzm")))))))));
}
