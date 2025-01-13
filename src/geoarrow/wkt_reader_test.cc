
#include <gtest/gtest.h>

#include <geoarrow.h>
#include "nanoarrow/nanoarrow.h"

#include "wkx_testing.hpp"

#define EXPECT_WKT_ROUNDTRIP(tester_, wkt_) EXPECT_EQ(tester_.AsWKT(wkt_), wkt_)

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
