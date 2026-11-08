
#include <gtest/gtest.h>

#include "geoarrow/geoarrow.h"

TEST(BuildIdTest, VersionTest) {
  EXPECT_STREQ(GeoArrowVersion(), GEOARROW_VERSION);
  EXPECT_EQ(GeoArrowVersionInt(), GEOARROW_VERSION_INT);
}

TEST(GeoArrowFromCharsTest, EndPointer) {
  const char text[] = {'1', '2', '.', '5', 'x'};
  double value = 0;
  const char* end = nullptr;
  ASSERT_EQ(GeoArrowFromChars(text, text + 4, &value, &end), GEOARROW_OK);
  EXPECT_EQ(value, 12.5);
  EXPECT_EQ(end, text + 4);

#if defined(GEOARROW_USE_FAST_FLOAT) && GEOARROW_USE_FAST_FLOAT
  // The prefix parser stops at the first non-number byte.
  end = nullptr;
  ASSERT_EQ(GeoArrowFromChars(text, text + sizeof(text), &value, &end), GEOARROW_OK);
  EXPECT_EQ(value, 12.5);
  EXPECT_EQ(end, text + 4);
#endif

  end = text;
  EXPECT_EQ(GeoArrowFromChars(text, text, &value, &end), EINVAL);
  EXPECT_EQ(end, text);
  EXPECT_EQ(GeoArrowFromChars(text + 4, text + 5, &value, &end), EINVAL);
  EXPECT_EQ(end, text);
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
