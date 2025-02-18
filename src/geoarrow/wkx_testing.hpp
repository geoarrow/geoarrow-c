
#pragma once

#include <exception>
#include <sstream>
#include <vector>

#include "geoarrow/geoarrow.h"
#include "nanoarrow/nanoarrow.h"

class WKXTestException : public std::exception {
 public:
  WKXTestException(const char* step, int code, const char* msg) {
    std::stringstream ss;
    ss << step << "(" << code << "): " << msg;
    message = ss.str();
  }

  const char* what() const noexcept { return message.c_str(); }

 private:
  std::string message;
};

class WKXTester {
 public:
  WKXTester() {
    GeoArrowWKTReaderInit(&wkt_reader_);
    GeoArrowWKTWriterInit(&wkt_writer_);
    GeoArrowWKBReaderInit(&wkb_reader_);
    GeoArrowWKBWriterInit(&wkb_writer_);
    wkt_writer_.use_flat_multipoint = false;
    v_.error = &error_;
    array_.release = nullptr;
    ArrowArrayViewInitFromType(&wkt_array_view_, NANOARROW_TYPE_STRING);
    ArrowArrayViewInitFromType(&wkb_array_view_, NANOARROW_TYPE_BINARY);
  }

  ~WKXTester() {
    GeoArrowWKTReaderReset(&wkt_reader_);
    GeoArrowWKTWriterReset(&wkt_writer_);
    GeoArrowWKBReaderReset(&wkb_reader_);
    GeoArrowWKBWriterReset(&wkb_writer_);
    if (array_.release != nullptr) {
      array_.release(&array_);
    }
    ArrowArrayViewReset(&wkt_array_view_);
    ArrowArrayViewReset(&wkb_array_view_);
  }

  void SetFlatMultipoint(bool use_flat_multipoint) {
    wkt_writer_.use_flat_multipoint = use_flat_multipoint;
  }

  void SetPrecision(int precision) { wkt_writer_.precision = precision; }

  std::string LastErrorMessage() { return std::string(error_.message); }

  std::string AsWKT(const std::string& str) {
    ReadWKT(str, WKTVisitor());
    return WKTValue();
  }

  std::string AsWKT(const std::basic_string<uint8_t>& str) {
    ReadWKB(str, WKTVisitor());
    return WKTValue();
  }

  std::basic_string<uint8_t> AsWKB(const std::string& str) {
    ReadWKT(str, WKBVisitor());
    return WKBValue();
  }

  std::basic_string<uint8_t> AsWKB(const std::basic_string<uint8_t>& str) {
    ReadWKB(str, WKBVisitor());
    return WKBValue();
  }

  void ReadWKB(const std::basic_string<uint8_t>& str, struct GeoArrowVisitor* v) {
    struct GeoArrowBufferView str_view;
    str_view.data = str.data();
    str_view.size_bytes = str.size();
    v->error = &error_;

    int result = GeoArrowWKBReaderVisit(&wkb_reader_, str_view, v);
    if (result != GEOARROW_OK) {
      throw WKXTestException("GeoArrowWKBReaderVisit", result, error_.message);
    }
  }

  void ReadWKT(const std::string& str, struct GeoArrowVisitor* v) {
    struct GeoArrowStringView str_view;
    str_view.data = str.data();
    str_view.size_bytes = str.size();
    v->error = &error_;

    int result = GeoArrowWKTReaderVisit(&wkt_reader_, str_view, v);
    if (result != GEOARROW_OK) {
      throw WKXTestException("GeoArrowWKTReaderVisit", result, error_.message);
    }
  }

  void ReadNulls(int64_t n, struct GeoArrowVisitor* v) {
    for (int64_t i = 0; i < n; i++) {
      v->feat_start(v);
      v->null_feat(v);
      v->feat_end(v);
    }
  }

  struct GeoArrowVisitor* WKTVisitor() {
    error_.message[0] = '\0';
    GeoArrowWKTWriterInitVisitor(&wkt_writer_, &v_);
    v_.error = &error_;
    return &v_;
  }

  struct GeoArrowVisitor* WKBVisitor() {
    error_.message[0] = '\0';
    GeoArrowWKBWriterInitVisitor(&wkb_writer_, &v_);
    v_.error = &error_;
    return &v_;
  }

  std::vector<std::string> WKTValues(const std::string& null_sentinel = "") {
    if (array_.release != nullptr) {
      array_.release(&array_);
    }

    int result = GeoArrowWKTWriterFinish(&wkt_writer_, &array_, &error_);
    if (result != GEOARROW_OK) {
      throw WKXTestException("GeoArrowWKTWriterFinish", result, error_.message);
    }

    result = ArrowArrayViewSetArray(&wkt_array_view_, &array_,
                                    reinterpret_cast<struct ArrowError*>(&error_));
    if (result != GEOARROW_OK) {
      throw WKXTestException("ArrowArrayViewSetArray", result, error_.message);
    }

    std::vector<std::string> out(array_.length);
    for (int64_t i = 0; i < array_.length; i++) {
      if (ArrowArrayViewIsNull(&wkt_array_view_, i)) {
        out[i] = null_sentinel;
      } else {
        struct ArrowStringView answer =
            ArrowArrayViewGetStringUnsafe(&wkt_array_view_, i);
        if (answer.size_bytes == 0) {
          out[i] = "";
        } else {
          out[i] = std::string(answer.data, answer.size_bytes);
        }
      }
    }

    return out;
  }

