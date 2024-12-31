
#ifndef GEOARROW_HPP_INCLUDED
#define GEOARROW_HPP_INCLUDED

#include <cstring>
#include <exception>
#include <string>

#include "geoarrow.h"

#if defined(GEOARROW_DEBUG)
#define _GEOARROW_THROW_NOT_OK_IMPL(NAME, EXPR, EXPR_STR, ERR)                      \
  do {                                                                              \
    const int NAME = (EXPR);                                                        \
    if (NAME) {                                                                     \
      throw ::geoarrow::ErrnoException(                                             \
          NAME,                                                                     \
          std::string(EXPR_STR) + std::string(" failed with errno ") +              \
              std::to_string(NAME) + std::string("\n * ") + std::string(__FILE__) + \
              std::string(":") + std::to_string(__LINE__),                          \
          ERR);                                                                     \
    }                                                                               \
  } while (0)
#else
#define _GEOARROW_THROW_NOT_OK_IMPL(NAME, EXPR, EXPR_STR, ERR)                  \
  do {                                                                          \
    const int NAME = (EXPR);                                                    \
    if (NAME) {                                                                 \
      throw ::geoarrow::ErrnoException(NAME,                                    \
                                       std::string(EXPR_STR) +                  \
                                           std::string(" failed with errno ") + \
                                           std::to_string(NAME),                \
                                       ERR);                                    \
    }                                                                           \
  } while (0)
#endif

