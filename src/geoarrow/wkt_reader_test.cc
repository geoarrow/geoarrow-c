#include <iomanip>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

#include "geoarrow/wkx_testing.hpp"

#define EXPECT_WKT_ROUNDTRIP(tester_, wkt_) EXPECT_EQ(tester_.AsWKT(wkt_), wkt_)

namespace {

class WKTNumberTester {
 public:
  WKTNumberTester() {
    GeoArrowWKTReaderInit(&reader_);
    GeoArrowVisitorInitVoid(&visitor_);
    visitor_.private_data = this;
    visitor_.error = &error;
    visitor_.coords = [](GeoArrowVisitor* v, const GeoArrowCoordView* coords) {
      auto* self = static_cast<WKTNumberTester*>(v->private_data);
      for (int64_t i = 0; i < coords->n_coords; i++) {
        for (int j = 0; j < coords->n_values; j++) {
          self->values.push_back(GEOARROW_COORD_VIEW_VALUE(coords, i, j));
        }
      }
      return GEOARROW_OK;
    };
  }

  ~WKTNumberTester() { GeoArrowWKTReaderReset(&reader_); }

  int Read(const char* data, int64_t size) {
    values.clear();
    return GeoArrowWKTReaderVisit(&reader_, {data, size}, &visitor_);
  }

  int Read(const std::string& wkt) { return Read(wkt.data(), wkt.size()); }

  std::vector<double> values;
  GeoArrowError error;

 private:
  GeoArrowWKTReader reader_;
  GeoArrowVisitor visitor_;
};

uint64_t DoubleBits(double value) {
  uint64_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

}  // namespace

TEST(WKTReaderTest, WKTReaderTestNumberBits) {
  const struct {
    const char* text;
    uint64_t bits;
  } cases[] = {
      {"0", 0x0000000000000000ULL},
      {"-0", 0x8000000000000000ULL},
      {"-00", 0x8000000000000000ULL},
      {"-0.0e10", 0x8000000000000000ULL},
      {".5", 0x3fe0000000000000ULL},
      {"-.5", 0xbfe0000000000000ULL},
      {"1e+2", 0x4059000000000000ULL},
      {"1.7976931348623157e308", 0x7fefffffffffffffULL},
      {"2.2250738585072014e-308", 0x0010000000000000ULL},
      {"4.9406564584124654e-324", 0x0000000000000001ULL},
      {"1.00000000000000011102230246251565404236316680908203125", 0x3ff0000000000000ULL},
      {"1.00000000000000011102230246251565404236316680908203126", 0x3ff0000000000001ULL},
  };
  WKTNumberTester tester;
  for (const auto& item : cases) {
    SCOPED_TRACE(item.text);
    ASSERT_EQ(tester.Read(std::string("POINT(") + item.text + " 0)"), GEOARROW_OK);
    ASSERT_EQ(tester.values.size(), 2);
    EXPECT_EQ(DoubleBits(tester.values[0]), item.bits);
  }
}

TEST(WKTReaderTest, WKTReaderTestNumbersMatchTokenBoundedParser) {
  // Match the backend's rounding and require it to consume the entire token.
  std::vector<std::string> tokens = {
      "1",      "12",      "-12",         "+12",
      "++1",    "1.",      "1e-10",       "1E+10",
      "1e",     "1e+",     "1.2.3",       "1suffix",
      "0x10",   "1_000",   "1e400",       "-1e400",
      "1e-400", "-1e-400", "nan",         "NaN",
      "-nan",   "inf",     "-inf",        "Infinity",
      "+inf",   "+NaN",    "nansuffix",   "-",
      ".",      "-.",      "1.0\xc2\xbe", "123456789012345678901234567890"};

  for (int i = 0; i <= 110; i++) {
    std::string integer = std::to_string(i);
    for (const char* prefix : {"", "-", "0", "-0"}) {
      tokens.push_back(prefix + integer);
    }
    tokens.push_back(integer + ".5");
    tokens.push_back(integer + "e-1");
  }

  uint64_t state = 161;
  for (int i = 0; i < 2000; i++) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    double value;
    std::memcpy(&value, &state, sizeof(value));
    if (!std::isfinite(value)) continue;
    char text[32];
    std::snprintf(text, sizeof(text), "%.17g", value);
    tokens.emplace_back(text);
  }

  WKTNumberTester tester;
  for (const auto& token : tokens) {
    SCOPED_TRACE(token);
    double expected;
    const char* end;
    int expected_result =
        GeoArrowFromChars(token.data(), token.data() + token.size(), &expected, &end);
    if (expected_result == GEOARROW_OK && end != token.data() + token.size()) {
      expected_result = EINVAL;
    }
    int result = tester.Read("POINT(" + token + " 0)");
    ASSERT_EQ(result, expected_result);
    if (result == GEOARROW_OK) {
      ASSERT_EQ(tester.values.size(), 2);
      EXPECT_EQ(DoubleBits(tester.values[0]), DoubleBits(expected));
    }
  }
}

