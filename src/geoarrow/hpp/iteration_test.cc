#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "array_reader.hpp"
#include "array_writer.hpp"
#include "iteration.hpp"

#include "wkx_testing.hpp"

using geoarrow::array::CoordSequence;
using geoarrow::array::Nested;
using geoarrow::array::XY;

TEST(GeoArrowHppTest, IterateCoords) {
  TestCoords coords{{0, 1, 2}, {5, 6, 7}};

  CoordSequence<XY> sequence{0, 3, coords.view()};

  ASSERT_EQ(sequence.size(), 3);
  XY last_coord{2, 7};
  ASSERT_EQ(sequence[2], last_coord);

  std::vector<XY> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));
}

TEST(GeoArrowHppTest, IterateNestedCoords) {
  TestCoords coords{{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}};
  std::vector<int32_t> offsets{0, 3, 5};

  Nested<CoordSequence<XY>> sequences;
  sequences.offset = 0;
  sequences.length = 2;
  sequences.offsets = offsets.data();
  sequences.child = {0, 5, coords.view()};

  std::vector<std::vector<XY>> elements;
  for (const auto& sequence : sequences) {
    std::vector<XY> coords;
    for (const auto& coord : sequence) {
      coords.push_back(coord);
    }
    elements.push_back(std::move(coords));
  }

  EXPECT_THAT(elements,
              ::testing::ElementsAre(std::vector<XY>{XY{0, 5}, XY{1, 6}, XY{2, 7}},
                                     std::vector<XY>{XY{3, 8}, XY{4, 9}}));
}

TEST(GeoArrowHppTest, SetArrayPoint) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_POINT);
  WKXTester tester;
  tester.ReadWKT("POINT (0 1)", writer.visitor());
  tester.ReadWKT("POINT (2 3)", writer.visitor());
  tester.ReadWKT("POINT (4 5)", writer.visitor());

  struct ArrowArray array;
  writer.Finish(&array);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_POINT);
  reader.SetArray(&array);

  geoarrow::array::PointArray<XY> point_array;
  point_array.Init(reader.View().array_view());

  std::vector<XY> coords_vec;
  for (const auto& coord : point_array.value) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}));
}
