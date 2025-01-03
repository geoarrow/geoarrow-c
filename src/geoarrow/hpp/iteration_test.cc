#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "array_reader.hpp"
#include "array_writer.hpp"
#include "iteration.hpp"

#include "wkx_testing.hpp"

using geoarrow::array::CoordSequence;
using geoarrow::array::ListSequence;
using geoarrow::array::XY;

TEST(GeoArrowHppTest, IterateCoords) {
  TestCoords coords{{0, 1, 2}, {5, 6, 7}};

  CoordSequence<XY> sequence;
  geoarrow::array::internal::InitFromCoordView(&sequence, coords.view());

  ASSERT_EQ(sequence.size(), 3);
  XY last_coord{2, 7};
  ASSERT_EQ(sequence[2], last_coord);

  std::vector<XY> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));
}

TEST(GeoArrowHppTest, IterateCoordsInterleaved) {
  std::vector<double> coords{0, 5, 1, 6, 2, 7};
  CoordSequence<XY> sequence;
  sequence.length = 3;
  sequence.offset = 0;
  sequence.stride = 2;
  sequence.values = {coords.data(), coords.data() + 1};

  std::vector<XY> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));
}

TEST(GeoArrowHppTest, IterateNestedCoords) {
  TestCoords coords{{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}};
  std::vector<int32_t> offsets{0, 3, 5};

  ListSequence<CoordSequence<XY>> sequences;
  sequences.offset = 0;
  sequences.length = 2;
  sequences.offsets = offsets.data();
  geoarrow::array::internal::InitFromCoordView(&sequences.child, coords.view());

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
  for (const auto type : {GEOARROW_TYPE_POINT, GEOARROW_TYPE_INTERLEAVED_POINT,
                          GEOARROW_TYPE_POINT_Z, GEOARROW_TYPE_INTERLEAVED_POINT_Z,
                          GEOARROW_TYPE_POINT_M, GEOARROW_TYPE_INTERLEAVED_POINT_M,
                          GEOARROW_TYPE_POINT_ZM, GEOARROW_TYPE_INTERLEAVED_POINT_ZM}) {
    SCOPED_TRACE(geoarrow::GeometryDataType::Make(type).ToString());
    geoarrow::ArrayWriter writer(type);
    WKXTester tester;
    tester.ReadWKT("POINT (0 1)", writer.visitor());
    tester.ReadWKT("POINT (2 3)", writer.visitor());
    tester.ReadWKT("POINT (4 5)", writer.visitor());

    struct ArrowArray array;
    writer.Finish(&array);

    geoarrow::ArrayReader reader(type);
    reader.SetArray(&array);

    geoarrow::array::PointArray<XY> native_array;
    ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

    std::vector<XY> points;
    for (const auto& coord : native_array.value) {
      points.push_back(coord);
    }

    EXPECT_THAT(points, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}));
  }
}

TEST(GeoArrowHppTest, SetArrayLinestring) {
  for (const auto type :
       {GEOARROW_TYPE_LINESTRING, GEOARROW_TYPE_INTERLEAVED_LINESTRING,
        GEOARROW_TYPE_LINESTRING_Z, GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z,
        GEOARROW_TYPE_LINESTRING_M, GEOARROW_TYPE_INTERLEAVED_LINESTRING_M,
        GEOARROW_TYPE_LINESTRING_ZM, GEOARROW_TYPE_INTERLEAVED_LINESTRING_ZM}) {
    SCOPED_TRACE(geoarrow::GeometryDataType::Make(type).ToString());
    geoarrow::ArrayWriter writer(type);
    WKXTester tester;
    tester.ReadWKT("LINESTRING (0 1, 2 3)", writer.visitor());
    tester.ReadWKT("LINESTRING (4 5, 6 7)", writer.visitor());
    tester.ReadWKT("LINESTRING (8 9, 10 11, 12 13)", writer.visitor());

    struct ArrowArray array;
    writer.Finish(&array);

    geoarrow::ArrayReader reader(type);
    reader.SetArray(&array);

    geoarrow::array::LinestringArray<XY> native_array;
    ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

    std::vector<std::vector<XY>> linestrings;
    for (const auto& linestring : native_array.value) {
      std::vector<XY> coords;
      for (const auto& coord : linestring) {
        coords.push_back(coord);
      }
      linestrings.push_back(std::move(coords));
    }

    EXPECT_THAT(linestrings, ::testing::ElementsAre(
                                 std::vector<XY>{XY{0, 1}, XY{2, 3}},
                                 std::vector<XY>{XY{4, 5}, XY{6, 7}},
                                 std::vector<XY>{XY{8, 9}, XY{10, 11}, XY{12, 13}}));
  }
}

