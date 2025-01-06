#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "array_reader.hpp"
#include "array_util.hpp"
#include "array_writer.hpp"

#include "wkx_testing.hpp"

using geoarrow::array_util::CoordSequence;
using geoarrow::array_util::ListSequence;
using geoarrow::array_util::XY;

TEST(GeoArrowHppTest, CoordXY) {
  XY coord{0, 1};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_TRUE(std::isnan(coord.z()));
  EXPECT_TRUE(std::isnan(coord.m()));
}

TEST(GeoArrowHppTest, CoordXYZ) {
  geoarrow::array_util::XYZ coord{0, 1, 2};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_EQ(coord.z(), 2);
  EXPECT_TRUE(std::isnan(coord.m()));
}

TEST(GeoArrowHppTest, CoordXYM) {
  geoarrow::array_util::XYM coord{0, 1, 2};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_TRUE(std::isnan(coord.z()));
  EXPECT_EQ(coord.m(), 2);
}

TEST(GeoArrowHppTest, CoordXYZM) {
  geoarrow::array_util::XYZM coord{0, 1, 2, 3};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_EQ(coord.z(), 2);
  EXPECT_EQ(coord.m(), 3);
}

TEST(GeoArrowHppTest, BoxXY) {
  geoarrow::array_util::BoxXY coord{0, 1, 2, 3};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord.xmin(), 0);
  EXPECT_EQ(coord.ymin(), 1);
  EXPECT_EQ(coord.zmin(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(coord.mmin(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(coord.xmax(), 2);
  EXPECT_EQ(coord.ymax(), 3);
  EXPECT_EQ(coord.zmax(), -std::numeric_limits<double>::infinity());
  EXPECT_EQ(coord.mmax(), -std::numeric_limits<double>::infinity());

  geoarrow::array_util::XY expected_lower{0, 1};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  geoarrow::array_util::XY expected_upper{2, 3};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, BoxXYZ) {
  geoarrow::array_util::BoxXYZ coord{0, 1, 2, 3, 4, 5};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord[3], 3);
  EXPECT_EQ(coord[4], 4);
  EXPECT_EQ(coord.xmin(), 0);
  EXPECT_EQ(coord.ymin(), 1);
  EXPECT_EQ(coord.zmin(), 2);
  EXPECT_EQ(coord.mmin(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(coord.xmax(), 3);
  EXPECT_EQ(coord.ymax(), 4);
  EXPECT_EQ(coord.zmax(), 5);
  EXPECT_EQ(coord.mmax(), -std::numeric_limits<double>::infinity());

  geoarrow::array_util::XYZ expected_lower{0, 1, 2};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  geoarrow::array_util::XYZ expected_upper{3, 4, 5};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, BoxXYM) {
  geoarrow::array_util::BoxXYM coord{0, 1, 2, 3, 4, 5};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord[3], 3);
  EXPECT_EQ(coord[4], 4);
  EXPECT_EQ(coord.xmin(), 0);
  EXPECT_EQ(coord.ymin(), 1);
  EXPECT_EQ(coord.zmin(), std::numeric_limits<double>::infinity());
  EXPECT_EQ(coord.mmin(), 2);
  EXPECT_EQ(coord.xmax(), 3);
  EXPECT_EQ(coord.ymax(), 4);
  EXPECT_EQ(coord.zmax(), -std::numeric_limits<double>::infinity());
  EXPECT_EQ(coord.mmax(), 5);

  geoarrow::array_util::XYM expected_lower{0, 1, 2};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  geoarrow::array_util::XYM expected_upper{3, 4, 5};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, BoxXYZM) {
  geoarrow::array_util::BoxXYZM coord{0, 1, 2, 3, 4, 5, 6, 7};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord[3], 3);
  EXPECT_EQ(coord[4], 4);
  EXPECT_EQ(coord[5], 5);
  EXPECT_EQ(coord[6], 6);
  EXPECT_EQ(coord.xmin(), 0);
  EXPECT_EQ(coord.ymin(), 1);
  EXPECT_EQ(coord.zmin(), 2);
  EXPECT_EQ(coord.mmin(), 3);
  EXPECT_EQ(coord.xmax(), 4);
  EXPECT_EQ(coord.ymax(), 5);
  EXPECT_EQ(coord.zmax(), 6);
  EXPECT_EQ(coord.mmax(), 7);

  geoarrow::array_util::XYZM expected_lower{0, 1, 2, 3};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  geoarrow::array_util::XYZM expected_upper{4, 5, 6, 7};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, RandomAccessIterator) {
  TestCoords coords{{0, 1, 2, 3, 4, 5}, {6, 7, 8, 9, 10, 11}};
  CoordSequence<XY> sequence;
  geoarrow::array_util::internal::InitFromCoordView(&sequence, coords.view());

  auto it = sequence.begin();
  EXPECT_EQ(sequence.end() - it, sequence.size());
  EXPECT_EQ(it - sequence.end(), -sequence.size());

  ++it;
  ASSERT_EQ(sequence.end() - it, (sequence.size() - 1));
  it += 2;
  ASSERT_EQ(sequence.end() - it, (sequence.size() - 3));
  it -= 2;
  ASSERT_EQ(sequence.end() - it, (sequence.size() - 1));
  --it;
  ASSERT_EQ(it, sequence.begin());

  ASSERT_TRUE(sequence.begin() == sequence.begin());
  ASSERT_FALSE(sequence.begin() != sequence.begin());
  ASSERT_TRUE(sequence.begin() >= sequence.begin());
  ASSERT_TRUE(sequence.begin() <= sequence.begin());
  ASSERT_FALSE(sequence.begin() > sequence.begin());
  ASSERT_FALSE(sequence.begin() < sequence.begin());

  ++it;
  ASSERT_TRUE(it > sequence.begin());
  ASSERT_TRUE(it >= sequence.begin());
  ASSERT_TRUE(sequence.begin() < it);
  ASSERT_TRUE(sequence.begin() <= it);
  ASSERT_FALSE(it < sequence.begin());
  ASSERT_FALSE(it <= sequence.begin());
  ASSERT_FALSE(sequence.begin() > it);
  ASSERT_FALSE(sequence.begin() >= it);
}

TEST(GeoArrowHppTest, IterateCoords) {
  TestCoords coords{{0, 1, 2}, {5, 6, 7}};

  CoordSequence<XY> sequence;
  geoarrow::array_util::internal::InitFromCoordView(&sequence, coords.view());

  ASSERT_EQ(sequence.size(), 3);
  XY last_coord{2, 7};
  ASSERT_EQ(sequence.coord(2), last_coord);

  std::vector<XY> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));
  EXPECT_THAT(sequence.Slice(1, 1), ::testing::ElementsAre(XY{1, 6}));

  // Check dbegin() iteration
  coords_vec.clear();
  const double* x = sequence.dbegin(0);
  const double* y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    x += sequence.stride;
    y += sequence.stride;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check dbegin() iteration with offset
  coords_vec.clear();
  sequence = sequence.Slice(1, 2);
  x = sequence.dbegin(0);
  y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    x += sequence.stride;
    y += sequence.stride;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{1, 6}, XY{2, 7}));
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

  // Check dbegin() iteration
  coords_vec.clear();
  const double* x = sequence.dbegin(0);
  const double* y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    x += sequence.stride;
    y += sequence.stride;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check dbegin() iteration with offset
  coords_vec.clear();
  sequence = sequence.Slice(1, 2);
  x = sequence.dbegin(0);
  y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    x += sequence.stride;
    y += sequence.stride;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{1, 6}, XY{2, 7}));
}

