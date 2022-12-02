
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

class WKTTestException : public std::exception {
 public:
  WKTTestException(const char* step, int code, const char* msg) {
    std::stringstream ss;
    ss << step << "(" << code << "): " << msg;
    message = ss.str();
  }

  const char* what() const noexcept { return message.c_str(); }

 private:
  std::string message;
};

class WKTTester {
 public:
  WKTTester(bool use_flat_multipoint = true) {
    GeoArrowWKTReaderInit(&reader_);
    GeoArrowWKTWriterInit(&writer_);
    writer_.use_flat_multipoint = use_flat_multipoint;
    GeoArrowWKTWriterInitVisitor(&writer_, &v_);
    v_.error = &error_;
    array_.release = nullptr;
    ArrowArrayViewInit(&array_view_, NANOARROW_TYPE_STRING);
  }

  ~WKTTester() {
    GeoArrowWKTReaderReset(&reader_);
    GeoArrowWKTWriterReset(&writer_);
    if (array_.release != nullptr) {
      array_.release(&array_);
    }
    ArrowArrayViewReset(&array_view_);
  }

  std::string LastErrorMessage() { return std::string(error_.message); }

  std::string AsWKT(const std::string& str) {
    error_.message[0] = '\0';
    if (array_.release != nullptr) {
      array_.release(&array_);
    }

    struct GeoArrowStringView str_view;
    str_view.data = str.data();
    str_view.n_bytes = str.size();

    int result = GeoArrowWKTReaderVisit(&reader_, str_view, &v_);
    if (result != GEOARROW_OK) {
      throw WKTTestException("GeoArrowWKTReaderVisit", result, error_.message);
    }

    result = GeoArrowWKTWriterFinish(&writer_, &array_, &error_);
    if (result != GEOARROW_OK) {
      throw WKTTestException("GeoArrowWKTWriterFinish", result, error_.message);
    }

    result = ArrowArrayViewSetArray(&array_view_, &array_,
                                    reinterpret_cast<struct ArrowError*>(&error_));
    if (result != GEOARROW_OK) {
      throw WKTTestException("ArrowArrayViewSetArray", result, error_.message);
    }

    struct ArrowStringView answer = ArrowArrayViewGetStringUnsafe(&array_view_, 0);
    return std::string(answer.data, answer.n_bytes);
  }

 private:
  struct GeoArrowWKTReader reader_;
  struct GeoArrowWKTWriter writer_;
  struct GeoArrowVisitor v_;
  struct ArrowArray array_;
  struct ArrowArrayView array_view_;
  struct GeoArrowError error_;
};

#define EXPECT_WKT_ROUNDTRIP(tester_, wkt_) EXPECT_EQ(tester_.AsWKT(wkt_), wkt_)

TEST(WKTReaderTest, WKTReaderTestBasic) {
  struct GeoArrowWKTReader reader;
  GeoArrowWKTReaderInit(&reader);
  GeoArrowWKTReaderReset(&reader);
}

