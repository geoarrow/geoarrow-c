#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.hpp"

#include "geoarrow/wkx_testing.hpp"

using geoarrow::array_util::CoordCast;
using geoarrow::array_util::CoordSequence;
using geoarrow::array_util::ListSequence;
using geoarrow::array_util::UnalignedCoordSequence;
using XY = geoarrow::array_util::XY<double>;
using XYZ = geoarrow::array_util::XYZ<double>;
using XYM = geoarrow::array_util::XYM<double>;
using XYZM = geoarrow::array_util::XYZM<double>;
using BoxXY = XY::box_type;
using BoxXYZ = XYZ::box_type;
using BoxXYM = XYM::box_type;
using BoxXYZM = XYZM::box_type;

TEST(GeoArrowHppTest, CoordXY) {
  XY coord{0, 1};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_TRUE(std::isnan(coord.z()));
  EXPECT_TRUE(std::isnan(coord.m()));

  EXPECT_EQ((CoordCast<XY, XY>(coord)), coord);

  auto xyz = CoordCast<XY, XYZ>(coord);
  EXPECT_EQ(xyz.x(), 0);
  EXPECT_EQ(xyz.y(), 1);
  EXPECT_TRUE(std::isnan(xyz.z()));

  auto xym = CoordCast<XY, XYM>(coord);
  EXPECT_EQ(xym.x(), 0);
  EXPECT_EQ(xym.y(), 1);
  EXPECT_TRUE(std::isnan(xyz.z()));

  auto xyzm = CoordCast<XY, XYZM>(coord);
  EXPECT_EQ(xyzm.x(), 0);
  EXPECT_EQ(xyzm.y(), 1);
  EXPECT_TRUE(std::isnan(xyzm.z()));
  EXPECT_TRUE(std::isnan(xyzm.m()));
}

TEST(GeoArrowHppTest, CoordXYZ) {
  XYZ coord{0, 1, 2};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_EQ(coord.z(), 2);
  EXPECT_TRUE(std::isnan(coord.m()));

  EXPECT_EQ((CoordCast<XYZ, XY>(coord)), (XY{0, 1}));
  EXPECT_EQ((CoordCast<XYZ, XYZ>(coord)), coord);
  auto xyzm = CoordCast<XYZ, XYZM>(coord);
  EXPECT_EQ(xyzm.x(), 0);
  EXPECT_EQ(xyzm.y(), 1);
  EXPECT_EQ(coord.z(), 2);
  EXPECT_TRUE(std::isnan(xyzm.m()));
}

TEST(GeoArrowHppTest, CoordXYM) {
  XYM coord{0, 1, 2};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_TRUE(std::isnan(coord.z()));
  EXPECT_EQ(coord.m(), 2);

  EXPECT_EQ((CoordCast<XYM, XY>(coord)), (XY{0, 1}));
  EXPECT_EQ((CoordCast<XYM, XYM>(coord)), coord);
  auto xyzm = CoordCast<XYM, XYZM>(coord);
  EXPECT_EQ(xyzm.x(), 0);
  EXPECT_EQ(xyzm.y(), 1);
  EXPECT_TRUE(std::isnan(xyzm.z()));
  EXPECT_EQ(coord.m(), 2);
}

TEST(GeoArrowHppTest, CoordXYZM) {
  XYZM coord{0, 1, 2, 3};
  EXPECT_EQ(coord[0], 0);
  EXPECT_EQ(coord[1], 1);
  EXPECT_EQ(coord[2], 2);
  EXPECT_EQ(coord.x(), 0);
  EXPECT_EQ(coord.y(), 1);
  EXPECT_EQ(coord.z(), 2);
  EXPECT_EQ(coord.m(), 3);

  EXPECT_EQ((CoordCast<XYZM, XY>(coord)), (XY{0, 1}));
  EXPECT_EQ((CoordCast<XYZM, XYZ>(coord)), (XYZ{0, 1, 2}));
  EXPECT_EQ((CoordCast<XYZM, XYM>(coord)), (XYM{0, 1, 3}));
  EXPECT_EQ((CoordCast<XYZM, XYZM>(coord)), coord);
}