TEST(GeoArrowHppTest, IterateNestedCoords) {
  TestCoords coords{{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}};
  std::vector<int32_t> offsets{0, 3, 5};

  ListSequence<CoordSequence<XY>> sequences;
  sequences.offset = 0;
  sequences.length = 2;
  sequences.offsets = offsets.data();
  geoarrow::array_util::internal::InitFromCoordView(&sequences.child, coords.view());

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

TEST(GeoArrowHppTest, CoordTraits) {
  EXPECT_EQ(geoarrow::array_util::PointArray<geoarrow::array_util::XY>::dimensions,
            GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(geoarrow::array_util::PointArray<geoarrow::array_util::XYZ>::dimensions,
            GEOARROW_DIMENSIONS_XYZ);
  EXPECT_EQ(geoarrow::array_util::PointArray<geoarrow::array_util::XYM>::dimensions,
            GEOARROW_DIMENSIONS_XYM);
  EXPECT_EQ(geoarrow::array_util::PointArray<geoarrow::array_util::XYZM>::dimensions,
            GEOARROW_DIMENSIONS_XYZM);
}

TEST(GeoArrowHppTest, ArrayNullness) {
  geoarrow::array_util::PointArray<XY> native_array;

  // With a set bitmap, ensure the formula is used
  uint8_t validity_byte = 0b0000101;
  native_array.validity = &validity_byte;
  EXPECT_TRUE(native_array.is_valid(0));
  EXPECT_FALSE(native_array.is_valid(1));
  EXPECT_TRUE(native_array.is_valid(2));
  EXPECT_FALSE(native_array.is_valid(3));

  EXPECT_FALSE(native_array.is_null(0));
  EXPECT_TRUE(native_array.is_null(1));
  EXPECT_FALSE(native_array.is_null(2));
  EXPECT_TRUE(native_array.is_null(3));

  // Make sure value offset is applied
  EXPECT_FALSE(native_array.Slice(1, 1).is_valid(0));
  EXPECT_TRUE(native_array.Slice(2, 1).is_valid(0));
  EXPECT_FALSE(native_array.Slice(3, 1).is_valid(0));

  // With a null bitmap, elements are never null
  native_array.validity = nullptr;
  EXPECT_TRUE(native_array.is_valid(0));
  EXPECT_TRUE(native_array.is_valid(1));
  EXPECT_TRUE(native_array.is_valid(2));
  EXPECT_TRUE(native_array.is_valid(3));

  EXPECT_FALSE(native_array.is_null(0));
  EXPECT_FALSE(native_array.is_null(1));
  EXPECT_FALSE(native_array.is_null(2));
  EXPECT_FALSE(native_array.is_null(3));
}

TEST(GeoArrowHppTest, SetArrayBox) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_LINESTRING);
  WKXTester tester;
  tester.ReadWKT("LINESTRING (0 1, 2 3)", writer.visitor());
  tester.ReadWKT("LINESTRING (4 5, 6 7)", writer.visitor());
  tester.ReadWKT("LINESTRING (8 9, 10 11, 12 13)", writer.visitor());

  struct ArrowSchema schema_feat;
  geoarrow::Linestring().InitSchema(&schema_feat);
  struct ArrowArray array_feat;
  writer.Finish(&array_feat);

  // Create a box array using the input linestrings
  struct GeoArrowKernel kernel;
  struct ArrowSchema schema;
  struct ArrowArray array;
  ASSERT_EQ(GeoArrowKernelInit(&kernel, "box", nullptr), GEOARROW_OK);
  ASSERT_EQ(kernel.start(&kernel, &schema_feat, nullptr, &schema, nullptr), GEOARROW_OK);
  ASSERT_EQ(kernel.push_batch(&kernel, &array_feat, &array, nullptr), GEOARROW_OK);
  kernel.release(&kernel);
  ArrowSchemaRelease(&schema_feat);
  ArrowArrayRelease(&array_feat);

  geoarrow::ArrayReader reader(&schema);
  reader.SetArray(&array);
  ArrowSchemaRelease(&schema);

  geoarrow::array_util::BoxArray<XY> native_array;
  ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

  using geoarrow::array_util::BoxXY;
  std::vector<BoxXY> boxes;
  for (const auto& coord : native_array.value) {
    boxes.push_back(coord);
  }

  EXPECT_THAT(boxes, ::testing::ElementsAre(BoxXY{0, 1, 2, 3}, BoxXY{4, 5, 6, 7},
                                            BoxXY{8, 9, 12, 13}));

  EXPECT_THAT(native_array.LowerBound().value,
              ::testing::ElementsAre(XY{0, 1}, XY{4, 5}, XY{8, 9}));
  EXPECT_THAT(native_array.UpperBound().value,
              ::testing::ElementsAre(XY{2, 3}, XY{6, 7}, XY{12, 13}));
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

    geoarrow::array_util::PointArray<XY> native_array;
    ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

    std::vector<XY> points;
    for (const auto& coord : native_array.value) {
      points.push_back(coord);
    }

    EXPECT_THAT(points, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}));
    EXPECT_THAT(native_array.Coords(),
                ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}));
    EXPECT_THAT(native_array.Slice(1, 1).Coords(), ::testing::ElementsAre(XY{2, 3}));
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

    geoarrow::array_util::LinestringArray<XY> native_array;
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

    EXPECT_THAT(native_array.Coords(),
                ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7}, XY{8, 9},
                                       XY{10, 11}, XY{12, 13}));
    EXPECT_THAT(native_array.Slice(1, 1).Coords(),
                ::testing::ElementsAre(XY{4, 5}, XY{6, 7}));
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

    geoarrow::array_util::MultiLinestringArray<XY> native_array;
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
    EXPECT_THAT(native_array.Coords(),
                ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7}, XY{8, 9},
                                       XY{10, 11}, XY{12, 13}, XY{15, 16}, XY{17, 18}));
    EXPECT_THAT(native_array.Slice(1, 1).Coords(),
                ::testing::ElementsAre(XY{4, 5}, XY{6, 7}));
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

    geoarrow::array_util::MultiPolygonArray<XY> native_array;
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
    EXPECT_THAT(native_array.Coords(),
                ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7}, XY{8, 9},
                                       XY{10, 11}, XY{12, 13}, XY{15, 16}, XY{17, 18}));
    EXPECT_THAT(native_array.Slice(1, 1).Coords(),
                ::testing::ElementsAre(XY{4, 5}, XY{6, 7}));
  }
}
