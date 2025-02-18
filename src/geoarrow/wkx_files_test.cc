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
      std::cout << path << "\n" << std::flush;
      std::basic_string<uint8_t> wkb_from_line_wkt = tester.AsWKB(line_wkt);
      // For all current examples, the ISO wkb size is the same as the EWKB size
      int64_t wkb_size = wkb_from_line_wkt.size();

      infile_wkb.read((char*)read_buffer, wkb_size);
      std::basic_string<uint8_t> line_wkb(read_buffer, wkb_size);

      infile_ewkb.read((char*)read_buffer, wkb_size);
      std::basic_string<uint8_t> line_ewkb(read_buffer, wkb_size);

      ASSERT_EQ(wkb_from_line_wkt, line_wkb);
      EXPECT_EQ(tester.AsWKT(line_wkt), line_wkt);

      EXPECT_EQ(tester.AsWKB(line_wkb), line_wkb);
      EXPECT_EQ(tester.AsWKB(line_ewkb), line_wkb);

      // Special case the empty point, which translates from WKB to
      // WKT as POINT [Z[M]] (nan nan [nan [nan]]) instead of EMPTY
      if (line_wkt == "POINT EMPTY") {
        EXPECT_EQ(tester.AsWKT(line_wkb), "POINT (nan nan)");
        EXPECT_EQ(tester.AsWKT(line_ewkb), "POINT (nan nan)");
      } else if (line_wkt == "POINT Z EMPTY") {
        EXPECT_EQ(tester.AsWKT(line_wkb), "POINT Z (nan nan nan)");
        EXPECT_EQ(tester.AsWKT(line_ewkb), "POINT Z (nan nan nan)");
      } else if (line_wkt == "POINT M EMPTY") {
        EXPECT_EQ(tester.AsWKT(line_wkb), "POINT M (nan nan nan)");
        EXPECT_EQ(tester.AsWKT(line_ewkb), "POINT M (nan nan nan)");
      } else if (line_wkt == "POINT ZM EMPTY") {
        EXPECT_EQ(tester.AsWKT(line_wkb), "POINT ZM (nan nan nan nan)");
        EXPECT_EQ(tester.AsWKT(line_ewkb), "POINT ZM (nan nan nan nan)");
      } else {
        EXPECT_EQ(tester.AsWKT(line_wkb), line_wkt);
        EXPECT_EQ(tester.AsWKT(line_ewkb), line_wkt);
      }
    }

    n_tested++;
  }

  // Make sure at least one file was tested
  EXPECT_GT(n_tested, 0);
}