TEST(GeoArrowHppTest, BoxXY) {
  BoxXY coord{0, 1, 2, 3};
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

  XY expected_lower{0, 1};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  XY expected_upper{2, 3};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, BoxXYZ) {
  BoxXYZ coord{0, 1, 2, 3, 4, 5};
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

  XYZ expected_lower{0, 1, 2};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  XYZ expected_upper{3, 4, 5};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, BoxXYM) {
  BoxXYM coord{0, 1, 2, 3, 4, 5};
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

  XYM expected_lower{0, 1, 2};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  XYM expected_upper{3, 4, 5};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, BoxXYZM) {
  BoxXYZM coord{0, 1, 2, 3, 4, 5, 6, 7};
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

  XYZM expected_lower{0, 1, 2, 3};
  EXPECT_EQ(coord.lower_bound(), expected_lower);
  XYZM expected_upper{4, 5, 6, 7};
  EXPECT_EQ(coord.upper_bound(), expected_upper);
}

TEST(GeoArrowHppTest, RandomAccessIterator) {
  TestCoords coords{{0, 1, 2, 3, 4, 5}, {6, 7, 8, 9, 10, 11}};
  CoordSequence<XY> sequence;
  sequence.InitFrom(coords.view());

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
  sequence.InitSeparated(3, {coords.storage()[0].data(), coords.storage()[1].data()});
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
  auto x = sequence.dbegin(0);
  auto y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check vertex visiting
  coords_vec.clear();
  sequence.VisitVertices<XY>([&](XY vertex) { coords_vec.push_back(vertex); });
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check edge visiting
  std::vector<std::pair<XY, XY>> edges;
  sequence.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>{XY{0, 5}, XY{1, 6}},
                                            std::pair<XY, XY>{XY{1, 6}, XY{2, 7}}));

  // Check dbegin() iteration with offset
  coords_vec.clear();
  sequence = sequence.Slice(1, 2);
  x = sequence.dbegin(0);
  y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{1, 6}, XY{2, 7}));
}

TEST(GeoArrowHppTest, IterateCoordsInterleaved) {
  std::vector<double> coords{0, 5, 1, 6, 2, 7};
  CoordSequence<XY> sequence;
  sequence.InitInterleaved(3, coords.data());

  std::vector<XY> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check dbegin() iteration
  coords_vec.clear();
  auto x = sequence.dbegin(0);
  auto y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check vertex visiting
  coords_vec.clear();
  sequence.VisitVertices<XY>([&](XY vertex) { coords_vec.push_back(vertex); });
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check edge visiting
  std::vector<std::pair<XY, XY>> edges;
  sequence.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>{XY{0, 5}, XY{1, 6}},
                                            std::pair<XY, XY>{XY{1, 6}, XY{2, 7}}));

  // Check dbegin() iteration with offset
  coords_vec.clear();
  sequence = sequence.Slice(1, 2);
  x = sequence.dbegin(0);
  y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{1, 6}, XY{2, 7}));
}

TEST(GeoArrowHppTest, IterateUnalignedCoords) {
  TestCoords coords{{0, 1, 2}, {5, 6, 7}};

  UnalignedCoordSequence<XY> sequence;
  sequence.InitSeparated(3, {coords.storage()[0].data(), coords.storage()[1].data()});

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
  auto x = sequence.dbegin(0);
  auto y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check vertex visiting
  coords_vec.clear();
  sequence.VisitVertices<XY>([&](XY vertex) { coords_vec.push_back(vertex); });
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check edge visiting
  std::vector<std::pair<XY, XY>> edges;
  sequence.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>{XY{0, 5}, XY{1, 6}},
                                            std::pair<XY, XY>{XY{1, 6}, XY{2, 7}}));

  // Check dbegin() iteration with offset
  coords_vec.clear();
  sequence = sequence.Slice(1, 2);
  x = sequence.dbegin(0);
  y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{1, 6}, XY{2, 7}));
}

