
#include <gtest/gtest.h>

#include <geoarrow.h>
#include "nanoarrow/nanoarrow.h"

TEST(MetadataTest, MetadataTestEmpty) {
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  metadata.data = nullptr;
  metadata.size_bytes = 0;

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, nullptr), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(metadata_view.crs.data, nullptr);
  EXPECT_EQ(metadata_view.crs.size_bytes, 0);
}

TEST(MetadataTest, MetadataTestReadJSONParsing) {
  struct GeoArrowError error;
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;

  metadata.data = "[";
  metadata.size_bytes = 1;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected valid GeoArrow JSON metadata but got '['");

  metadata.data = "{";
  metadata.size_bytes = 1;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);
  EXPECT_STREQ(error.message, "Expected valid GeoArrow JSON metadata but got '{'");

  metadata.data = "{\"unterminated string\\\"";
  metadata.size_bytes = 23;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Expected valid GeoArrow JSON metadata but got '{\"unterminated string\\\"'");

  metadata.data = "{\"key\": [";
  metadata.size_bytes = 9;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);
  EXPECT_STREQ(error.message,
               "Expected valid GeoArrow JSON metadata but got '{\"key\": ['");

  metadata.data = "{}abc";
  metadata.size_bytes = 6;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);
  EXPECT_STREQ(
      error.message,
      "Expected JSON object with no trailing characters but found trailing 'abc'");

  metadata.data = " \t\r\n{\"key\": [1, \"two\", {\"three\": 4}]} ";
  metadata.size_bytes = 38;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);

  metadata.data = "{}";
  metadata.size_bytes = 2;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);

  // Incomplete 'null'
  metadata.data = "{\"key\": n";
  metadata.size_bytes = strlen(metadata.data);
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);

  // Enough characters but not actually 'null'
  metadata.data = "{\"key\": nincompoop}";
  metadata.size_bytes = strlen(metadata.data);
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);
}

void TestValidJSONMetadata(const std::string& json, enum GeoArrowEdgeType edge_type,
                           enum GeoArrowCrsType crs_type, const std::string& crs) {
  struct GeoArrowError error {};
  struct GeoArrowMetadataView metadata_view {};

  struct GeoArrowStringView metadata;
  metadata.data = json.data();
  metadata.size_bytes = static_cast<int64_t>(json.size());

  ASSERT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK)
      << error.message;
  EXPECT_EQ(metadata_view.edge_type, edge_type);
  EXPECT_EQ(metadata_view.crs_type, crs_type);
  EXPECT_EQ(std::string(metadata_view.crs.data, metadata_view.crs.size_bytes), crs);
}

