
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.hpp"

#include "geoarrow/wkx_testing.hpp"

TEST(GeoArrowHppTest, ArrayWriterByBuffer) {
  geoarrow::ArrayBuilder writer(geoarrow::Point());

  // Check SetBufferWrapped() overload with a movable C++ object + arbitrary buffer view
  // (make it long enough that it keeps the same allocation when moved)
  std::vector<uint8_t> validity(1024);
  std::memset(validity.data(), 0, validity.size());
  validity[0] = 0b00001111;
  writer.SetBufferWrapped(0, std::move(validity));

  // Check SetBufferWrapped() overload with a sequence
  writer.SetBufferWrapped(1, std::vector<double>{0, 1, 2, 3});

  // Check buffer appender
  std::vector<double> ys{4, 5, 6, 7};
  writer.AppendToBuffer(2, ys);

  struct ArrowArray array;
  writer.Finish(&array);
  ASSERT_EQ(array.length, 4);
  ASSERT_EQ(array.n_children, 2);

  geoarrow::ArrayReader reader(geoarrow::Point());
  reader.SetArray(&array);
  EXPECT_EQ(reader.View().array_view()->schema_view.type, GEOARROW_TYPE_POINT);
  EXPECT_EQ(reader.View().array_view()->validity_bitmap[0], 0b00001111);
  EXPECT_EQ(reader.View().array_view()->coords.values[0][0], 0);
  EXPECT_EQ(reader.View().array_view()->coords.values[1][0], 4);
}

TEST(GeoArrowHppTest, ArrayWriterByOffsetAndCoords) {
  TestCoords coords({0, 1, 2, 3, 4}, {5, 6, 7, 8, 9});

  geoarrow::ArrayBuilder writer(geoarrow::Linestring());
  writer.AppendToOffsetBuffer(0, std::vector<int32_t>{0, 2, 5});
  writer.AppendCoords(coords.view(), GEOARROW_DIMENSIONS_XY, 0, 5);

  struct ArrowArray array;
  writer.Finish(&array);
  ASSERT_EQ(array.length, 2);
  ASSERT_EQ(array.n_children, 1);

  WKXTester tester;
  geoarrow::ArrayReader reader(geoarrow::Linestring());
  reader.SetArray(&array);
  ASSERT_EQ(reader.Visit(tester.WKTVisitor(), 0, array.length), GEOARROW_OK);

  EXPECT_THAT(tester.WKTValues(), ::testing::ElementsAre("LINESTRING (0 5, 1 6)",
                                                         "LINESTRING (2 7, 3 8, 4 9)"));
}

TEST(GeoArrowHppTest, ArrayWriterByVisitor) {
  WKXTester tester;
  geoarrow::ArrayWriter writer(geoarrow::Point());
  tester.ReadWKT("POINT (0 4)", writer.visitor());

  struct ArrowArray array;
  writer.Finish(&array);
  ASSERT_EQ(array.length, 1);
  ASSERT_EQ(array.n_children, 2);

  geoarrow::ArrayReader reader(geoarrow::Point());
  reader.SetArray(&array);
  EXPECT_EQ(reader.View().array_view()->schema_view.type, GEOARROW_TYPE_POINT);
  EXPECT_EQ(reader.View().array_view()->coords.values[0][0], 0);
  EXPECT_EQ(reader.View().array_view()->coords.values[1][0], 4);
}

TEST(GeoArrowHppTest, ArrayReaderWriterGeometry) {
  WKXTester tester;
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_WKB);
  writer.Append(GeoArrowGeometryAsView(&tester.AsGeometry("POINT (1 2)")));
  writer.AppendNull();
  ArrowArray array;
  writer.Finish(&array);
  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array);
  GeoArrowGeometry geom;
  ASSERT_EQ(GeoArrowGeometryInit(&geom), GEOARROW_OK);
  geoarrow::ArrayWriter output(GEOARROW_TYPE_WKT);
  for (int64_t i = 0; i < 2; i++) {
    ASSERT_EQ(reader.Read(i, &geom), GEOARROW_OK);
    output.Append(GeoArrowGeometryAsView(&geom));
  }
  output.Finish(&array);
  geoarrow::ArrayReader result(GEOARROW_TYPE_WKT);
  result.SetArray(&array);
  ASSERT_EQ(result.Visit(tester.WKTVisitor(), 0, 2), GEOARROW_OK);
  EXPECT_EQ(tester.WKTValues("null"), (std::vector<std::string>{"POINT (1 2)", "null"}));
  GeoArrowGeometryReset(&geom);
}

TEST(GeoArrowHppTest, ArrayReaderGeometryCallback) {
  WKXTester tester;
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_WKB);
  writer.Append(GeoArrowGeometryAsView(&tester.AsGeometry("POINT (1 2)")));
  writer.AppendNull();
  ArrowArray array;
  writer.Finish(&array);
  geoarrow::ArrayReader reader(GEOARROW_TYPE_WKB);
  reader.SetArray(&array);
  int calls = 0;
  auto callback = [](GeoArrowGeometryView geom, void* data, GeoArrowError*) {
    auto count = static_cast<int*>(data);
    EXPECT_EQ(geom.root->geometry_type, *count == 0 ? GEOARROW_GEOMETRY_TYPE_POINT
                                                    : GEOARROW_GEOMETRY_TYPE_GEOMETRY);
    ++*count;
    return GEOARROW_OK;
  };
  EXPECT_EQ(reader.VisitGeometry(callback, &calls, 0, 2), GEOARROW_OK);
  EXPECT_EQ(calls, 2);
}
