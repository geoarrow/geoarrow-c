#ifndef GEOARROW_HPP_ARRAY_READER_INCLUDED
#define GEOARROW_HPP_ARRAY_READER_INCLUDED

#include "geoarrow/hpp/exception.hpp"
#include "geoarrow/hpp/geometry_data_type.hpp"
#include "geoarrow/hpp/internal.hpp"

namespace geoarrow {

class ArrayView {
 public:
  explicit ArrayView(const struct GeoArrowArrayView* array_view = nullptr)
      : array_view_(array_view) {}

  void Init(const struct GeoArrowArrayView* array_view) { array_view_ = array_view; }

  bool is_valid() {
    return array_view_ != nullptr &&
           array_view_->schema_view.type != GEOARROW_TYPE_UNINITIALIZED;
  }

  const struct GeoArrowArrayView* array_view() { return array_view_; }

 private:
  const struct GeoArrowArrayView* array_view_{};
};

class ArrayReader {
 public:
  explicit ArrayReader(GeoArrowType type) {
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayReaderInitFromType(&reader_, type));
  }

  explicit ArrayReader(const GeometryDataType& type) : ArrayReader(type.id()) {}

  explicit ArrayReader(const ArrowSchema* schema) {
    struct GeoArrowError error {};
    GEOARROW_THROW_NOT_OK(&error,
                          GeoArrowArrayReaderInitFromSchema(&reader_, schema, &error));
  }

  ~ArrayReader() {
    if (reader_.private_data != nullptr) {
      GeoArrowArrayReaderReset(&reader_);
    }
  }

  void SetArray(struct ArrowArray* array) {
    std::memcpy(&array_.array, array, sizeof(struct ArrowArray));
    array->release = nullptr;
    SetArrayNonOwning(&array_.array);
  }

  void SetArrayNonOwning(const struct ArrowArray* array) {
    struct GeoArrowError error {};
    GEOARROW_THROW_NOT_OK(&error, GeoArrowArrayReaderSetArray(&reader_, array, &error));
  }

  GeoArrowErrorCode Visit(struct GeoArrowVisitor* visitor, int64_t offset, int64_t length,
                          struct GeoArrowError* error = nullptr) {
    visitor->error = error;
    return GeoArrowArrayReaderVisit(&reader_, offset, length, visitor);
  }

  ArrayView& View() {
    if (!view_.is_valid()) {
      const struct GeoArrowArrayView* array_view = nullptr;
      GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayReaderArrayView(&reader_, &array_view));
      view_.Init(array_view);
    }

    return view_;
  }

 private:
  struct GeoArrowArrayReader reader_ {};
  ArrayView view_;
  internal::ArrayHolder array_;
};
}  // namespace geoarrow

#endif