TEST(WKTReaderTest, WKTReaderTestPoint) {
  WKTTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, "POINT EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "POINT (0 1)");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT Z (0 1 2)");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT M (0 1 3)");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT ZM (0 1 2 3)");

  // Extra whitespace is OK; no whitepsace after POINT is OK
  EXPECT_EQ(tester.AsWKT(" POINT(0    1) "), "POINT (0 1)");

  EXPECT_THROW(tester.AsWKT("POINT A"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected '(' or 'EMPTY' at byte 6");
  EXPECT_THROW(tester.AsWKT("POINT Z A"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected '(' or 'EMPTY' at byte 8");
  EXPECT_THROW(tester.AsWKT("POINT ZA"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected '(' or 'EMPTY' at byte 6");
  EXPECT_THROW(tester.AsWKT("POINT (0)"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected whitespace at byte 8");
  EXPECT_THROW(tester.AsWKT("POINT (0 )"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 9");
  EXPECT_THROW(tester.AsWKT("POINT (0 1) shouldbetheend"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected end of input at byte 12");
}

TEST(WKTReaderTest, WKTReaderTestLinestring) {
  WKTTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING ZM EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING (1 2)");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING (1 2, 2 3)");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING Z (1 2 3)");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING Z (1 2 3, 2 3 4)");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING M (1 2 3)");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING M (1 2 4, 2 3 5)");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING ZM (1 2 3 4)");
  EXPECT_WKT_ROUNDTRIP(tester, "LINESTRING ZM (1 2 3 4, 2 3 4 5)");

  EXPECT_THROW(tester.AsWKT("LINESTRING (0)"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected whitespace at byte 13");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 )"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 14");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1()"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected ',' at byte 15");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1,)"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 16");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1, )"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 17");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1, 1)"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected whitespace at byte 18");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1, 1 )"), WKTTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 19");
}

TEST(WKTReaderTest, WKTReaderTestPolygon) {
  WKTTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON ((1 2, 2 3, 4 5, 1 2))");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON ((1 2, 2 3, 4 5, 1 2), (1 2, 2 3, 4 5, 1 2))");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON Z ((1 2 3, 2 3 4, 4 5 6, 1 2 3))");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON M ((1 2 4, 2 3 5, 4 5 7, 1 2 4))");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON ZM ((1 2 3 4, 2 3 4 5, 4 5 6 7, 1 2 3 4))");

  // Not really valid WKT but happens to parse here
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON (EMPTY)");
  EXPECT_WKT_ROUNDTRIP(tester, "POLYGON (EMPTY, EMPTY)");
}

TEST(WKTReaderTest, WKTReaderTestFlatMultipoint) {
  WKTTester tester(true);
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT (1 2)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT (1 2, 2 3)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT Z (1 2 3)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT M (1 2 4)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT ZM (1 2 3 4)");
}

TEST(WKTReaderTest, WKTReaderTestMultipoint) {
  WKTTester tester(false);
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT ((1 2))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT ((1 2), (2 3))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT Z ((1 2 3))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT M ((1 2 4))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT ZM ((1 2 3 4))");

  // Not really valid WKT but happens to parse here
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT (EMPTY)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOINT (EMPTY, EMPTY)");
}

TEST(WKTReaderTest, WKTReaderTestMultilinestring) {
  WKTTester tester;

  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING ((1 2, 2 3))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING ((1 2, 2 3), (1 2, 2 3))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING Z ((1 2 3, 2 3 4))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING M ((1 2 4, 2 3 5))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING ZM ((1 2 3 4, 2 3 4 5))");

  // Not really valid WKT but happens to parse here
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING (EMPTY)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTILINESTRING (EMPTY, EMPTY)");
}

TEST(WKTReaderTest, WKTReaderTestMultipolygon) {
  WKTTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)))");
  EXPECT_WKT_ROUNDTRIP(tester,
                       "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2), (1 2, 2 3, 4 5, 1 2)))");
  EXPECT_WKT_ROUNDTRIP(tester,
                       "MULTIPOLYGON (((1 2, 2 3, 4 5, 1 2)), ((1 2, 2 3, 4 5, 1 2)))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON Z (((1 2 3, 2 3 4, 4 5 6, 1 2 3)))");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON M (((1 2 4, 2 3 5, 4 5 7, 1 2 4)))");
  EXPECT_WKT_ROUNDTRIP(tester,
                       "MULTIPOLYGON ZM (((1 2 3 4, 2 3 4 5, 4 5 6 7, 1 2 3 4)))");

  // Not really valid WKT but happens to parse here
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON (EMPTY)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON (EMPTY, EMPTY)");
  EXPECT_WKT_ROUNDTRIP(tester, "MULTIPOLYGON ((EMPTY))");
}

TEST(WKTReaderTest, WKTReaderTestGeometrycollection) {
  WKTTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION (POINT (1 2))");
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION (POINT (1 2), LINESTRING (2 3, 4 5))");
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION Z (POINT Z (1 2 3))");
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION M (POINT M (1 2 4))");
  EXPECT_WKT_ROUNDTRIP(tester, "GEOMETRYCOLLECTION ZM (POINT ZM (1 2 3 4))");
}

TEST(WKTReaderTest, WKTReaderTestManyCoordinates) {
  // The reader uses an internal coordinate buffer of 64 coordinates; however,
  // none of the above tests have enough coordinates to run into a situation
  // where it must be flushed.

  // Make a big linestring;
  std::stringstream ss;
  ss << "LINESTRING (0 1";
  for (int i = 1; i < 128; i++) {
    ss << ", " << i << " " << (i + 1);
  }
  ss << ")";

  WKTTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, ss.str());
}

TEST(WKTReaderTest, WKTReaderTestLongCoordinates) {
  // All of the readers above use integer coordinates. This tests really
  // long coordinates that always use all 16 precision spaces.

  std::stringstream ss;
  ss << std::setprecision(16);
  ss << "LINESTRING (" << (1.0 / 3.0) << " " << (1 + (1.0 / 3.0));
  for (int i = 1; i < 10; i++) {
    ss << ", " << (i + (1.0 / 3.0)) << " " << (i + 1 + (1.0 / 3.0));
  }
  ss << ")";

  WKTTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, ss.str());
}

TEST(WKTReaderTest, WKTReaderTestRoundtripTestingFiles) {
  const char* testing_dir = getenv("GEOARROW_TESTING_DIR");
  if (testing_dir == nullptr || strlen(testing_dir) == 0) {
    GTEST_SKIP();
  }

  WKTTester tester(false);
  int n_tested = 0;
  for (const auto& item : std::filesystem::directory_iterator(testing_dir)) {
    // Make sure we have a .wkt file
    std::string path = item.path();
    if (path.size() < 4) {
      continue;
    }

    if (path.substr(path.size() - 4, path.size()) != ".wkt") {
      continue;
    }

    // Expect that all lines roundtrip
    std::ifstream infile(item.path());
    std::string line;
    while (std::getline(infile, line)) {
      EXPECT_WKT_ROUNDTRIP(tester, line);
    }

    n_tested++;
  }

  // Make sure at least one file was tested
  EXPECT_GT(n_tested, 0);
}
