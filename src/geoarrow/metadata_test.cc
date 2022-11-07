
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
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(metadata_view.crs.data, nullptr);
  EXPECT_EQ(metadata_view.crs.n_bytes, 0);
}

TEST(MetadataTest, MetadataTestBasicDeprecated) {
  // (test will only work on little endian)
  char simple_metadata[] = {'\2', '\0', '\0', '\0', '\3', '\0', '\0', '\0', 'c',
                            'r',  's',  '\6', '\0', '\0', '\0', 'c',  'r',  's',
                            'v',  'a',  'l',  '\5', '\0', '\0', '\0', 'e',  'd',
                            'g',  'e',  's',  '\n', '\0', '\0', '\0', 's',  'p',
                            'h',  'e',  'r',  'i',  'c',  'a',  'l'};
  struct GeoArrowError error;
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  metadata.data = simple_metadata;
  metadata.n_bytes = sizeof(simple_metadata);

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), GEOARROW_OK);
  EXPECT_EQ(metadata_view.edge_type, GEOARROW_EDGE_TYPE_SPHERICAL);
  EXPECT_EQ(metadata_view.crs_type, GEOARROW_CRS_TYPE_UNKNOWN);
  EXPECT_EQ(std::string(metadata_view.crs.data, metadata_view.crs.n_bytes), "crsval");
}

TEST(MetadataTest, MetadataTestJSON) {
  struct GeoArrowError error;
  struct GeoArrowMetadataView metadata_view;
  struct GeoArrowStringView metadata;
  metadata.data = "{}";
  metadata.n_bytes = 2;

  EXPECT_EQ(GeoArrowMetadataViewInit(&metadata_view, metadata, &error), ENOTSUP);
  EXPECT_STREQ(error.message, "JSON format not yet supported");
}
