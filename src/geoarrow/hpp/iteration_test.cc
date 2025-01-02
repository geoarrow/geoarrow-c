#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "iteration.hpp"

#include "wkx_testing.hpp"

using Coord2 = std::array<double, 2>;
using geoarrow::array::CoordSequence;
using geoarrow::array::Nested;

TEST(GeoArrowHppTest, IterateCoords) {
  TestCoords coords{{0, 1, 2}, {5, 6, 7}};

  CoordSequence<Coord2> sequence{0, 3, coords.view()};

  std::vector<Coord2> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec,
              ::testing::ElementsAre(Coord2{0, 5}, Coord2{1, 6}, Coord2{2, 7}));
}

TEST(GeoArrowHppTest, IterateNestedCoords) {
  TestCoords coords{{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}};
  std::vector<int32_t> offsets{0, 3, 5};

  Nested<CoordSequence<Coord2>> sequences;
  sequences.offset = 0;
  sequences.length = 2;
  sequences.offsets = offsets.data();
  sequences.child = {0, 5, coords.view()};

  std::vector<std::vector<Coord2>> elements;
  for (const auto& sequence : sequences) {
    std::vector<Coord2> coords;
    for (const auto& coord : sequence) {
      coords.push_back(coord);
    }
    elements.push_back(std::move(coords));
  }

  EXPECT_THAT(elements, ::testing::ElementsAre(
                            std::vector<Coord2>{Coord2{0, 5}, Coord2{1, 6}, Coord2{2, 7}},
                            std::vector<Coord2>{Coord2{3, 8}, Coord2{4, 9}}));
}
