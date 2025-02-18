
#ifndef GEOARROW_HPP_GEOMETRY_DATA_TYPE_INCLUDED
#define GEOARROW_HPP_GEOMETRY_DATA_TYPE_INCLUDED

#include <cstring>
#include <string>
#include <vector>

#include "geoarrow/geoarrow.h"

#include "geoarrow/hpp/exception.hpp"

namespace geoarrow {

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

  GeometryDataType WithCrsLonLat() {
    struct GeoArrowMetadataView metadata_view_copy = metadata_view_;
    GeoArrowMetadataSetLonLat(&metadata_view_copy);
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
        throw ::geoarrow::Exception(
            std::string("Can't make simple type from geometry type ") +
            GeoArrowGeometryTypeString(geometry_type()));
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
        throw ::geoarrow::Exception(
            std::string("Can't make multi type from geometry type ") +
            GeoArrowGeometryTypeString(geometry_type()));
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

  enum GeoArrowType id() const { return schema_view_.type; }

  enum GeoArrowGeometryType geometry_type() const { return schema_view_.geometry_type; }
  enum GeoArrowCoordType coord_type() const { return schema_view_.coord_type; }

  enum GeoArrowDimensions dimensions() const { return schema_view_.dimensions; }

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

  enum GeoArrowEdgeType edge_type() const { return metadata_view_.edge_type; }

  enum GeoArrowCrsType crs_type() const { return metadata_view_.crs_type; }

  std::string crs() const {
    int64_t len = GeoArrowUnescapeCrs(metadata_view_.crs, nullptr, 0);
    char* out = reinterpret_cast<char*>(malloc(len));
    GeoArrowUnescapeCrs(metadata_view_.crs, out, len);
    std::string out_str(out, len);
    free(out);
    return out_str;
  }

  std::string ToString(size_t max_crs_size = 40) const {
    if (id() == GEOARROW_TYPE_UNINITIALIZED) {
      return "<Uninitialized GeometryDataType>";
    }

    std::string ext_name = extension_name();

    std::string dims;
    switch (dimensions()) {
      case GEOARROW_DIMENSIONS_UNKNOWN:
      case GEOARROW_DIMENSIONS_XY:
        break;
      default:
        dims = std::string("_") +
               std::string(GeoArrowDimensionsString(dimensions())).substr(2);
        break;
    }

    std::vector<std::string> modifiers;
    if (id() == GEOARROW_TYPE_LARGE_WKT || id() == GEOARROW_TYPE_LARGE_WKB) {
      modifiers.push_back("large");
    }

    if (edge_type() != GEOARROW_EDGE_TYPE_PLANAR) {
      modifiers.push_back(GeoArrowEdgeTypeString(edge_type()));
    }

    if (coord_type() == GEOARROW_COORD_TYPE_INTERLEAVED) {
      modifiers.push_back("interleaved");
    }

    std::string type_prefix;
    for (const auto& modifier : modifiers) {
      type_prefix += modifier + " ";
    }

    std::string crs_suffix;
    switch (crs_type()) {
      case GEOARROW_CRS_TYPE_NONE:
        break;
      case GEOARROW_CRS_TYPE_UNKNOWN:
        crs_suffix = crs();
        break;
      default:
        crs_suffix = std::string(GeoArrowCrsTypeString(crs_type())) + ":" + crs();
        break;
    }

    if (!crs_suffix.empty()) {
      crs_suffix = std::string("<") + crs_suffix + ">";
    }

    if (crs_suffix.size() >= max_crs_size) {
      crs_suffix = crs_suffix.substr(0, max_crs_size - 4) + "...>";
    }

    return type_prefix + ext_name + dims + crs_suffix;
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
