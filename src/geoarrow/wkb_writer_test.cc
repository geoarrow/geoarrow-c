#include <stdexcept>

#include <gtest/gtest.h>

#include "geoarrow.h"
#include "nanoarrow.h"

class WKTTestException : public std::exception {
 public:
  WKTTestException(const char* step, int code, const char* msg) {
    std::stringstream ss;
    ss << step << "(" << code << "): " << msg;
    message = ss.str();
  }

  const char* what() const noexcept { return message.c_str(); }

 private:
  std::string message;
};

class WKBTester {
 public:
  WKBTester() {
    GeoArrowWKTReaderInit(&reader_);
    GeoArrowWKBWriterInit(&writer_);
    GeoArrowWKBWriterInitVisitor(&writer_, &v_);
    v_.error = &error_;
    array_.release = nullptr;
    ArrowArrayViewInit(&array_view_, NANOARROW_TYPE_BINARY);
  }

  ~WKBTester() {
    GeoArrowWKTReaderReset(&reader_);
    GeoArrowWKBWriterReset(&writer_);
    if (array_.release != nullptr) {
      array_.release(&array_);
    }
    ArrowArrayViewReset(&array_view_);
  }

  std::string LastErrorMessage() { return std::string(error_.message); }

  std::basic_string<uint8_t> AsWKB(const std::string& str) {
    error_.message[0] = '\0';
    if (array_.release != nullptr) {
      array_.release(&array_);
    }

    struct GeoArrowStringView str_view;
    str_view.data = str.data();
    str_view.n_bytes = str.size();

    int result = GeoArrowWKTReaderVisit(&reader_, str_view, &v_);
    if (result != GEOARROW_OK) {
      throw WKTTestException("GeoArrowWKTReaderVisit", result, error_.message);
    }

    result = GeoArrowWKBWriterFinish(&writer_, &array_, &error_);
    if (result != GEOARROW_OK) {
      throw WKTTestException("GeoArrowWKBWriterFinish", result, error_.message);
    }

    result = ArrowArrayViewSetArray(&array_view_, &array_,
                                    reinterpret_cast<struct ArrowError*>(&error_));
    if (result != GEOARROW_OK) {
      throw WKTTestException("ArrowArrayViewSetArray", result, error_.message);
    }

    struct ArrowBufferView answer = ArrowArrayViewGetBytesUnsafe(&array_view_, 0);
    return std::basic_string<uint8_t>(answer.data.as_uint8, answer.n_bytes);
  }

 private:
  struct GeoArrowWKTReader reader_;
  struct GeoArrowWKBWriter writer_;
  struct GeoArrowVisitor v_;
  struct ArrowArray array_;
  struct ArrowArrayView array_view_;
  struct GeoArrowError error_;
};

TEST(WKBWriterTest, WKBWriterTestBasic) {
  struct GeoArrowWKBWriter writer;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestOneNull) {
  struct GeoArrowWKBWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKBWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 1);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_STRING);
  ArrowArrayViewSetArray(&view, &array, nullptr);

  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 0));

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestOneValidOneNull) {
  struct GeoArrowWKBWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            GEOARROW_OK);
  // TODO: support the empty point
  EXPECT_EQ(v.geom_end(&v), ENOTSUP);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.null_feat(&v), GEOARROW_OK);
  EXPECT_EQ(v.feat_end(&v), GEOARROW_OK);

  struct ArrowArray array;
  EXPECT_EQ(GeoArrowWKBWriterFinish(&writer, &array, nullptr), GEOARROW_OK);
  EXPECT_EQ(array.length, 2);
  EXPECT_EQ(array.null_count, 1);

  struct ArrowArrayView view;
  ArrowArrayViewInit(&view, NANOARROW_TYPE_BINARY);
  ASSERT_EQ(ArrowArrayViewSetArray(&view, &array, nullptr), GEOARROW_OK);

  EXPECT_FALSE(ArrowArrayViewIsNull(&view, 0));
  EXPECT_TRUE(ArrowArrayViewIsNull(&view, 1));
  struct ArrowBufferView value = ArrowArrayViewGetBytesUnsafe(&view, 0);

  ArrowArrayViewReset(&view);
  array.release(&array);
  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestErrors) {
  struct GeoArrowWKBWriter writer;
  struct GeoArrowVisitor v;
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  // Invalid because level < 0
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  EXPECT_EQ(v.ring_end(&v), EINVAL);
  EXPECT_EQ(v.coords(&v, nullptr, 0, 2), GEOARROW_OK);

  GeoArrowWKBWriterReset(&writer);
  GeoArrowWKBWriterInit(&writer);
  GeoArrowWKBWriterInitVisitor(&writer, &v);

  // Invalid because of too much nesting
  EXPECT_EQ(v.feat_start(&v), GEOARROW_OK);
  for (int i = 0; i < 32; i++) {
    EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
              GEOARROW_OK);
  }
  EXPECT_EQ(v.geom_start(&v, GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY),
            EINVAL);

  GeoArrowWKBWriterReset(&writer);
}

TEST(WKBWriterTest, WKBWriterTestPoint) {
  WKBTester tester;

  EXPECT_EQ(tester.AsWKB("POINT (30 10)"),
            std::basic_string<uint8_t>({0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x40}));
}
