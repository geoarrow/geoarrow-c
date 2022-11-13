
#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

TEST(MetadataTest, MetadataTestEmpty) {
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  metadata.data = nullptr;
  metadata.n_bytes = 0;

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, nullptr), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(metadata_view.crs.data, nullptr);
  EXPECT_EQ(metadata_view.crs.n_bytes, 0);
}

TEST(MetadataTest, MetadataTestBasicDeprecated) {
  // (test will only work on little endian)
  char simple_metadata[] = {'\2', '\0', '\0', '\0', '\5', '\0', '\0', '\0', 'e', 'd', 'g',
                            'e',  's',  '\t', '\0', '\0', '\0', 's',  'p',  'h', 'e', 'r',
                            'i',  'c',  'a',  'l',  '\3', '\0', '\0', '\0', 'c', 'r', 's',
                            '\6', '\0', '\0', '\0', 'c',  'r',  's',  'v',  'a', 'l'};
  struct GeoArrowError error;
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  metadata.data = simple_metadata;

  // Make sure all the buffer range checks work
  for (int64_t i = 1; i < sizeof(simple_metadata); i++) {
    metadata.n_bytes = i;
    EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), ENOTSUP);
  }

  metadata.n_bytes = sizeof(simple_metadata);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(std::string(metadata_view.crs.data, metadata_view.crs.n_bytes), "crsval");

  struct ArrowSchema schema;
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_WKB), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaSetMetadataDeprecated(&schema, &metadata_view), GEOARROW_OK);

  struct GeoArrowSchemaView schema_view;
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, NULL), GEOARROW_OK);

  EXPECT_EQ(memcmp(schema_view.extension_metadata.data, simple_metadata,
                   sizeof(simple_metadata)),
            0);
  schema.release(&schema);
}

TEST(MetadataTest, MetadataTestReadJSON) {
  struct GeoArrowError error;
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  metadata.data = "{}";
  metadata.n_bytes = 2;

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), ENOTSUP);
  EXPECT_STREQ(error.message, "JSON format not yet supported");
}

TEST(MetadataTest, MetadataTestWriteJSON) {
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  struct ArrowSchema schema;
  struct GeoArrowSchemaView schema_view;
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_WKB), GEOARROW_OK);

  metadata.data = nullptr;
  metadata.n_bytes = 0;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, nullptr), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaSetMetadata(&schema, &metadata_view), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, NULL), GEOARROW_OK);
  std::string metadata_json(schema_view.extension_metadata.data,
                            schema_view.extension_metadata.n_bytes);
  EXPECT_EQ(metadata_json, "{}");

  metadata_view.edge_type = GEOARROW_EDGE_TYPE_SPHERICAL;
  ASSERT_EQ(GeoArrowSchemaSetMetadata(&schema, &metadata_view), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, NULL), GEOARROW_OK);
  metadata_json = std::string(schema_view.extension_metadata.data,
                              schema_view.extension_metadata.n_bytes);
  EXPECT_EQ(metadata_json, "{\"edges\":\"spherical\"}");

  const char* crs_value_with_quote = "crsval has \"quotes\"";
  metadata_view.crs.data = crs_value_with_quote;
  metadata_view.crs.n_bytes = strlen(crs_value_with_quote);
  metadata_view.crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
  ASSERT_EQ(GeoArrowSchemaSetMetadata(&schema, &metadata_view), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, NULL), GEOARROW_OK);
  metadata_json = std::string(schema_view.extension_metadata.data,
                              schema_view.extension_metadata.n_bytes);
  EXPECT_EQ(metadata_json,
            "{\"edges\":\"spherical\",\"crs\":\"crsval has \\\"quotes\\\"\"}");

  metadata_view.crs.data = "{}";
  metadata_view.crs.n_bytes = 2;
  metadata_view.crs_type = GEOARROW_CRS_TYPE_PROJJSON;
  ASSERT_EQ(GeoArrowSchemaSetMetadata(&schema, &metadata_view), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, NULL), GEOARROW_OK);
  metadata_json = std::string(schema_view.extension_metadata.data,
                              schema_view.extension_metadata.n_bytes);
  EXPECT_EQ(metadata_json, "{\"edges\":\"spherical\",\"crs\":{}}");

  metadata_view.edge_type = GEOARROW_EDGE_TYPE_PLANAR;
  ASSERT_EQ(GeoArrowSchemaSetMetadata(&schema, &metadata_view), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, NULL), GEOARROW_OK);
  metadata_json = std::string(schema_view.extension_metadata.data,
                              schema_view.extension_metadata.n_bytes);
  EXPECT_EQ(metadata_json, "{\"crs\":{}}");

  schema.release(&schema);
}
