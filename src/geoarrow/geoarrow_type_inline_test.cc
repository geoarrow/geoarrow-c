
#include <cmath>

#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

#include "geoarrow/wkx_testing.hpp"

TEST(TypeInlineTest, TypeInlineTestGeometryTypeFromType) {
  EXPECT_EQ(GeoArrowGeometryTypeFromType(GEOARROW_TYPE_WKB),
            GEOARROW_GEOMETRY_TYPE_GEOMETRY);
  EXPECT_EQ(GeoArrowGeometryTypeFromType(GEOARROW_TYPE_POINT),
            GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(GeoArrowGeometryTypeFromType(GEOARROW_TYPE_POINT_Z),
            GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(GeoArrowGeometryTypeFromType(GEOARROW_TYPE_POINT_M),
            GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(GeoArrowGeometryTypeFromType(GEOARROW_TYPE_POINT_ZM),
            GEOARROW_GEOMETRY_TYPE_POINT);
  EXPECT_EQ(GeoArrowGeometryTypeFromType(GEOARROW_TYPE_INTERLEAVED_POINT),
            GEOARROW_GEOMETRY_TYPE_POINT);
}

TEST(TypeInlineTest, TypeInlineTestMakeType) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XY, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZ, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING_Z);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON_Z);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYM, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING_M);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON_M);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POINT_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_LINESTRING_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_POLYGON_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOINT_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTILINESTRING_ZM);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_MULTIPOLYGON_ZM);
}

TEST(TypeInlineTest, TypeInlineTestMakeTypeInvalidCoordType) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XY, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZ, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYZ,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYM, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON, GEOARROW_DIMENSIONS_XYM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);

  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XYZM,
                             GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                             GEOARROW_DIMENSIONS_XYZM, GEOARROW_COORD_TYPE_UNKNOWN),
            GEOARROW_TYPE_UNINITIALIZED);
}

TEST(TypeInlineTest, TypeInlineTestMakeTypeInvalidDimensions) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_UNKNOWN,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_LINESTRING,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_POLYGON, GEOARROW_DIMENSIONS_UNKNOWN,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON,
                             GEOARROW_DIMENSIONS_UNKNOWN, GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
}

TEST(TypeInlineTest, TypeInlineTestMakeTypeInvalidGeometryType) {
  EXPECT_EQ(GeoArrowMakeType(GEOARROW_GEOMETRY_TYPE_GEOMETRY, GEOARROW_DIMENSIONS_XY,
                             GEOARROW_COORD_TYPE_SEPARATE),
            GEOARROW_TYPE_UNINITIALIZED);
}

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