#define GEOARROW_THROW_NOT_OK(ERR, EXPR)                                             \
  _GEOARROW_THROW_NOT_OK_IMPL(_GEOARROW_MAKE_NAME(errno_status_, __COUNTER__), EXPR, \
                              #EXPR, ERR)

namespace geoarrow {

class Exception : public std::exception {
 public:
  std::string message;

  Exception() = default;

  explicit Exception(const std::string& msg) : message(msg){};

  const char* what() const noexcept override { return message.c_str(); }
};

class ErrnoException : public Exception {
 public:
  GeoArrowErrorCode code{};

  ErrnoException(GeoArrowErrorCode code, const std::string& msg, GeoArrowError* error)
      : code(code) {
    if (error != nullptr) {
      message = msg + ": \n" + error->message;
    } else {
      message = msg;
    }
  }
};

namespace internal {
struct SchemaHolder {
  struct ArrowSchema schema {};
  ~SchemaHolder() {
    if (schema.release != nullptr) {
      schema.release(&schema);
    }
  }
};

struct ArrayHolder {
  struct ArrowArray array {};
  ~ArrayHolder() {
    if (array.release != nullptr) {
      array.release(&array);
    }
  }
};
}  // namespace internal

class GeometryDataType {
 public:
  GeometryDataType() = default;

  GeometryDataType(const GeometryDataType& other)
      : schema_view_(other.schema_view_),
        metadata_view_(other.metadata_view_),
        crs_(other.crs_) {
    metadata_view_.crs.data = crs_.data();
  }

  GeometryDataType& operator=(const GeometryDataType& other) {
    this->schema_view_ = other.schema_view_;
    this->metadata_view_ = other.metadata_view_;
    this->crs_ = other.crs_;
    this->metadata_view_.crs.data = this->crs_.data();
    return *this;
  }

  GeometryDataType(GeometryDataType&& other) { MoveFrom(&other); }

  GeometryDataType& operator=(GeometryDataType&& other) {
    MoveFrom(&other);
    return *this;
  }

  void MoveFrom(GeometryDataType* other) {
    schema_view_ = other->schema_view_;
    metadata_view_ = other->metadata_view_;
    crs_ = std::move(other->crs_);
    metadata_view_.crs.data = crs_.data();

    std::memset(&other->schema_view_, 0, sizeof(struct GeoArrowSchemaView));
    std::memset(&other->metadata_view_, 0, sizeof(struct GeoArrowMetadataView));
  }

  /// \brief Make a GeometryDataType from a geometry type, dimensions, and coordinate
  /// type.
  static GeometryDataType Make(
      enum GeoArrowGeometryType geometry_type,
      enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY,
      enum GeoArrowCoordType coord_type = GEOARROW_COORD_TYPE_SEPARATE,
      const std::string& metadata = "") {
    enum GeoArrowType type = GeoArrowMakeType(geometry_type, dimensions, coord_type);
    if (type == GEOARROW_TYPE_UNINITIALIZED) {
      throw ::geoarrow::Exception(
          std::string("Combination of geometry type/dimensions/coord type not valid: ") +
          GeoArrowGeometryTypeString(geometry_type) + "/" +
          GeoArrowDimensionsString(dimensions) + "/" +
          GeoArrowCoordTypeString(coord_type));
    }

    return Make(type, metadata);
  }

  /// \brief Make a GeometryDataType from a type identifier and optional extension
  /// metadata.
  static GeometryDataType Make(enum GeoArrowType type, const std::string& metadata = "") {
    switch (type) {
      case GEOARROW_TYPE_UNINITIALIZED:
        throw Exception(
            "Can't construct GeometryDataType from GEOARROW_TYPE_UNINITIALIZED");
      default:
        break;
    }

    GeoArrowError error{};
    GeoArrowSchemaView schema_view;
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowSchemaViewInitFromType(&schema_view, type));

    GeoArrowStringView metadata_str_view = {metadata.data(), (int64_t)metadata.size()};
    GeoArrowMetadataView metadata_view{};
    GEOARROW_THROW_NOT_OK(
        &error, GeoArrowMetadataViewInit(&metadata_view, metadata_str_view, &error));

    return GeometryDataType(schema_view, metadata_view);
  }

  /// \brief Make a GeometryDataType from an ArrowSchema extension type
  ///
  /// The caller retains ownership of schema.
  static GeometryDataType Make(const struct ArrowSchema* schema) {
    GeoArrowError error{};
    GeoArrowSchemaView schema_view{};
    GEOARROW_THROW_NOT_OK(&error, GeoArrowSchemaViewInit(&schema_view, schema, &error));

    GeoArrowMetadataView metadata_view{};
    GEOARROW_THROW_NOT_OK(
        &error,
        GeoArrowMetadataViewInit(&metadata_view, schema_view.extension_metadata, &error));

    return GeometryDataType(schema_view, metadata_view);
  }

  /// \brief Make a GeometryDataType from an ArrowSchema storage type
  ///
  /// The caller retains ownership of schema. If schema is an extension type,
  /// any extension type or metadata is ignored.
  static GeometryDataType Make(struct ArrowSchema* schema,
                               const std::string& extension_name,
                               const std::string& metadata = "") {
    struct GeoArrowError error {};
    struct GeoArrowSchemaView schema_view {};

    struct GeoArrowStringView extension_name_view = {extension_name.data(),
                                                     (int64_t)extension_name.size()};
    GEOARROW_THROW_NOT_OK(&error, GeoArrowSchemaViewInitFromStorage(
                                      &schema_view, schema, extension_name_view, &error));

    struct GeoArrowStringView metadata_str_view = {metadata.data(),
                                                   (int64_t)metadata.size()};
    struct GeoArrowMetadataView metadata_view {};
    GEOARROW_THROW_NOT_OK(
        &error, GeoArrowMetadataViewInit(&metadata_view, metadata_str_view, &error));

    return GeometryDataType(schema_view, metadata_view);
  }

  GeometryDataType WithGeometryType(enum GeoArrowGeometryType geometry_type) const {
    return Make(geometry_type, dimensions(), coord_type(), extension_metadata());
  }

  GeometryDataType WithCoordType(enum GeoArrowCoordType coord_type) const {
    return Make(geometry_type(), dimensions(), coord_type, extension_metadata());
  }

  GeometryDataType WithDimensions(enum GeoArrowDimensions dimensions) const {
    return Make(geometry_type(), dimensions, coord_type(), extension_metadata());
  }

  GeometryDataType WithEdgeType(enum GeoArrowEdgeType edge_type) const {
    GeometryDataType new_type(*this);
    new_type.metadata_view_.edge_type = edge_type;
    return new_type;
  }

  GeometryDataType WithCrs(const std::string& crs,
                           enum GeoArrowCrsType crs_type = GEOARROW_CRS_TYPE_UNKNOWN) {
    struct GeoArrowMetadataView metadata_view_copy = metadata_view_;
    metadata_view_copy.crs.data = crs.data();
    metadata_view_copy.crs.size_bytes = crs.size();
    metadata_view_copy.crs_type = crs_type;

    return GeometryDataType(schema_view_, metadata_view_copy);
  }

  GeometryDataType XY() const { return WithDimensions(GEOARROW_DIMENSIONS_XY); }

  GeometryDataType XYZ() const { return WithDimensions(GEOARROW_DIMENSIONS_XYZ); }

  GeometryDataType XYM() const { return WithDimensions(GEOARROW_DIMENSIONS_XYM); }

  GeometryDataType XYZM() const { return WithDimensions(GEOARROW_DIMENSIONS_XYZM); }

  GeometryDataType Simple() const {
    switch (geometry_type()) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
        return WithGeometryType(GEOARROW_GEOMETRY_TYPE_POINT);
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
        return WithGeometryType(GEOARROW_GEOMETRY_TYPE_LINESTRING);
      case GEOARROW_GEOMETRY_TYPE_POLYGON:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
        return WithGeometryType(GEOARROW_GEOMETRY_TYPE_POLYGON);
      default:
        throw ::geoarrow::Exception("Can't make simple type type");
    }
  }

  GeometryDataType Multi() const {
    switch (geometry_type()) {
      case GEOARROW_GEOMETRY_TYPE_POINT:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
        return WithGeometryType(GEOARROW_GEOMETRY_TYPE_MULTIPOINT);
      case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
        return WithGeometryType(GEOARROW_GEOMETRY_TYPE_MULTILINESTRING);
      case GEOARROW_GEOMETRY_TYPE_POLYGON:
      case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
        return WithGeometryType(GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON);
      default:
        throw ::geoarrow::Exception("Can't make multi type");
    }
  }

  void InitSchema(struct ArrowSchema* schema_out) const {
    GEOARROW_THROW_NOT_OK(nullptr,
                          GeoArrowSchemaInitExtension(schema_out, schema_view_.type));
    GEOARROW_THROW_NOT_OK(nullptr,
                          GeoArrowSchemaSetMetadata(schema_out, &metadata_view_));
  }

  void InitStorageSchema(struct ArrowSchema* schema_out) const {
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowSchemaInit(schema_out, schema_view_.type));
  }

  std::string extension_name() const {
    const char* name = GeoArrowExtensionNameFromType(schema_view_.type);
    if (name == NULL) {
      throw ::geoarrow::Exception(
          std::string("Extension name not available for type with id ") +
          std::to_string(schema_view_.type));
    }

    return name;
  }

  std::string extension_metadata() const {
    int64_t metadata_size = GeoArrowMetadataSerialize(&metadata_view_, nullptr, 0);
    char* out = reinterpret_cast<char*>(malloc(metadata_size));
    GeoArrowMetadataSerialize(&metadata_view_, out, metadata_size);
    std::string metadata(out, metadata_size);
    free(out);
    return metadata;
  }

  const enum GeoArrowType id() const { return schema_view_.type; }

  const enum GeoArrowGeometryType geometry_type() const {
    return schema_view_.geometry_type;
  }
  const enum GeoArrowCoordType coord_type() const { return schema_view_.coord_type; }

  const enum GeoArrowDimensions dimensions() const { return schema_view_.dimensions; }

  int num_dimensions() const {
    switch (dimensions()) {
      case GEOARROW_DIMENSIONS_XY:
        return 2;
      case GEOARROW_DIMENSIONS_XYZ:
      case GEOARROW_DIMENSIONS_XYM:
        return 3;
      case GEOARROW_DIMENSIONS_XYZM:
        return 4;
      default:
        return -1;
    }
  }

  const enum GeoArrowEdgeType edge_type() const { return metadata_view_.edge_type; }

  const enum GeoArrowCrsType crs_type() const { return metadata_view_.crs_type; }

  const std::string crs() const {
    int64_t len = GeoArrowUnescapeCrs(metadata_view_.crs, nullptr, 0);
    char* out = reinterpret_cast<char*>(malloc(len));
    GeoArrowUnescapeCrs(metadata_view_.crs, out, len);
    std::string out_str(out, len);
    free(out);
    return out_str;
  }

 private:
  struct GeoArrowSchemaView schema_view_ {};
  struct GeoArrowMetadataView metadata_view_ {};
  std::string crs_;

  GeometryDataType(struct GeoArrowSchemaView schema_view,
                   struct GeoArrowMetadataView metadata_view) {
    schema_view_.geometry_type = schema_view.geometry_type;
    schema_view_.dimensions = schema_view.dimensions;
    schema_view_.coord_type = schema_view.coord_type;
    schema_view_.type = schema_view.type;

    metadata_view_.edge_type = metadata_view.edge_type;
    crs_ = std::string(metadata_view.crs.data, metadata_view.crs.size_bytes);
    metadata_view_.crs_type = metadata_view.crs_type;
    metadata_view_.crs.data = crs_.data();
    metadata_view_.crs.size_bytes = crs_.size();
  }
};

