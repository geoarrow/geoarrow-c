
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
  auto maybe_type = geoarrow::ExtensionType::Make(GEOARROW_TYPE_MULTIPOINT);
  ASSERT_ARROW_OK(maybe_type.status());
  auto type = maybe_type.ValueUnsafe();
  EXPECT_EQ(type->extension_name(), "geoarrow.multipoint");
  EXPECT_EQ(type->Serialize(), "");
  EXPECT_EQ(type->GeoArrowType(), GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(type->GeometryType(), GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
  EXPECT_EQ(type->CoordType(), GEOARROW_COORD_TYPE_SEPARATE);
  EXPECT_EQ(type->Dimensions(), GEOARROW_DIMENSIONS_XY);
}

TEST(ArrowTest, ArrowTestExtensionTypeRegister) {
  struct ArrowSchema schema;
  ASSERT_ARROW_OK(geoarrow::ExtensionType::RegisterAll());
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  auto maybe_ext_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_ext_type.status());
  EXPECT_EQ(maybe_ext_type.ValueUnsafe()->id(), arrow::Type::EXTENSION);

  ASSERT_ARROW_OK(geoarrow::ExtensionType::UnregisterAll());
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_MULTIPOINT), GEOARROW_OK);
  maybe_ext_type = ImportType(&schema);
  ASSERT_ARROW_OK(maybe_ext_type.status());
  EXPECT_NE(maybe_ext_type.ValueUnsafe()->id(), arrow::Type::EXTENSION);
}
