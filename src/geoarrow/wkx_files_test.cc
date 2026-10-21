#include <filesystem>
#include <fstream>

#include "geoarrow/wkx_testing.hpp"

#include <gtest/gtest.h>

TEST(WKXFilesTest, WKXFilesTestFiles) {
  const char* testing_dir = getenv("GEOARROW_TESTING_DIR");

  if (testing_dir == nullptr || strlen(testing_dir) == 0) {
    GTEST_SKIP();
  }

  WKXTester tester;
  int n_tested = 0;
  // Needs to be bigger than all the WKB items
  uint8_t read_buffer[8096];

  for (const auto& item : std::filesystem::directory_iterator(testing_dir)) {
    // Make sure we have a .wkt file
    std::string path = item.path();
    if (path.size() < 4) {
      continue;
    }

    if (path.substr(path.size() - 4, path.size()) != ".wkt") {
      continue;
    }

    // Some values from nc.wkt don't round trip exactly because of double precision
    // printing
    if (path.substr(path.size() - 6, path.size()) == "nc.wkt") {
      continue;
    }

    SCOPED_TRACE("Path: " + path);

    std::stringstream wkb_path_builder;
    wkb_path_builder << path.substr(0, path.size() - 4) << ".wkb";

    std::stringstream ewkb_path_builder;
    ewkb_path_builder << path.substr(0, path.size() - 4) << ".ewkb";

    // Expect that all lines roundtrip
    std::ifstream infile(path);
    std::ifstream infile_wkb(wkb_path_builder.str());
    std::ifstream infile_ewkb(ewkb_path_builder.str());
    std::string line_wkt;
    while (std::getline(infile, line_wkt)) {
      SCOPED_TRACE("WKT: " + line_wkt);

      std::vector<uint8_t> wkb_from_line_wkt = tester.AsWKB(line_wkt);
      // For all current examples, the ISO wkb size is the same as the EWKB size
      int64_t wkb_size = wkb_from_line_wkt.size();

      infile_wkb.read((char*)read_buffer, wkb_size);
      std::vector<uint8_t> line_wkb(read_buffer, read_buffer + wkb_size);

      infile_ewkb.read((char*)read_buffer, wkb_size);
      std::vector<uint8_t> line_ewkb(read_buffer, read_buffer + wkb_size);

      ASSERT_EQ(wkb_from_line_wkt, line_wkb);
      EXPECT_EQ(tester.AsWKT(line_wkt), line_wkt);

      EXPECT_EQ(tester.AsWKB(line_wkb), line_wkb);
      EXPECT_EQ(tester.AsWKB(line_ewkb), line_wkb);

      struct GeoArrowGeometry geom_cloner;
      ASSERT_EQ(GeoArrowGeometryInit(&geom_cloner), GEOARROW_OK);

      const GeoArrowGeometry& geom_from_wkt = tester.AsGeometry(line_wkt);
      EXPECT_EQ(tester.AsWKTUsingVisitor(geom_from_wkt), line_wkt);
      EXPECT_EQ(tester.AsWKT(geom_from_wkt), line_wkt);

      // Geometry collections do not have a native GeoArrow array type. For every
      // other geometry, exercise scalar access using both native coordinate layouts.
      if (geom_from_wkt.root->geometry_type !=
          GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION) {
        for (auto coord_type :
             {GEOARROW_COORD_TYPE_SEPARATE, GEOARROW_COORD_TYPE_INTERLEAVED}) {
          enum GeoArrowType type = GeoArrowMakeType(
              static_cast<enum GeoArrowGeometryType>(geom_from_wkt.root->geometry_type),
              static_cast<enum GeoArrowDimensions>(geom_from_wkt.root->dimensions),
              coord_type);
          SCOPED_TRACE(coord_type == GEOARROW_COORD_TYPE_SEPARATE ? "separate"
                                                                  : "interleaved");

          struct GeoArrowNativeWriter writer;
          ASSERT_EQ(GeoArrowNativeWriterInit(&writer, type), GEOARROW_OK);
          ASSERT_EQ(GeoArrowNativeWriterAppend(
                        &writer, GeoArrowGeometryAsView(&geom_from_wkt), nullptr),
                    GEOARROW_OK);

          struct ArrowArray array;
          ASSERT_EQ(GeoArrowNativeWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
          GeoArrowNativeWriterReset(&writer);

          struct GeoArrowArrayView array_view;
          ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, type), GEOARROW_OK);
          ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array, nullptr), GEOARROW_OK);

          struct GeoArrowGeometry geom_from_array;
          ASSERT_EQ(GeoArrowGeometryInit(&geom_from_array), GEOARROW_OK);
          ASSERT_EQ(GeoArrowArrayViewGetGeometry(&array_view, 0, &geom_from_array),
                    GEOARROW_OK);
          EXPECT_EQ(tester.AsWKT(geom_from_array), line_wkt);
          EXPECT_EQ(tester.AsWKTUsingVisitor(geom_from_array), line_wkt);

          GeoArrowGeometryReset(&geom_from_array);
          array.release(&array);
        }
      }

      ASSERT_EQ(GeoArrowGeometryShallowCopy(GeoArrowGeometryAsView(&geom_from_wkt),
                                            &geom_cloner),
                GEOARROW_OK);
      EXPECT_EQ(tester.AsWKTUsingVisitor(geom_cloner), line_wkt);
      EXPECT_EQ(tester.AsWKT(geom_cloner), line_wkt);

      ASSERT_EQ(
          GeoArrowGeometryDeepCopy(GeoArrowGeometryAsView(&geom_from_wkt), &geom_cloner),
          GEOARROW_OK);
      EXPECT_EQ(tester.AsWKTUsingVisitor(geom_cloner), line_wkt);
      EXPECT_EQ(tester.AsWKT(geom_cloner), line_wkt);

      GeoArrowGeometryReset(&geom_cloner);

      const GeoArrowGeometry& geom_from_wkb = tester.AsGeometry(line_wkb);
      EXPECT_EQ(tester.AsWKB(geom_from_wkb), line_wkb);

      EXPECT_EQ(tester.AsWKT(line_wkb), line_wkt);
      EXPECT_EQ(tester.AsWKT(line_ewkb), line_wkt);
    }

    n_tested++;
  }

  // Make sure at least one file was tested
  EXPECT_GT(n_tested, 0);
}
