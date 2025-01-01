
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "array_reader.hpp"
#include "array_writer.hpp"

#include "wkx_testing.hpp"

TEST(GeoArrowHppTest, ArrayWriterByBuffer) {
  geoarrow::ArrayWriter writer(geoarrow::Point());

  // Check SetBufferWrapped() overload with a movable C++ object + arbitrary buffer view
  // (make it long enough that it keeps the same allocation when moved)
  std::vector<uint8_t> validity(1024);
  std::memset(validity.data(), 0, validity.size());
  validity[0] = 0b00001111;
  writer.builder().SetBufferWrapped(0, std::move(validity));

  // Check SetBufferWrapped() overload with a sequence
  writer.builder().SetBufferWrapped(1, std::vector<double>{0, 1, 2, 3});

  // Check buffer appender
  std::vector<double> ys{4, 5, 6, 7};
  writer.builder().AppendToBuffer(2, ys);

  // Make sure we can't also use the visitor
  EXPECT_THROW(writer.visitor(), geoarrow::Exception);

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

  geoarrow::ArrayWriter writer(geoarrow::Linestring());
  writer.builder().AppendToOffsetBuffer(0, std::vector<int32_t>{0, 2, 5});
  writer.builder().AppendCoords(coords.view(), GEOARROW_DIMENSIONS_XY, 0, 5);

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

  EXPECT_THROW(writer.builder(), geoarrow::Exception);

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