namespace internal {

template <typename T>
static inline void FreeWrappedBuffer(uint8_t* ptr, int64_t size, void* private_data) {
  auto obj = reinterpret_cast<T*>(private_data);
  delete obj;
}

template <typename T>
static inline struct GeoArrowBufferView BufferView(const T& v) {
  if (v.size() == 0) {
    return {nullptr, 0};
  } else {
    return {reinterpret_cast<const uint8_t*>(v.data()),
            static_cast<int64_t>(v.size() * sizeof(typename T::value_type))};
  }
}

}  // namespace internal

class ArrayView {
 public:
  explicit ArrayView(const struct GeoArrowArrayView* array_view = nullptr)
      : array_view_(array_view) {}

  void Init(const struct GeoArrowArrayView* array_view) { array_view_ = array_view; }

  bool is_valid() {
    return array_view_ != nullptr &&
           array_view_->schema_view.type != GEOARROW_TYPE_UNINITIALIZED;
  }

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
    struct GeoArrowError error {};
    GEOARROW_THROW_NOT_OK(&error, GeoArrowArrayReaderSetArray(&reader_, array, &error));
    if (array_.array.release != nullptr) {
      array_.array.release(&array_.array);
    }

    std::memcpy(&array_, array, sizeof(struct ArrowArray));
    array->release = nullptr;
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

class ArrayBuilder {
 public:
  explicit ArrayBuilder(struct GeoArrowBuilder* builder = nullptr) : builder_(builder) {}
  void Init(struct GeoArrowBuilder* builder) { builder_ = builder; }

