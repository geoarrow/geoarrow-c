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

TEST(SchemaTest, SchemaTestMakeType) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XY, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZ, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON_Z);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYM, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON_M);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON_ZM);
}

TEST(SchemaTest, SchemaTestMakeTypeInvalidCoordType) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XY, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZ, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYM, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
}

TEST(SchemaTest, SchemaTestMakeTypeInvalidDimensions) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_UNKNOWN,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_UNKNOWN,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
}

TEST(SchemaTest, SchemaTestMakeTypeInvalidGeometryType) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_GEOMETRY, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
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

TEST(SchemaTest, SchemaTestInitSchemaLinestring) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(list(coord_type("xy"))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(list(coord_type("xyz"))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(list(coord_type("xym"))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_LINESTRING_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(list(coord_type("xyzm"))));
}

TEST(SchemaTest, SchemaTestInitSchemaPolygon) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(list(list(coord_type("xy")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(list(list(coord_type("xyz")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(list(list(coord_type("xym")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_POLYGON_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(list(list(coord_type("xyzm")))));
}

TEST(SchemaTest, SchemaTestInitSchemaMultipoint) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(list(coord_type("xy"))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(list(coord_type("xyz"))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(list(coord_type("xym"))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOINT_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(list(coord_type("xyzm"))));
}

TEST(SchemaTest, SchemaTestInitSchemaMultilinestring) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(list(list(coord_type("xy")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(list(list(coord_type("xyz")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(list(list(coord_type("xym")))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTILINESTRING_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(list(list(coord_type("xyzm")))));
}

TEST(SchemaTest, SchemaTestInitSchemaMultipolygon) {
  struct ArrowSchema schema;

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON), GEOARROW_OK);
  auto maybe_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type.status());
  EXPECT_TRUE(maybe_type.ValueUnsafe()->Equals(list(list(list(coord_type("xy"))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON_Z), GEOARROW_OK);
  auto maybe_type_z = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_z.status());
  EXPECT_TRUE(maybe_type_z.ValueUnsafe()->Equals(list(list(list(coord_type("xyz"))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON_M), GEOARROW_OK);
  auto maybe_type_m = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_m.status());
  EXPECT_TRUE(maybe_type_m.ValueUnsafe()->Equals(list(list(list(coord_type("xym"))))));

  EXPECT_EQ(GeoArrowSchemaInit(&schema, GEOARROW_TYPE_MULTIPOLYGON_ZM), GEOARROW_OK);
  auto maybe_type_zm = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_type_zm.status());
  EXPECT_TRUE(maybe_type_zm.ValueUnsafe()->Equals(list(list(list(coord_type("xyzm"))))));
}