TEST(WKTReaderTest, WKTReaderTestRejectNumericSuffixes) {
  WKTNumberTester tester;
  for (const char* token : {"1trash", "1e", "1e+", "1.2.3", "1_000", "1.0\xc2\xbe",
                            "nantrash", "-nantrash", "inftrash", "Infinitytrash"}) {
    SCOPED_TRACE(token);
    EXPECT_EQ(tester.Read(std::string("POINT(") + token + " 2)"), EINVAL);
    EXPECT_EQ(tester.Read(std::string("POINT(1 ") + token + ")"), EINVAL);
    EXPECT_EQ(tester.Read(std::string("LINESTRING(1 ") + token + ",3 4)"), EINVAL);
    EXPECT_EQ(tester.Read(std::string("POINT ZM(1 2 ") + token + " 4)"), EINVAL);
  }
}

TEST(WKTReaderTest, WKTReaderTestRejectTrailingBytes) {
  WKTNumberTester tester;
  for (const char* geometry : {"POINT(1 2)", "POINT EMPTY", "GEOMETRYCOLLECTION EMPTY"}) {
    for (const char* whitespace : {"", " \t\r\n"}) {
      std::string wkt = std::string(geometry) + whitespace;
      SCOPED_TRACE(wkt);
      ASSERT_EQ(tester.Read(wkt), GEOARROW_OK);
      const auto valid_size = wkt.size();
      EXPECT_EQ(tester.Read(wkt + "trash"), EINVAL);
      wkt.push_back('\0');
      EXPECT_EQ(tester.Read(wkt), EINVAL);
      wkt += "trash";
      EXPECT_EQ(tester.Read(wkt), EINVAL);
      EXPECT_EQ(tester.error.message,
                "Expected end of input at byte " + std::to_string(valid_size));
      // Bytes outside the supplied view must still be ignored.
      EXPECT_EQ(tester.Read(wkt.data(), valid_size), GEOARROW_OK);
    }
  }
}

TEST(WKTReaderTest, WKTReaderTestNumberBoundaries) {
  WKTNumberTester tester;
  EXPECT_EQ(tester.Read("LINESTRING ZM(1\t2\r3\n4,5 6 7 8)"), GEOARROW_OK);
  EXPECT_EQ(tester.values, (std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}));

  // A string view need not end at a NUL byte; bytes past its end are not input.
  const char bounded[] = {'P', 'O', 'I', 'N', 'T', '(', '1', ' ', '2', ')', '3'};
  EXPECT_EQ(tester.Read(bounded, 10), GEOARROW_OK);
  EXPECT_EQ(tester.values, (std::vector<double>{1, 2}));
  EXPECT_EQ(tester.Read(bounded, 7), EINVAL);
  EXPECT_STREQ(tester.error.message, "Expected whitespace at byte 7");
  EXPECT_EQ(tester.Read(bounded, 9), EINVAL);
  EXPECT_STREQ(tester.error.message, "Expected ')' at byte 9");

  const char embedded_nul[] = "POINT(1\0 2)";
  EXPECT_EQ(tester.Read(embedded_nul, sizeof(embedded_nul) - 1), EINVAL);
  EXPECT_STREQ(tester.error.message, "Expected whitespace at byte 7");

  for (const char* wkt :
       {"POINT(nan() 2)", "POINT(nan(payload) 2)", "POINT(NaN(payload) 2)"}) {
    SCOPED_TRACE(wkt);
    EXPECT_EQ(tester.Read(wkt), EINVAL);
    EXPECT_STREQ(tester.error.message, "Expected whitespace at byte 9");
  }
  EXPECT_EQ(tester.Read("POINT(-nan(payload) 2)"), EINVAL);
  EXPECT_STREQ(tester.error.message, "Expected whitespace at byte 10");
  EXPECT_EQ(tester.Read("POINT(nan nan)"), GEOARROW_OK);
  ASSERT_EQ(tester.values.size(), 2);
  EXPECT_TRUE(std::isnan(tester.values[0]));
  EXPECT_TRUE(std::isnan(tester.values[1]));
}

TEST(WKTReaderTest, WKTReaderTestBasic) {
  struct GeoArrowWKTReader reader;
  GeoArrowWKTReaderInit(&reader);
  GeoArrowWKTReaderReset(&reader);
}