  bool is_valid() { return builder_ != nullptr && builder_->private_data != nullptr; }

  template <typename T>
  void SetBufferWrapped(int64_t i, T obj, GeoArrowBufferView value) {
    T* obj_moved = new T(std::move(obj));
    GeoArrowErrorCode result = GeoArrowBuilderSetOwnedBuffer(
        &builder_, i, value, &internal::FreeWrappedBuffer<T>, obj_moved);
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
        &builder_, i, value, &internal::FreeWrappedBuffer<T>, obj_moved);
    if (result != GEOARROW_OK) {
      delete obj_moved;
      throw ::geoarrow::ErrnoException(result, "GeoArrowBuilderSetOwnedBuffer()",
                                       nullptr);
    }
  }

  template <typename T>
  void AppendToBuffer(int64_t i, const T& obj) {
    GEOARROW_THROW_NOT_OK(
        nullptr, GeoArrowBuilderAppendBuffer(&builder_, i, internal::BufferView(obj)));
  }

  template <typename T>
  void AppendToOffsetBuffer(int64_t i, const T& obj) {
    GEOARROW_THROW_NOT_OK(
        nullptr, GeoArrowBuilderAppendBuffer(builder_, i, internal::BufferView(obj)));
  }

  void AppendCoords(const GeoArrowCoordView* coords, enum GeoArrowDimensions dimensions,
                    int64_t offset, int64_t length) {
    GEOARROW_THROW_NOT_OK(nullptr, GeoArrowBuilderCoordsAppend(
                                       builder_, coords, dimensions, offset, length));
  }

  void Finish(struct ArrowArray* out) {
    struct GeoArrowError error {};
    GEOARROW_THROW_NOT_OK(&error, GeoArrowBuilderFinish(builder_, out, &error));
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
    if (visitor_.coords == nullptr) {
      throw Exception("Can't use ArrayBuilder with GeoArrowVisitor in ArrayWriter");
    }

    if (!builder_.is_valid()) {
      struct GeoArrowBuilder* builder = nullptr;
      GEOARROW_THROW_NOT_OK(nullptr, GeoArrowArrayWriterBuilder(&writer_, &builder));
      builder_.Init(builder);
    }

    return builder_;
  }

 private:
  struct GeoArrowArrayWriter writer_ {};
  ArrayBuilder builder_;
  struct GeoArrowVisitor visitor_ {};
};

static inline GeometryDataType Wkb() { return GeometryDataType::Make(GEOARROW_TYPE_WKB); }

static inline GeometryDataType Wkt() { return GeometryDataType::Make(GEOARROW_TYPE_WKT); }

static inline GeometryDataType Box() { return GeometryDataType::Make(GEOARROW_TYPE_BOX); }

static inline GeometryDataType Point() {
  return GeometryDataType::Make(GEOARROW_TYPE_POINT);
}

static inline GeometryDataType Linestring() {
  return GeometryDataType::Make(GEOARROW_TYPE_LINESTRING);
}

static inline GeometryDataType Polygon() {
  return GeometryDataType::Make(GEOARROW_TYPE_POLYGON);
}

}  // namespace geoarrow

#endif
