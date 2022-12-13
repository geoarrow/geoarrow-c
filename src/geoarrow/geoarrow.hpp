
#ifndef GEOARROW_HPP_INCLUDED
#define GEOARROW_HPP_INCLUDED

#include <cerrno>
#include <sstream>
#include <string>

#include "geoarrow.h"

namespace geoarrow {

class VectorType {
 public:
  VectorType(): VectorType("") {}

  VectorType(const VectorType& other)
      : VectorType(other.schema_view_, other.metadata_view_) {}

  /// \brief Make a VectorType from a geometry type, dimensions, and coordinate type.
  static VectorType Make(enum GeoArrowGeometryType geometry_type,
                         enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY,
                         enum GeoArrowCoordType coord_type = GEOARROW_COORD_TYPE_SEPARATE,
                         const std::string& metadata = "") {
    return Make(GeoArrowMakeType(geometry_type, dimensions, coord_type), metadata);
  }

  /// \brief Make a VectorType from a type identifier and optional extension metadata.
  static VectorType Make(enum GeoArrowType type, const std::string& metadata = "") {
    struct GeoArrowSchemaView schema_view;
    int result = GeoArrowSchemaViewInitFromType(&schema_view, type);
    if (result != GEOARROW_OK) {
      return Invalid("Failed to initialize GeoArrowSchemaView");
    }

    struct GeoArrowStringView metadata_str_view = {metadata.data(),
                                                   (int64_t)metadata.size()};
    struct GeoArrowMetadataView metadata_view;
    struct GeoArrowError error;
    result = GeoArrowMetadataViewInit(&metadata_view, metadata_str_view, &error);
    if (result != GEOARROW_OK) {
      std::stringstream ss;
      ss << "Failed to initialize GeoArrowMetadataView: " << error.message;
      return Invalid(ss.str());
    }

    return VectorType(schema_view, metadata_view);
  }

  /// \brief Make a VectorType from an ArrowSchema extension type
  ///
  /// The caller retains ownership of schema.
  static VectorType Make(struct ArrowSchema* schema) {
    struct GeoArrowSchemaView schema_view;
    struct GeoArrowError error;
    int result = GeoArrowSchemaViewInit(&schema_view, schema, &error);
    if (result != GEOARROW_OK) {
      std::stringstream ss;
      ss << "Failed to initialize GeoArrowSchemaView: " << error.message;
      return Invalid(ss.str());
    }

    struct GeoArrowMetadataView metadata_view;
    result =
        GeoArrowMetadataViewInit(&metadata_view, schema_view.extension_metadata, &error);
    if (result != GEOARROW_OK) {
      std::stringstream ss;
      ss << "Failed to initialize GeoArrowMetadataView: " << error.message;
      return Invalid(ss.str());
    }

    return VectorType(schema_view, metadata_view);
  }

  /// \brief Make a VectorType from an ArrowSchema storage type
  ///
  /// The caller retains ownership of schema. If schema is an extension type,
  /// any extension type or metadata is ignored.
  static VectorType Make(struct ArrowSchema* schema, const std::string& extension_name,
                         const std::string& metadata = "") {
    struct GeoArrowSchemaView schema_view;
    struct GeoArrowError error;
    struct GeoArrowStringView extension_name_view = {extension_name.data(),
                                                     (int64_t)extension_name.size()};
    int result = GeoArrowSchemaViewInitFromStorage(&schema_view, schema,
                                                   extension_name_view, &error);
    if (result != GEOARROW_OK) {
      std::stringstream ss;
      ss << "Failed to initialize GeoArrowSchemaView: " << error.message;
      return Invalid(ss.str());
    }

    struct GeoArrowStringView metadata_str_view = {metadata.data(),
                                                   (int64_t)metadata.size()};
    struct GeoArrowMetadataView metadata_view;
    result = GeoArrowMetadataViewInit(&metadata_view, metadata_str_view, &error);
    if (result != GEOARROW_OK) {
      std::stringstream ss;
      ss << "Failed to initialize GeoArrowMetadataView: " << error.message;
      return Invalid(ss.str());
    }

    return VectorType(schema_view, metadata_view);
  }