TEST(WKTReaderTest, WKTReaderTestPoint) {
  WKXTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, "POINT EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT Z EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT M EMPTY");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT ZM EMPTY");

  EXPECT_WKT_ROUNDTRIP(tester, "POINT (0 1)");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT Z (0 1 2)");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT M (0 1 3)");
  EXPECT_WKT_ROUNDTRIP(tester, "POINT ZM (0 1 2 3)");

  // Extra whitespace is OK; no whitespace after POINT is OK
  EXPECT_EQ(tester.AsWKT(" POINT(0    1) "), "POINT (0 1)");

  EXPECT_THROW(tester.AsWKT("POINT A"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected '(' or 'EMPTY' at byte 6");
  EXPECT_THROW(tester.AsWKT("POINT Z A"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected '(' or 'EMPTY' at byte 8");
  EXPECT_THROW(tester.AsWKT("POINT ZA"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected '(' or 'EMPTY' at byte 6");
  EXPECT_THROW(tester.AsWKT("POINT (0)"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected whitespace at byte 8");
  EXPECT_THROW(tester.AsWKT("POINT (0 )"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 9");
  EXPECT_THROW(tester.AsWKT("POINT (0 1) shouldbetheend"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected end of input at byte 12");
}

TEST(WKTReaderTest, WKTReaderTestPointMultipleDims) {
  struct GeoArrowNativeWriter writer;
  struct GeoArrowVisitor v;
  ASSERT_EQ(GeoArrowNativeWriterInit(&writer, GEOARROW_TYPE_POINT_ZM), GEOARROW_OK);
  GeoArrowNativeWriterInitVisitor(&writer, &v);

  WKXTester tester;
  tester.ReadWKT("POINT (1 2)", &v);
  tester.ReadWKT("POINT Z (1 2 3)", &v);
  tester.ReadWKT("POINT M (1 2 3)", &v);
  tester.ReadWKT("POINT ZM (1 2 3 4)", &v);

  struct ArrowArray array_out;
  struct GeoArrowArrayView array_view;
  EXPECT_EQ(GeoArrowNativeWriterFinish(&writer, &array_out, nullptr), GEOARROW_OK);
  GeoArrowNativeWriterReset(&writer);

  ASSERT_EQ(GeoArrowArrayViewInitFromType(&array_view, GEOARROW_TYPE_POINT_ZM),
            GEOARROW_OK);
  ASSERT_EQ(GeoArrowArrayViewSetArray(&array_view, &array_out, nullptr), GEOARROW_OK);
  ASSERT_EQ(
      GeoArrowArrayViewVisitNative(&array_view, 0, array_out.length, tester.WKTVisitor()),
      GEOARROW_OK);
  array_out.release(&array_out);

  auto values = tester.WKTValues("<null value>");
  ASSERT_EQ(values.size(), 4);
  EXPECT_EQ(values[0], "POINT ZM (1 2 nan nan)");
  EXPECT_EQ(values[1], "POINT ZM (1 2 3 nan)");
  EXPECT_EQ(values[2], "POINT ZM (1 2 nan 3)");
  EXPECT_EQ(values[3], "POINT ZM (1 2 3 4)");
}

TEST(WKTReaderTest, WKTReaderTestLinestring) {
  WKXTester tester;
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

  EXPECT_THROW(tester.AsWKT("LINESTRING (0)"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected whitespace at byte 13");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 )"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 14");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1()"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected ',' at byte 15");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1,)"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 16");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1, )"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 17");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1, 1)"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected whitespace at byte 18");
  EXPECT_THROW(tester.AsWKT("LINESTRING (0 1, 1 )"), WKXTestException);
  EXPECT_EQ(tester.LastErrorMessage(), "Expected number at byte 19");
}

TEST(WKTReaderTest, WKTReaderTestPolygon) {
  WKXTester tester;
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
  WKXTester tester;
  tester.SetFlatMultipoint(true);
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
  WKXTester tester;
  tester.SetFlatMultipoint(false);
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
  WKXTester tester;

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
  WKXTester tester;
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
  WKXTester tester;
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

  WKXTester tester;
  EXPECT_WKT_ROUNDTRIP(tester, ss.str());
}

TEST(WKTReaderTest, WKTReaderTestLongCoordinates) {
  // All of the readers above use integer coordinates. This tests really
  // long coordinates that always use all 15 precision spaces.

  std::stringstream ss;
  ss << std::setprecision(15);
  ss << "LINESTRING (" << (1.0 / 3.0) << " " << (1 + (1.0 / 3.0));
  for (int i = 1; i < 10; i++) {
    ss << ", " << (i + (1.0 / 3.0)) << " " << (i + 1 + (1.0 / 3.0));
  }
  ss << ")";

  WKXTester tester;
  tester.SetPrecision(15);
  EXPECT_WKT_ROUNDTRIP(tester, ss.str());
}