TEST(GeoArrowHppTest, IterateUnalignedCoordsInterleaved) {
  std::vector<double> coords{0, 5, 1, 6, 2, 7};
  UnalignedCoordSequence<XY> sequence;
  sequence.InitInterleaved(3, coords.data());

  std::vector<XY> coords_vec;
  for (const auto& coord : sequence) {
    coords_vec.push_back(coord);
  }

  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check dbegin() iteration
  coords_vec.clear();
  auto x = sequence.dbegin(0);
  auto y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
  }
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check vertex visiting
  coords_vec.clear();
  sequence.VisitVertices<XY>([&](XY vertex) { coords_vec.push_back(vertex); });
  EXPECT_THAT(coords_vec, ::testing::ElementsAre(XY{0, 5}, XY{1, 6}, XY{2, 7}));

  // Check edge visiting
  std::vector<std::pair<XY, XY>> edges;
  sequence.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>{XY{0, 5}, XY{1, 6}},
                                            std::pair<XY, XY>{XY{1, 6}, XY{2, 7}}));

  // Check dbegin() iteration with offset
  coords_vec.clear();
  sequence = sequence.Slice(1, 2);
  x = sequence.dbegin(0);
  y = sequence.dbegin(1);
  for (uint32_t i = 0; i < sequence.size(); i++) {
    coords_vec.push_back(XY{*x, *y});
    ++x;
    ++y;
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
  sequences.child.InitFrom(coords.view());

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
  EXPECT_EQ(geoarrow::array_util::PointArray<XY>::dimensions, GEOARROW_DIMENSIONS_XY);
  EXPECT_EQ(geoarrow::array_util::PointArray<XYZ>::dimensions, GEOARROW_DIMENSIONS_XYZ);
  EXPECT_EQ(geoarrow::array_util::PointArray<XYM>::dimensions, GEOARROW_DIMENSIONS_XYM);
  EXPECT_EQ(geoarrow::array_util::PointArray<XYZM>::dimensions, GEOARROW_DIMENSIONS_XYZM);
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

  std::vector<BoxXY> boxes;
  for (const auto& coord : native_array.value) {
    boxes.push_back(coord);
  }

  EXPECT_THAT(boxes, ::testing::ElementsAre(BoxXY{0, 1, 2, 3}, BoxXY{4, 5, 6, 7},
                                            BoxXY{8, 9, 12, 13}));

  std::vector<double> xmins(native_array.value.dbegin(0), native_array.value.dend(0));
  EXPECT_THAT(xmins, ::testing::ElementsAre(0, 4, 8));
  std::vector<double> ymins(native_array.value.dbegin(1), native_array.value.dend(1));
  EXPECT_THAT(ymins, ::testing::ElementsAre(1, 5, 9));
  std::vector<double> xmaxs(native_array.value.dbegin(2), native_array.value.dend(2));
  EXPECT_THAT(xmaxs, ::testing::ElementsAre(2, 6, 12));
  std::vector<double> ymaxs(native_array.value.dbegin(3), native_array.value.dend(3));
  EXPECT_THAT(ymaxs, ::testing::ElementsAre(3, 7, 13));

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

    // Visitors
    points.clear();
    native_array.VisitVertices<XY>([&](XY v) { points.push_back(v); });
    EXPECT_THAT(points, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}));

    std::vector<std::pair<XY, XY>> edges;
    native_array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
    EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {0, 1}),
                                              std::pair<XY, XY>({2, 3}, {2, 3}),
                                              std::pair<XY, XY>({4, 5}, {4, 5})));

    auto sliced_coords = native_array.Slice(1, 1).Coords();
    EXPECT_THAT(sliced_coords, ::testing::ElementsAre(XY{2, 3}));

    std::vector<double> sliced_x(sliced_coords.dbegin(0), sliced_coords.dend(0));
    EXPECT_THAT(sliced_x, ::testing::ElementsAre(2));
    std::vector<double> sliced_y(sliced_coords.dbegin(1), sliced_coords.dend(1));
    EXPECT_THAT(sliced_y, ::testing::ElementsAre(3));
  }
}