  std::string WKTValue(int64_t i = 0, const std::string& null_sentinel = "") {
    auto values = WKTValues(null_sentinel);
    return values[i];
  }

  std::vector<std::basic_string<uint8_t>> WKBValues(
      const std::basic_string<uint8_t>& null_sentinel = {}) {
    if (array_.release != nullptr) {
      array_.release(&array_);
    }

    int result = GeoArrowWKBWriterFinish(&wkb_writer_, &array_, &error_);
    if (result != GEOARROW_OK) {
      throw WKXTestException("GeoArrowWKBWriterFinish", result, error_.message);
    }

    result = ArrowArrayViewSetArray(&wkb_array_view_, &array_,
                                    reinterpret_cast<struct ArrowError*>(&error_));
    if (result != GEOARROW_OK) {
      throw WKXTestException("ArrowArrayViewSetArray", result, error_.message);
    }

    std::vector<std::basic_string<uint8_t>> out(array_.length);
    for (int64_t i = 0; i < array_.length; i++) {
      if (ArrowArrayViewIsNull(&wkb_array_view_, i)) {
        out[i] = null_sentinel;
      } else {
        struct ArrowBufferView answer = ArrowArrayViewGetBytesUnsafe(&wkb_array_view_, i);
        out[i] = std::basic_string<uint8_t>(answer.data.as_uint8, answer.size_bytes);
      }
    }

    return out;
  }

  std::basic_string<uint8_t> WKBValue(
      int64_t i = 0, const std::basic_string<uint8_t>& null_sentinel = {}) {
    auto values = WKBValues(null_sentinel);
    return values[i];
  }

 private:
  struct GeoArrowWKTReader wkt_reader_;
  struct GeoArrowWKTWriter wkt_writer_;
  struct GeoArrowWKBReader wkb_reader_;
  struct GeoArrowWKBWriter wkb_writer_;
  struct GeoArrowVisitor v_;
  struct ArrowArray array_;
  struct ArrowArrayView wkt_array_view_;
  struct ArrowArrayView wkb_array_view_;
  struct GeoArrowError error_;
};

class TestCoords {
 public:
  TestCoords(std::vector<double> x1, std::vector<double> x2) : storage_(2) {
    storage_[0] = std::move(x1);
    storage_[1] = std::move(x2);
    setup_view();
  }

  TestCoords(std::vector<double> x1, std::vector<double> x2, std::vector<double> x3)
      : storage_(3) {
    storage_[0] = std::move(x1);
    storage_[1] = std::move(x2);
    storage_[2] = std::move(x3);
    setup_view();
  }

  TestCoords(std::vector<double> x1, std::vector<double> x2, std::vector<double> x3,
             std::vector<double> x4)
      : storage_(4) {
    storage_[0] = std::move(x1);
    storage_[1] = std::move(x2);
    storage_[2] = std::move(x3);
    storage_[3] = std::move(x4);
    setup_view();
  }

  struct GeoArrowCoordView* view() { return &coord_view_; }

  struct GeoArrowWritableCoordView* writable_view() { return &writable_coord_view_; }

  const std::vector<std::vector<double>>& storage() { return storage_; }

 private:
  std::vector<std::vector<double>> storage_;
  struct GeoArrowCoordView coord_view_;
  struct GeoArrowWritableCoordView writable_coord_view_;

  void setup_view() {
    coord_view_.coords_stride = 1;
    coord_view_.n_coords = storage_[0].size();
    coord_view_.n_values = static_cast<int32_t>(storage_.size());
    for (size_t i = 0; i < storage_.size(); i++) {
      coord_view_.values[i] = storage_[i].data();
    }

    writable_coord_view_.coords_stride = 1;
    writable_coord_view_.size_coords = storage_[0].size();
    writable_coord_view_.capacity_coords = storage_[0].capacity();
    writable_coord_view_.n_values = static_cast<int32_t>(storage_.size());
    for (size_t i = 0; i < storage_.size(); i++) {
      writable_coord_view_.values[i] = storage_[i].data();
    }
  }
};

template <typename T>
static inline struct GeoArrowBufferView MakeBufferView(const std::vector<T>& v) {
  struct GeoArrowBufferView buffer_view;
  buffer_view.data = (const uint8_t*)v.data();
  buffer_view.size_bytes = v.size() * sizeof(T);
  return buffer_view;
}
