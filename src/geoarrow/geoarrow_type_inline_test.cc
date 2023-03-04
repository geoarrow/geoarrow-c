
#include <cmath>

#include <gtest/gtest.h>

#include "geoarrow.h"

#include "wkx_testing.hpp"

TEST(TypeInlineTest, TypeInlineTestDimMap) {
  int map[4];

  // XY
  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XY, GEOARROW_DIMENSIONS_XY, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XY, GEOARROW_DIMENSIONS_XYZ, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XY, GEOARROW_DIMENSIONS_XYM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XY, GEOARROW_DIMENSIONS_XYZM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZ, GEOARROW_DIMENSIONS_XY, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  // XYZ
  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZ, GEOARROW_DIMENSIONS_XYZ, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], 2);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZ, GEOARROW_DIMENSIONS_XYM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZ, GEOARROW_DIMENSIONS_XYZM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], 2);
  EXPECT_EQ(map[3], -1);

  // XYM
  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYM, GEOARROW_DIMENSIONS_XY, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYM, GEOARROW_DIMENSIONS_XYZ, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYM, GEOARROW_DIMENSIONS_XYM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], 2);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYM, GEOARROW_DIMENSIONS_XYZM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], 2);

  // XYZM
  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZM, GEOARROW_DIMENSIONS_XY, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], -1);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZM, GEOARROW_DIMENSIONS_XYZ, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], 2);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZM, GEOARROW_DIMENSIONS_XYM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], 3);
  EXPECT_EQ(map[3], -1);

  GeoArrowMapDimensions(GEOARROW_DIMENSIONS_XYZM, GEOARROW_DIMENSIONS_XYZM, map);
  EXPECT_EQ(map[0], 0);
  EXPECT_EQ(map[1], 1);
  EXPECT_EQ(map[2], 2);
  EXPECT_EQ(map[3], 3);
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYtoXY) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XY, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XY, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYtoXYZ) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XY, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XYZ, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);

  EXPECT_EQ(dst.storage()[2][0], 0);
  EXPECT_EQ(dst.storage()[2][1], 0);
  EXPECT_TRUE(std::isnan(dst.storage()[2][2]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][3]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][4]));
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYtoXYM) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XY, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XYM, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);

  EXPECT_EQ(dst.storage()[2][0], 0);
  EXPECT_EQ(dst.storage()[2][1], 0);
  EXPECT_TRUE(std::isnan(dst.storage()[2][2]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][3]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][4]));
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYtoXYZM) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XY, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XYZM, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);

  EXPECT_EQ(dst.storage()[2][0], 0);
  EXPECT_EQ(dst.storage()[2][1], 0);
  EXPECT_TRUE(std::isnan(dst.storage()[2][2]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][3]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][4]));

  EXPECT_EQ(dst.storage()[3][0], 0);
  EXPECT_EQ(dst.storage()[3][1], 0);
  EXPECT_TRUE(std::isnan(dst.storage()[3][2]));
  EXPECT_TRUE(std::isnan(dst.storage()[3][3]));
  EXPECT_TRUE(std::isnan(dst.storage()[3][4]));
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYZtoXY) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XYZ, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XY, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYZtoXYZ) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XYZ, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XYZ, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);

  EXPECT_EQ(dst.storage()[2][0], 0);
  EXPECT_EQ(dst.storage()[2][1], 0);
  EXPECT_EQ(dst.storage()[2][2], 12);
  EXPECT_EQ(dst.storage()[2][3], 13);
  EXPECT_EQ(dst.storage()[2][4], 14);
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYZtoXYM) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XYZ, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XYM, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);

  EXPECT_EQ(dst.storage()[2][0], 0);
  EXPECT_EQ(dst.storage()[2][1], 0);
  EXPECT_TRUE(std::isnan(dst.storage()[2][2]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][3]));
  EXPECT_TRUE(std::isnan(dst.storage()[2][4]));
}

TEST(TypeInlineTest, TypeInlineTestCopyCoordsXYZtoXYZM) {
  TestCoords src({1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 12, 13, 14, 15},
                 {16, 17, 18, 19, 20});
  TestCoords dst({0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0});

  GeoArrowCoordViewCopy(src.view(), GEOARROW_DIMENSIONS_XYZ, 1, dst.writable_view(),
                        GEOARROW_DIMENSIONS_XYZM, 2, 3);
  EXPECT_EQ(dst.storage()[0][0], 0);
  EXPECT_EQ(dst.storage()[0][1], 0);
  EXPECT_EQ(dst.storage()[0][2], 2);
  EXPECT_EQ(dst.storage()[0][3], 3);
  EXPECT_EQ(dst.storage()[0][4], 4);

  EXPECT_EQ(dst.storage()[1][0], 0);
  EXPECT_EQ(dst.storage()[1][1], 0);
  EXPECT_EQ(dst.storage()[1][2], 7);
  EXPECT_EQ(dst.storage()[1][3], 8);
  EXPECT_EQ(dst.storage()[1][4], 9);

  EXPECT_EQ(dst.storage()[2][0], 0);
  EXPECT_EQ(dst.storage()[2][1], 0);
  EXPECT_EQ(dst.storage()[2][2], 12);
  EXPECT_EQ(dst.storage()[2][3], 13);
  EXPECT_EQ(dst.storage()[2][4], 14);

  EXPECT_EQ(dst.storage()[3][0], 0);
  EXPECT_EQ(dst.storage()[3][1], 0);
  EXPECT_TRUE(std::isnan(dst.storage()[3][2]));
  EXPECT_TRUE(std::isnan(dst.storage()[3][3]));
  EXPECT_TRUE(std::isnan(dst.storage()[3][4]));
}