TEST(GeoArrowHppTest, SetArrayNullablePoint) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_POINT);
  WKXTester tester;
  tester.ReadWKT("POINT (0 1)", writer.visitor());
  tester.ReadNulls(1, writer.visitor());
  tester.ReadWKT("POINT (2 3)", writer.visitor());
  tester.ReadWKT("POINT (4 5)", writer.visitor());

  struct ArrowArray array;
  writer.Finish(&array);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_POINT);
  reader.SetArray(&array);

  geoarrow::array_util::PointArray<XY> native_array;
  ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

  // Visitors
  std::vector<XY> points;
  native_array.VisitVertices<XY>([&](XY v) { points.push_back(v); });
  EXPECT_THAT(points, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}));

  std::vector<std::pair<XY, XY>> edges;
  native_array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {0, 1}),
                                            std::pair<XY, XY>({2, 3}, {2, 3}),
                                            std::pair<XY, XY>({4, 5}, {4, 5})));
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

    // Visitors
    std::vector<XY> coords;
    native_array.VisitVertices<XY>([&](XY v) { coords.push_back(v); });
    EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7},
                                               XY{8, 9}, XY{10, 11}, XY{12, 13}));

    std::vector<std::pair<XY, XY>> edges;
    native_array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
    EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {2, 3}),
                                              std::pair<XY, XY>({4, 5}, {6, 7}),
                                              std::pair<XY, XY>({8, 9}, {10, 11}),
                                              std::pair<XY, XY>({10, 11}, {12, 13})));

    auto sliced_coords = native_array.Slice(1, 1).Coords();
    EXPECT_THAT(sliced_coords, ::testing::ElementsAre(XY{4, 5}, XY{6, 7}));

    std::vector<double> sliced_x(sliced_coords.dbegin(0), sliced_coords.dend(0));
    EXPECT_THAT(sliced_x, ::testing::ElementsAre(4, 6));
    std::vector<double> sliced_y(sliced_coords.dbegin(1), sliced_coords.dend(1));
    EXPECT_THAT(sliced_y, ::testing::ElementsAre(5, 7));
  }
}

TEST(GeoArrowHppTest, SetArrayNullableLinestring) {
  geoarrow::ArrayWriter writer(GEOARROW_TYPE_LINESTRING);
  WKXTester tester;
  tester.ReadWKT("LINESTRING (0 1, 2 3)", writer.visitor());
  tester.ReadNulls(1, writer.visitor());
  tester.ReadWKT("LINESTRING (4 5, 6 7)", writer.visitor());
  tester.ReadWKT("LINESTRING (8 9, 10 11, 12 13)", writer.visitor());

  struct ArrowArray array;
  writer.Finish(&array);

  geoarrow::ArrayReader reader(GEOARROW_TYPE_LINESTRING);
  reader.SetArray(&array);

  geoarrow::array_util::LinestringArray<XY> native_array;
  ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

  // Visitors
  std::vector<XY> points;
  native_array.VisitVertices<XY>([&](XY v) { points.push_back(v); });
  EXPECT_THAT(points, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7},
                                             XY{8, 9}, XY{10, 11}, XY{12, 13}));

  std::vector<std::pair<XY, XY>> edges;
  native_array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
  EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {2, 3}),
                                            std::pair<XY, XY>({4, 5}, {6, 7}),
                                            std::pair<XY, XY>({8, 9}, {10, 11}),
                                            std::pair<XY, XY>({10, 11}, {12, 13})));
}

