
#ifndef GEOARROW_HPP_INCLUDED
#define GEOARROW_HPP_INCLUDED

#include <string>
#include <sstream>

#include "geoarrow.h"

namespace geoarrow {

class VectorType {
  VectorType(const VectorType& other)
      : VectorType(other.schema_view_, other.metadata_view_) {}

  static VectorType Make(enum GeoArrowGeometryType geometry_type,
                         enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY,
                         enum GeoArrowCoordType coord_type = GEOARROW_COORD_TYPE_SEPARATE,
                         const std::string& metadata = "") {
    return Make(GeoArrowMakeType(geometry_type, dimensions, coord_type), metadata);
  }

  static VectorType Make(enum GeoArrowType type, const std::string& metadata = "") {
    struct ArrowSchema schema;
    int result = GeoArrowSchemaInit(&schema, type);
    if (result != GEOARROW_OK) {
      return Invalid("Invalid input GeoArrow type");
    }

    struct GeoArrowSchemaView schema_view;
    result = GeoArrowSchemaViewInitFromType(&schema_view, type);
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

  static VectorType Invalid(const std::string& err = "invalid") {
    return VectorType(err);
  }

  bool valid() const { return error_.size() > 0; }

  std::string error() const { return error_; }

  std::string extension_name() const {
    return GeoArrowExtensionNameFromType(schema_view_.type);
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

  VectorType(const std::string& err = "") : crs_(""), error_(err) {
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

}  // namespace geoarrow

#endif