  /// \brief Make an invalid VectorType for which valid() returns false.
  static VectorType Invalid(const std::string& err = "") { return VectorType(err); }

  VectorType WithGeometryType(enum GeoArrowGeometryType geometry_type) const {
    return Make(geometry_type, dimensions(), coord_type(), extension_metadata());
  }

  VectorType WithCoordType(enum GeoArrowCoordType coord_type) const {
    return Make(geometry_type(), dimensions(), coord_type, extension_metadata());
  }

  VectorType WithDimensions(enum GeoArrowDimensions dimensions) const {
    return Make(geometry_type(), dimensions, coord_type(), extension_metadata());
  }

  VectorType WithEdgeType(enum GeoArrowEdgeType edge_type) const {
    VectorType new_type(*this);
    new_type.metadata_view_.edge_type = edge_type;
    return new_type;
  }

  VectorType WithCrs(const std::string& crs,
                     enum GeoArrowCrsType crs_type = GEOARROW_CRS_TYPE_UNKNOWN) {
    struct GeoArrowMetadataView metadata_view_copy = metadata_view_;
    metadata_view_copy.crs.data = crs.data();
    metadata_view_copy.crs.n_bytes = crs.size();
    metadata_view_copy.crs_type = crs_type;

    return VectorType(schema_view_, metadata_view_copy);
  }

  VectorType XY() const {
    return WithDimensions(GEOARROW_DIMENSIONS_XY);
  }

  VectorType XYZ() const {
    return WithDimensions(GEOARROW_DIMENSIONS_XYZ);
  }

  VectorType XYM() const {
    return WithDimensions(GEOARROW_DIMENSIONS_XYM);
  }

  VectorType XYZM() const {
    return WithDimensions(GEOARROW_DIMENSIONS_XYZM);
  }

  VectorType Simple() const {
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
      return Invalid("Can't make simple type type");
    }
  }

  VectorType Multi() const {
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
      return Invalid("Can't make multi type");
    }
  }

  GeoArrowErrorCode InitSchema(struct ArrowSchema* schema_out) const {
    if (!valid()) {
      return EINVAL;
    }

    int result = GeoArrowSchemaInitExtension(schema_out, schema_view_.type);
    if (result != GEOARROW_OK) {
      return result;
    }

    return GeoArrowSchemaSetMetadata(schema_out, &metadata_view_);
  }

  bool valid() const { return schema_view_.type != GEOARROW_TYPE_UNINITIALIZED; }

  std::string error() const { return error_; }

  std::string extension_name() const {
    return GeoArrowExtensionNameFromType(schema_view_.type);
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
  struct GeoArrowSchemaView schema_view_;
  struct GeoArrowMetadataView metadata_view_;
  std::string crs_;
  std::string error_;

  VectorType(const std::string& err) : crs_(""), error_(err) {
    memset(&schema_view_, 0, sizeof(struct GeoArrowSchemaView));
    memset(&metadata_view_, 0, sizeof(struct GeoArrowMetadataView));
  }

  VectorType(struct GeoArrowSchemaView schema_view,
             struct GeoArrowMetadataView metadata_view)
      : error_("") {
    schema_view_.geometry_type = schema_view.geometry_type;
    schema_view_.dimensions = schema_view.dimensions;
    schema_view_.coord_type = schema_view.coord_type;
    schema_view_.type = schema_view.type;

    metadata_view_.edge_type = metadata_view.edge_type;
    crs_ = std::string(metadata_view.crs.data, metadata_view.crs.n_bytes);
    metadata_view_.crs_type = metadata_view.crs_type;
    metadata_view_.crs.data = crs_.data();
    metadata_view_.crs.n_bytes = crs_.size();
  }
};

static inline VectorType wkb() { return VectorType::Make(GEOARROW_TYPE_WKB); }

static inline VectorType large_wkb() { return VectorType::Make(GEOARROW_TYPE_LARGE_WKB); }

static inline VectorType point() { return VectorType::Make(GEOARROW_TYPE_POINT); }

static inline VectorType linestring() { return VectorType::Make(GEOARROW_TYPE_LINESTRING); }

static inline VectorType polygon() { return VectorType::Make(GEOARROW_TYPE_POLYGON); }

}  // namespace geoarrow

#endif