TEST(GeoArrowHppTest, SetArrayMultipoint) {
  for (const auto type :
       {GEOARROW_TYPE_MULTIPOINT, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT,
        GEOARROW_TYPE_MULTIPOINT_Z, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_Z,
        GEOARROW_TYPE_MULTIPOINT_M, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_M,
        GEOARROW_TYPE_MULTIPOINT_ZM, GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_ZM}) {
    SCOPED_TRACE(geoarrow::GeometryDataType::Make(type).ToString());
    geoarrow::ArrayWriter writer(type);
    WKXTester tester;
    tester.ReadWKT("MULTIPOINT (0 1, 2 3)", writer.visitor());
    tester.ReadWKT("MULTIPOINT (4 5, 6 7)", writer.visitor());
    tester.ReadWKT("MULTIPOINT (8 9, 10 11, 12 13)", writer.visitor());

    struct ArrowArray array;
    writer.Finish(&array);

    geoarrow::ArrayReader reader(type);
    reader.SetArray(&array);

    geoarrow::array_util::MultipointArray<XY> native_array;
    ASSERT_EQ(native_array.Init(reader.View().array_view()), GEOARROW_OK);

    std::vector<std::vector<XY>> multipoints;
    for (const auto& multipoint : native_array.value) {
      std::vector<XY> coords;
      for (const auto& coord : multipoint) {
        coords.push_back(coord);
      }
      multipoints.push_back(std::move(coords));
    }

    EXPECT_THAT(multipoints, ::testing::ElementsAre(
                                 std::vector<XY>{XY{0, 1}, XY{2, 3}},
                                 std::vector<XY>{XY{4, 5}, XY{6, 7}},
                                 std::vector<XY>{XY{8, 9}, XY{10, 11}, XY{12, 13}}));

    EXPECT_THAT(native_array.Coords(),
                ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7}, XY{8, 9},
                                       XY{10, 11}, XY{12, 13}));

    // Visitors
    std::vector<XY> coords;
    native_array.VisitVertices<XY>([&](XY v) { coords.push_back(v); });
    EXPECT_THAT(coords, ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7},
                                               XY{8, 9}, XY{10, 11}, XY{12, 13}));

    std::vector<std::pair<XY, XY>> edges;
    native_array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
    EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {0, 1}),
                                              std::pair<XY, XY>({2, 3}, {2, 3}),
                                              std::pair<XY, XY>({4, 5}, {4, 5}),
                                              std::pair<XY, XY>({6, 7}, {6, 7}),
                                              std::pair<XY, XY>({8, 9}, {8, 9}),
                                              std::pair<XY, XY>({10, 11}, {10, 11}),
                                              std::pair<XY, XY>({12, 13}, {12, 13})));

    auto sliced_coords = native_array.Slice(1, 1).Coords();
    EXPECT_THAT(sliced_coords, ::testing::ElementsAre(XY{4, 5}, XY{6, 7}));

    std::vector<double> sliced_x(sliced_coords.dbegin(0), sliced_coords.dend(0));
    EXPECT_THAT(sliced_x, ::testing::ElementsAre(4, 6));
    std::vector<double> sliced_y(sliced_coords.dbegin(1), sliced_coords.dend(1));
    EXPECT_THAT(sliced_y, ::testing::ElementsAre(5, 7));
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

    // Visitors
    std::vector<XY> coords;
    native_array.VisitVertices<XY>([&](XY v) { coords.push_back(v); });
    EXPECT_THAT(coords,
                ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7}, XY{8, 9},
                                       XY{10, 11}, XY{12, 13}, XY{15, 16}, XY{17, 18}));

    std::vector<std::pair<XY, XY>> edges;
    native_array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
    EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {2, 3}),
                                              std::pair<XY, XY>({4, 5}, {6, 7}),
                                              std::pair<XY, XY>({8, 9}, {10, 11}),
                                              std::pair<XY, XY>({10, 11}, {12, 13}),
                                              std::pair<XY, XY>({15, 16}, {17, 18})));

    auto sliced_coords = native_array.Slice(1, 1).Coords();
    EXPECT_THAT(native_array.Slice(1, 1).Coords(),
                ::testing::ElementsAre(XY{4, 5}, XY{6, 7}));
    std::vector<double> sliced_x(sliced_coords.dbegin(0), sliced_coords.dend(0));
    EXPECT_THAT(sliced_x, ::testing::ElementsAre(4, 6));
    std::vector<double> sliced_y(sliced_coords.dbegin(1), sliced_coords.dend(1));
    EXPECT_THAT(sliced_y, ::testing::ElementsAre(5, 7));
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

    // Visitors
    std::vector<XY> coords;
    native_array.VisitVertices<XY>([&](XY v) { coords.push_back(v); });
    EXPECT_THAT(coords,
                ::testing::ElementsAre(XY{0, 1}, XY{2, 3}, XY{4, 5}, XY{6, 7}, XY{8, 9},
                                       XY{10, 11}, XY{12, 13}, XY{15, 16}, XY{17, 18}));

    std::vector<std::pair<XY, XY>> edges;
    native_array.VisitEdges<XY>([&](XY v0, XY v1) { edges.push_back({v0, v1}); });
    EXPECT_THAT(edges, ::testing::ElementsAre(std::pair<XY, XY>({0, 1}, {2, 3}),
                                              std::pair<XY, XY>({4, 5}, {6, 7}),
                                              std::pair<XY, XY>({8, 9}, {10, 11}),
                                              std::pair<XY, XY>({10, 11}, {12, 13}),
                                              std::pair<XY, XY>({15, 16}, {17, 18})));

    auto sliced_coords = native_array.Slice(1, 1).Coords();
    EXPECT_THAT(sliced_coords, ::testing::ElementsAre(XY{4, 5}, XY{6, 7}));
    std::vector<double> sliced_x(sliced_coords.dbegin(0), sliced_coords.dend(0));
    EXPECT_THAT(sliced_x, ::testing::ElementsAre(4, 6));
    std::vector<double> sliced_y(sliced_coords.dbegin(1), sliced_coords.dend(1));
    EXPECT_THAT(sliced_y, ::testing::ElementsAre(5, 7));
  }
}