TEST(MetadataTest, MetadataTestReadJSON) {
  struct GeoArrowError error;
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;

  const char* json_crs_spherical = "{\"edges\":\"spherical\",\"crs\":{}}";
  metadata.data = json_crs_spherical;
  metadata.size_bytes = strlen(json_crs_spherical);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(std::string(metadata_view.crs.data, metadata_view.crs.size_bytes), "{}");

  const char* json_crs_unknown = "{\"crs\":\"a string\"}";
  metadata.data = json_crs_unknown;
  metadata.size_bytes = strlen(json_crs_unknown);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(std::string(metadata_view.crs.data, metadata_view.crs.size_bytes),
            "\"a string\"");

  const char* json_crs_projjson = R"({"crs": {}, "crs_type": "projjson"})";
  metadata.data = json_crs_projjson;
  metadata.size_bytes = strlen(json_crs_projjson);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_PROJJSON);
  EXPECT_EQ(std::string(metadata_view.crs.data, metadata_view.crs.size_bytes), "{}");

  const char* json_crs_wkt2 = R"({"crs": {}, "crs_type": "wkt2:2019"})";
  metadata.data = json_crs_wkt2;
  metadata.size_bytes = strlen(json_crs_wkt2);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_WKT2_2019);
  EXPECT_EQ(std::string(metadata_view.crs.data, metadata_view.crs.size_bytes), "{}");

  const char* json_crs_none = "{\"crs\":null}";
  metadata.data = json_crs_none;
  metadata.size_bytes = strlen(json_crs_none);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(metadata_view.crs.size_bytes, 0);

  // If the crs_type was specified but the crs was unset, we should still get
  // CRS_TYPE_NONE.
  const char* json_crs_none_with_type = R"({"crs":null, "crs_type": "projjson"})";
  metadata.data = json_crs_none;
  metadata.size_bytes = strlen(json_crs_none);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(metadata_view.crs.size_bytes, 0);

  const char* json_crs_invalid = "{\"crs\":[]}";
  metadata.data = json_crs_invalid;
  metadata.size_bytes = strlen(json_crs_invalid);
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);

  const char* json_edges_none = "{\"edges\":null}";
  metadata.data = json_edges_none;
  metadata.size_bytes = strlen(json_edges_none);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(metadata_view.crs.size_bytes, 0);

  const char* json_edges_planar = "{\"edges\":\"planar\"}";
  metadata.data = json_edges_planar;
  metadata.size_bytes = strlen(json_edges_planar);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_PLANAR);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_NONE);
  EXPECT_EQ(metadata_view.crs.size_bytes, 0);

  const char* json_edges_invalid = "{\"edges\":\"unsupported value\"}";
  metadata.data = json_edges_invalid;
  metadata.size_bytes = strlen(json_edges_invalid);
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);

  const char* json_edges_invalid_type = "{\"edges\":true}";
  metadata.data = json_edges_invalid_type;
  metadata.size_bytes = strlen(json_edges_invalid_type);
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), EINVAL);
}

TEST(MetadataTest, MetadataTestSetMetadata) {
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  struct ArrowSchema schema;
  struct GeoArrowSchemaView schema_view;
  ASSERT_EQ(GeoArrowSchemaInitExtension(&schema, GEOARROW_TYPE_WKB), GEOARROW_OK);
  metadata.data = nullptr;
  metadata.size_bytes = 0;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, nullptr), GEOARROW_OK);

  metadata_view.crs.data = "{}";
  metadata_view.crs.size_bytes = 2;
  metadata_view.crs_type = GEOARROW_CRS_TYPE_PROJJSON;
  ASSERT_EQ(GeoArrowSchemaSetMetadata(&schema, &metadata_view), GEOARROW_OK);
  ASSERT_EQ(GeoArrowSchemaViewInit(&schema_view, &schema, NULL), GEOARROW_OK);
  std::string metadata_json = std::string(schema_view.extension_metadata.data,
                                          schema_view.extension_metadata.size_bytes);
  EXPECT_EQ(metadata_json, "{\"crs\":{}}");

  schema.release(&schema);
}

TEST(MetadataTest, MetadataTestWriteJSON) {
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  char out[1024];
  memset(out, 'Z', sizeof(out));

  metadata.data = nullptr;
  metadata.size_bytes = 0;
  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, nullptr), GEOARROW_OK);
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, nullptr, 0), 2);
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, out, sizeof(out)), 2);
  EXPECT_STREQ(out, "{}");

  metadata_view.edge_type = GEOARROW_EDGE_TYPE_SPHERICAL;
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, nullptr, 0), 21);
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, out, sizeof(out)), 21);
  EXPECT_STREQ(out, "{\"edges\":\"spherical\"}");

  const char* crs_value_with_quote = "crsval has \"quotes\"";
  metadata_view.crs.data = crs_value_with_quote;
  metadata_view.crs.size_bytes = strlen(crs_value_with_quote);
  metadata_view.crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, nullptr, 0), 51);
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, out, sizeof(out)), 51);
  EXPECT_STREQ(out, "{\"edges\":\"spherical\",\"crs\":\"crsval has \\\"quotes\\\"\"}");

  const char* crs_value_with_preescaped_quote = "\"crsval has \\\"quotes\\\"\"";
  metadata_view.crs.data = crs_value_with_preescaped_quote;
  metadata_view.crs.size_bytes = strlen(crs_value_with_preescaped_quote);
  metadata_view.crs_type = GEOARROW_CRS_TYPE_UNKNOWN;
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, nullptr, 0), 51);
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, out, sizeof(out)), 51);
  EXPECT_STREQ(out, "{\"edges\":\"spherical\",\"crs\":\"crsval has \\\"quotes\\\"\"}");

  metadata_view.crs.data = "{}";
  metadata_view.crs.size_bytes = 2;
  metadata_view.crs_type = GEOARROW_CRS_TYPE_PROJJSON;
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, nullptr, 0), 30);
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, out, sizeof(out)), 30);
  EXPECT_STREQ(out, "{\"edges\":\"spherical\",\"crs\":{}}");

  metadata_view.edge_type = GEOARROW_EDGE_TYPE_PLANAR;
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, nullptr, 0), 10);
  EXPECT_EQ(GeoArrowMetadataSerialize(&metadata_view, out, sizeof(out)), 10);
  EXPECT_STREQ(out, "{\"crs\":{}}");
}

