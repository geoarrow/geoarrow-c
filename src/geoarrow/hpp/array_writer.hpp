#ifndef GEOARROW_HPP_ARRAY_WRITER_INCLUDED
#define GEOARROW_HPP_ARRAY_WRITER_INCLUDED

#include "hpp/exception.hpp"
#include "hpp/geometry_data_type.hpp"
#include "hpp/internal.hpp"

namespace geoarrow {

class ArrayBuilder {
 public:
  explicit ArrayBuilder(struct GeoArrowBuilder* builder = nullptr) : builder_(builder) {}
  void Init(struct GeoArrowBuilder* builder) { builder_ = builder; }

  bool is_valid() { return builder_ != nullptr && builder_->private_data != nullptr; }

  struct GeoArrowBuilder* builder() { return builder_; }

  template <typename T>
  void SetBufferWrapped(int64_t i, T obj, GeoArrowBufferView value) {
    T* obj_moved = new T(std::move(obj));
    GeoArrowErrorCode result = GeoArrowBuilderSetOwnedBuffer(
        builder_, i, value, &internal::FreeWrappedBuffer<T>, obj_moved);
    if (result != GEOARROW_OK) {
      delete obj_moved;
      throw ::geoarrow::ErrnoException(result, "GeoArrowBuilderSetOwnedBuffer()",
                                       nullptr);
    }
  }

  template <typename T>
  void SetBufferWrapped(int64_t i, T obj) {
    T* obj_moved = new T(std::move(obj));
    struct GeoArrowBufferView value {
      reinterpret_cast<const uint8_t*>(obj_moved->data()),
          static_cast<int64_t>(obj_moved->size() * sizeof(typename T::value_type))
    };

    GeoArrowErrorCode result = GeoArrowBuilderSetOwnedBuffer(
        builder_, i, value, &internal::FreeWrappedBuffer<T>, obj_moved);
    if (result != GEOARROW_OK) {
      delete obj_moved;
      throw ::geoarrow::ErrnoException(result, "GeoArrowBuilderSetOwnedBuffer()",
                                       nullptr);
    }
  }

  template <typename T>
  void AppendToBuffer(int64_t i, const T& obj) {
    GEOARROW_THROW_NOT_OK(
        nullptr, GeoArrowBuilderAppendBuffer(builder_, i, internal::BufferView(obj)));
  }

  template <typename T>
  void AppendToOffsetBuffer(int64_t i, const T& obj) {
    GEOARROW_THROW_NOT_OK(nullptr,
                          GeoArrowBuilderOffsetAppend(builder_, i, obj.data(),
                                                      static_cast<int64_t>(obj.size())));
  }

  void AppendCoords(const GeoArrowCoordView* coords, enum GeoArrowDimensions dimensions,
                    int64_t offset, int64_t length) {
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowBuilderCoordsAppend(
                                       builder_, coords, dimensions, offset, length));
  }

 protected:
  struct GeoArrowBuilder* builder_{};
};

class ArrayWriter {
 public:
  explicit ArrayWriter(GeoArrowType type) {
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayWriterInitFromType(&writer_, type));
  }
  explicit ArrayWriter(const GeometryDataType& type) : ArrayWriter(type.id()) {}
  explicit ArrayWriter(const ArrowSchema* schema) {
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayWriterInitFromSchema(&writer_, schema));
  }

  ~ArrayWriter() {
    if (writer_.private_data != nullptr) {
      GeoArrowArrayWriterReset(&writer_);
    }
  }

  void SetPrecision(int precision) {
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayWriterSetPrecision(&writer_, precision));
  }

  void SetFlatMultipoint(bool flat_multipoint) {
    GEOARROW_THROW_NOT_OK(nullptr,
                          GeoArrowArrayWriterSetPrecision(&writer_, flat_multipoint));
  }

  struct GeoArrowVisitor* visitor() {
    if (builder_.is_valid()) {
      throw Exception("Can't use GeoArrowVisitor with ArrayBuilder in ArrayWriter");
    }

    if (visitor_.coords == nullptr) {
      GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayWriterInitVisitor(&writer_, &visitor_));
    }

    return &visitor_;
  }

  ArrayBuilder& builder() {
    if (visitor_.coords != nullptr) {
      throw Exception("Can't use ArrayBuilder with GeoArrowVisitor in ArrayWriter");
    }

    if (!builder_.is_valid()) {
      struct GeoArrowBuilder* builder = nullptr;
      GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayWriterBuilder(&writer_, &builder));
      builder_.Init(builder);
    }

    return builder_;
  }

  void Finish(struct ArrowArray* out) {
    struct GeoArrowError error {};
    GEOARROW_THROW_NOT_OK(&error, GeoArrowArrayWriterFinish(&writer_, out, &error));
  }

 private:
  struct GeoArrowArrayWriter writer_ {};
  ArrayBuilder builder_;
  struct GeoArrowVisitor visitor_ {};
};

}  // namespace geoarrow

#endif