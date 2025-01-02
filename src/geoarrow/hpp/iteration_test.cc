#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "iteration.hpp"

#include "wkx_testing.hpp"

using Coord2 = std::array<double, 2>;

TEST(GeoArrowHppTest, IterateCoords) {
  TestCoords coords{{0, 1, 2}, {5, 6, 7}};

  geoarrow::array::CoordSequence<Coord2> sequence{0, 3, coords.view()};

  std::vector<Coord2> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec,
              ::testing::ElementsAre(Coord2{0, 5}, Coord2{1, 6}, Coord2{2, 7}));
}
