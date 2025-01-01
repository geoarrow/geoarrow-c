
#include <stdexcept>

#include <arrow/array.h>
#include <arrow/c/bridge.h>
#include <gtest/gtest.h>

#include "arrow_extension_type.hpp"

using namespace arrow;

void ASSERT_ARROW_OK(Status status) {
  if (!status.ok()) {
    throw std::runtime_error(status.message());
  }
}

using geoarrow::arrow::GeometryExtensionType;

TEST(ArrowTest, ArrowTestExtensionType) {
  auto maybe_type = GeometryExtensionType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type = maybe_type.ValueUnsafe();
  EXPECT_EQ(type->extension_name(), "geoarrow.multipoint");
  EXPECT_EQ(type->Serialize(), "{}");
  EXPECT_EQ(type->GeoArrowType().id(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(type->GeoArrowType().geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(type->GeoArrowType().coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(type->GeoArrowType().dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(type->GeoArrowType().edge_type(), GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(type->GeoArrowType().crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(type->GeoArrowType().crs(), "");
  EXPECT_EQ(type->ToString(), "GeometryExtensionType(geoarrow.multipoint)");

  auto maybe_type2 = GeometryExtensionType::Make(GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type2 = maybe_type.ValueUnsafe();
  EXPECT_TRUE(type->Equals(type2));
}

TEST(ArrowTest, ArrowTestExtensionTypeError) {
  auto maybe_type = GeometryExtensionType::Make(GEOARROW_TYPE_UNINITIALIZED);
  ASSERT_FALSE(maybe_type.ok());
  EXPECT_EQ(maybe_type.status().ToStringWithoutContextLines(),
            "Invalid: Can't construct GeometryDataType from GEOARROW_TYPE_UNINITIALIZED");

  maybe_type =
      GeometryExtensionType::Make(GEOARROW_GEOMETRY_TYPE_BOX, GEOARROW_DIMENSIONS_XY,
                                  GEOARROW_COORD_TYPE_INTERLEAVED);
  ASSERT_FALSE(maybe_type.ok());
  EXPECT_EQ(maybe_type.status().ToStringWithoutContextLines(),
            "Invalid: Combination of geometry type/dimensions/coord type not valid: "
            "BOX/xy/interleaved");
}

TEST(ArrowTest, ArrowTestExtensionTypeDeserialize) {
  auto maybe_type = GeometryExtensionType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type = maybe_type.ValueUnsafe();

  auto maybe_result = type->Deserialize(
      type->storage_type(), "{\"edges\": \"spherical\", \"crs\": \"OGC:CRS84\"}");
  ASSERT_ARROW_OK(maybe_result.status());
  auto result =
      std::dynamic_pointer_cast<GeometryExtensionType>(maybe_result.ValueUnsafe());
  EXPECT_EQ(result->GeoArrowType().id(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(result->GeoArrowType().geometry_type(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(result->GeoArrowType().coord_type(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(result->GeoArrowType().dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(result->GeoArrowType().edge_type(), GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(result->GeoArrowType().crs_type(), GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(result->GeoArrowType().crs(), "OGC:CRS84");
}

TEST(ArrowTest, ArrowTestExtensionTypeModify) {
  auto maybe_type = GeometryExtensionType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type = maybe_type.ValueUnsafe();

  auto new_type = type->WithGeometryType(GEOARROW_GEOMETRY_TYPE_POINT);
  ASSERT_ARROW_OK(new_type.status());
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().geometry_type(),
            GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().coord_type(),
            GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().edge_type(),
            GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs(), "");

  new_type = type->WithDimensions(GEOARROW_DIMENSIONS_XYM);
  ASSERT_ARROW_OK(new_type.status());
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().dimensions(), GEOARROW_DIMENSIONS_XYM);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().geometry_type(),
            GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().coord_type(),
            GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().edge_type(),
            GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs(), "");

  new_type = type->WithEdgeType(GEOARROW_EDGE_TYPE_SPHERICAL);
  ASSERT_ARROW_OK(new_type.status());
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().edge_type(),
            GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().geometry_type(),
            GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().coord_type(),
            GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs_type(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs(), "");

  new_type = type->WithCrs("some crs value");
  ASSERT_ARROW_OK(new_type.status());
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs(), "some crs value");
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().crs_type(), GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().geometry_type(),
            GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().coord_type(),
            GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(new_type.ValueUnsafe()->GeoArrowType().edge_type(),
            GEOARROW_EDGE_TYPE_PLANAR);
}

TEST(ArrowTest, ArrowTestExtensionTypeRegister) {
  struct ArrowSchema schema;
  ASSERT_ARROW_OK(GeometryExtensionType::RegisterAll());
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  auto maybe_ext_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_ext_type.status());
  EXPECT_EQ(maybe_ext_type.ValueUnsafe()->id(), arrow::Type::EXTENSION);

  ASSERT_ARROW_OK(GeometryExtensionType::UnregisterAll());
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  maybe_ext_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_ext_type.status());
  EXPECT_NE(maybe_ext_type.ValueUnsafe()->id(), arrow::Type::EXTENSION);
}