TEST(GeoArrowHppTest, SetArrayMultiLinestring) {
  for (const auto type :
       {GEOARROW_TYPE_MULTILINESTRING, GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING,
        GEOARROW_TYPE_MULTILINESTRING_Z, GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_Z,
        GEOARROW_TYPE_MULTILINESTRING_M, GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_M,
        GEOARROW_TYPE_MULTILINESTRING_ZM, GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_ZM}) {
    SCOPED_TRACE(geoarrow::GeometryDataType::Make(type).ToString());
    geoarrow::ArrayWriter writer(type);
    WKXTester tester;
    tester.ReadWKT("MULTILINESTRING ((0 1, 2 3))", writer.visitor());
    tester.ReadWKT("MULTILINESTRING ((4 5, 6 7))", writer.visitor());
    tester.ReadWKT("MULTILINESTRING ((8 9, 10 11, 12 13), (15 16, 17 18))",
                   writer.visitor());

    struct ArrowArray array;
    writer.Finish(&array);

    geoarrow::ArrayReader reader(type);
    reader.SetArray(&array);

    geoarrow::array::MultiLinestringArray<XY> native_array;
    ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

    std::vector<std::vector<std::vector<XY>>> multilinestrings;
    for (const auto& multilinestring : native_array.value) {
      std::vector<std::vector<XY>> linestrings;
      for (const auto& linestring : multilinestring) {
        std::vector<XY> coords;
        for (const auto& coord : linestring) {
          coords.push_back(coord);
        }
        linestrings.push_back(std::move(coords));
      }
      multilinestrings.push_back(std::move(linestrings));
    }

    EXPECT_THAT(multilinestrings,
                ::testing::ElementsAre(
                    std::vector<std::vector<XY>>{{XY{0, 1}, XY{2, 3}}},
                    std::vector<std::vector<XY>>{{XY{4, 5}, XY{6, 7}}},
                    std::vector<std::vector<XY>>{{XY{8, 9}, XY{10, 11}, XY{12, 13}},
                                                 {XY{15, 16}, XY{17, 18}}}));
  }
}

TEST(GeoArrowHppTest, SetArrayMultiPolygon) {
  for (const auto type :
       {GEOARROW_TYPE_MULTIPOLYGON, GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON,
        GEOARROW_TYPE_MULTIPOLYGON_Z, GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_Z,
        GEOARROW_TYPE_MULTIPOLYGON_M, GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_M,
        GEOARROW_TYPE_MULTIPOLYGON_ZM, GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_ZM}) {
    SCOPED_TRACE(geoarrow::GeometryDataType::Make(type).ToString());
    geoarrow::ArrayWriter writer(type);
    WKXTester tester;
    tester.ReadWKT("MULTIPOLYGON (((0 1, 2 3)))", writer.visitor());
    tester.ReadWKT("MULTIPOLYGON (((4 5, 6 7)))", writer.visitor());
    tester.ReadWKT("MULTIPOLYGON (((8 9, 10 11, 12 13), (15 16, 17 18)))",
                   writer.visitor());

    struct ArrowArray array;
    writer.Finish(&array);

    geoarrow::ArrayReader reader(type);
    reader.SetArray(&array);

    geoarrow::array::MultiPolygonArray<XY> native_array;
    ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

    std::vector<std::vector<std::vector<std::vector<XY>>>> multipolygons;
    for (const auto& multipolygon : native_array.value) {
      std::vector<std::vector<std::vector<XY>>> polygons;
      for (const auto& polygon : multipolygon) {
        std::vector<std::vector<XY>> rings;
        for (const auto& ring : polygon) {
          std::vector<XY> coords;
          for (const auto& coord : ring) {
            coords.push_back(coord);
          }
          rings.push_back(std::move(coords));
        }
        polygons.push_back(std::move(rings));
      }
      multipolygons.push_back(std::move(polygons));
    }

    EXPECT_THAT(multipolygons,
                ::testing::ElementsAre(
                    std::vector<std::vector<std::vector<XY>>>{{{XY{0, 1}, XY{2, 3}}}},
                    std::vector<std::vector<std::vector<XY>>>{{{XY{4, 5}, XY{6, 7}}}},
                    std::vector<std::vector<std::vector<XY>>>{
                        {{XY{8, 9}, XY{10, 11}, XY{12, 13}}, {XY{15, 16}, XY{17, 18}}}}));
  }
}
