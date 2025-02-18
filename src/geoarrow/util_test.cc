
#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

TEST(BuildIdTest, VersionTest) {
  EXPECT_STREQ(GeoArrowVersion(), GEOARROW_VERSION);
  EXPECT_EQ(GeoArrowVersionInt(), GEOARROW_VERSION_INT);
}

TEST(GeoArrowErrorTest, ErrorTestSet) {
  struct GeoArrowError error;
  EXPECT_EQ(GeoArrowErrorSet(&error, "there were %d foxes", 4), GEOARROW_OK);
  EXPECT_STREQ(error.message, "there were 4 foxes");
}

TEST(GeoArrowErrorTest, ErrorTestSetOverrun) {
  struct GeoArrowError error;
  char big_error[2048];
  const char* a_few_chars = "abcdefg";
  for (int i = 0; i < 2047; i++) {
    big_error[i] = a_few_chars[i % strlen(a_few_chars)];
  }
  big_error[2047] = '\0';

  EXPECT_EQ(GeoArrowErrorSet(&error, "%s", big_error), ERANGE);
  EXPECT_EQ(std::string(error.message), std::string(big_error, 1023));

  wchar_t bad_string[] = {0xFFFF, 0};
  EXPECT_EQ(GeoArrowErrorSet(&error, "%ls", bad_string), EINVAL);
}