TEST(MetadataTest, MetadataTestUnescapeCRS) {
  char out[1024];
  memset(out, 'Z', sizeof(out));

  struct GeoArrowStringView crs;
  crs.data = nullptr;
  crs.size_bytes = 0;

  EXPECT_EQ(GeoArrowUnescapeCrs(crs, out, 0), 0);
  EXPECT_EQ(out[0], 'Z');
  EXPECT_EQ(GeoArrowUnescapeCrs(crs, out, 1), 0);
  EXPECT_EQ(out[0], '\0');

  // Check that an unquoted value is returned as is
  const char* crs_unquoted = "some unquoted value";
  crs.data = crs_unquoted;
  crs.size_bytes = strlen(crs_unquoted);

  // Basic length without ever writing to out
  EXPECT_EQ(GeoArrowUnescapeCrs(crs, nullptr, 0), strlen(crs_unquoted));

  // Write just enough characters to omit the null terminator
  EXPECT_EQ(GeoArrowUnescapeCrs(crs, out, strlen(crs_unquoted)), strlen(crs_unquoted));
  EXPECT_EQ(std::string(out, crs.size_bytes), crs_unquoted);
  EXPECT_EQ(out[crs.size_bytes], 'Z');

  // Make sure the null-terminator is written if possible
  EXPECT_EQ(GeoArrowUnescapeCrs(crs, out, strlen(crs_unquoted) + 1),
            strlen(crs_unquoted));
  EXPECT_EQ(std::string(out, crs.size_bytes), crs_unquoted);
  EXPECT_EQ(out[crs.size_bytes], '\0');

  // Check a quoted value
  const char* crs_quoted = "\"some quoted value with a \\\\ escape\"";
  const char* crs_quoted_unescaped = "some quoted value with a \\ escape";
  crs.data = crs_quoted;
  crs.size_bytes = strlen(crs_quoted);

  // Basic length without ever writing to out
  EXPECT_EQ(GeoArrowUnescapeCrs(crs, nullptr, 0), strlen(crs_quoted_unescaped));

  // Write just enough characters to omit the null terminator
  EXPECT_EQ(GeoArrowUnescapeCrs(crs, out, strlen(crs_quoted_unescaped)),
            strlen(crs_quoted_unescaped));
  EXPECT_EQ(std::string(out, strlen(crs_quoted_unescaped)), crs_quoted_unescaped);
  EXPECT_EQ(out[strlen(crs_quoted_unescaped)], 'Z');

  // Make sure the null-terminator is written if possible
  EXPECT_EQ(GeoArrowUnescapeCrs(crs, out, strlen(crs_quoted_unescaped) + 1),
            strlen(crs_quoted_unescaped));
  EXPECT_EQ(std::string(out, strlen(crs_quoted_unescaped)), crs_quoted_unescaped);
  EXPECT_EQ(out[strlen(crs_quoted_unescaped)], '\0');
}
