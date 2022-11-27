
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

class WKTTester {
 public:
  WKTTester() {
    GeoArrowWKTReaderInit(&reader_);
    GeoArrowWKTWriterInit(&writer_);
    GeoArrowWKTWriterInitVisitor(&writer_, &v_);
    array_.release = nullptr;
    ArrowArrayViewInit(&array_view_, NANOARROW_TYPE_STRING);
  }

  ~WKTTester() {
    GeoArrowWKTReaderReset(&reader_);
    GeoArrowWKTWriterReset(&writer_);
    if (array_.release != nullptr) {
      array_.release(&array_);
    }
    ArrowArrayViewReset(&array_view_);
  }

  std::string test(const std::string& str) {
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

    result = GeoArrowWKTWriterFinish(&writer_, &array_, &error_);
    if (result != GEOARROW_OK) {
      throw WKTTestException("GeoArrowWKTWriterFinish", result, error_.message);
    }

    result = ArrowArrayViewSetArray(&array_view_, &array_,
                                    reinterpret_cast<struct ArrowError*>(&error_));
    if (result != GEOARROW_OK) {
      throw WKTTestException("ArrowArrayViewSetArray", result, error_.message);
    }

    struct ArrowStringView answer = ArrowArrayViewGetStringUnsafe(&array_view_, 0);
    return std::string(answer.data, answer.n_bytes);
  }

 private:
  struct GeoArrowWKTReader reader_;
  struct GeoArrowWKTWriter writer_;
  struct GeoArrowVisitor v_;
  struct ArrowArray array_;
  struct ArrowArrayView array_view_;
  struct GeoArrowError error_;
};

TEST(WKTReaderTest, WKTReaderTestBasic) {
  struct GeoArrowWKTReader reader;
  GeoArrowWKTReaderInit(&reader);
  GeoArrowWKTReaderReset(&reader);
}

TEST(WKTReaderTest, WKTReaderTestPoint) {
  WKTTester tester;
  EXPECT_EQ(tester.test("POINT (0 1)"), "POINT (0 1)");
}
