
#include <stdexcept>

#include <arrow/array.h>
#include <arrow/c/bridge.h>
#include <gtest/gtest.h>

#include "geoarrow_arrow.hpp"

using namespace arrow;

void ASSERT_ARROW_OK(Status status) {
  if (!status.ok()) {
    throw std::runtime_error(status.message());
  }
}

TEST(ArrowTest, ArrowTestExtensionType) {
  auto maybe_type = geoarrow::VectorType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type = maybe_type.ValueUnsafe();
  EXPECT_EQ(type->extension_name(), "geoarrow.multipoint");
  EXPECT_EQ(type->Serialize(), "");
  EXPECT_EQ(type->GeoArrowType(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(type->GeometryType(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(type->CoordType(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(type->Dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(type->EdgeType(), GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(type->CrsType(), GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(type->Crs(), "");

  auto maybe_type2 = geoarrow::VectorType::Make(GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type2 = maybe_type.ValueUnsafe();
  EXPECT_TRUE(type->Equals(type2));
}

TEST(ArrowTest, ArrowTestExtensionTypeDeserialize) {
  auto maybe_type = geoarrow::VectorType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type = maybe_type.ValueUnsafe();

  auto maybe_result = type->Deserialize(
      type->storage_type(), "{\"edges\": \"spherical\", \"crs\": \"OGC:CRS84\"}");
  ASSERT_ARROW_OK(maybe_result.status());
  auto result =
      std::dynamic_pointer_cast<geoarrow::VectorType>(maybe_result.ValueUnsafe());
  EXPECT_EQ(result->GeoArrowType(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(result->GeometryType(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(result->CoordType(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(result->Dimensions(), GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(result->EdgeType(), GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(result->CrsType(), GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(result->Crs(), "OGC:CRS84");
}

TEST(ArrowTest, ArrowTestExtensionTypeRegister) {
  struct ArrowSchema schema;
  ASSERT_ARROW_OK(geoarrow::VectorType::RegisterAll());
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  auto maybe_ext_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_ext_type.status());
  EXPECT_EQ(maybe_ext_type.ValueUnsafe()->id(), arrow::Type::EXTENSION);

  ASSERT_ARROW_OK(geoarrow::VectorType::UnregisterAll());
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  maybe_ext_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_ext_type.status());
  EXPECT_NE(maybe_ext_type.ValueUnsafe()->id(), arrow::Type::EXTENSION);
}
