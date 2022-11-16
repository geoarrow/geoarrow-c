
#include <arrow/array.h>
#include <arrow/c/bridge.h>
#include <arrow/extension_type.h>
#include <arrow/type.h>

#include "geoarrow.h"

namespace geoarrow {

class VectorType : public arrow::ExtensionType {
 public:
  static arrow::Result<std::shared_ptr<VectorType>> Make(
      enum GeoArrowGeometryType geometry_type,
      enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY,
      enum GeoArrowCoordType coord_type = GEOARROW_COORD_TYPE_SEPARATE,
      const std::string& metadata = "") {
    return Make(GeoArrowMakeType(geometry_type, dimensions, coord_type), metadata);
  }

  static arrow::Result<std::shared_ptr<VectorType>> Make(
      enum GeoArrowType type, const std::string& metadata = "") {
    struct ArrowSchema schema;
    int result = GeoArrowSchemaInit(&schema, type);
    if (result != GEOARROW_OK) {
      return arrow::Status::Invalid("Invalid input GeoArrow type");
    }

    auto maybe_arrow_type = arrow::ImportType(&schema);
    ARROW_RETURN_NOT_OK(maybe_arrow_type);
    auto type_result = std::shared_ptr<VectorType>(new VectorType(
        GeoArrowExtensionNameFromType(type), "", maybe_arrow_type.ValueUnsafe()));

    result = GeoArrowSchemaViewInitFromType(&type_result->schema_view_, type);
    if (result != GEOARROW_OK) {
      return arrow::Status::Invalid("Failed to initialize GeoArrowSchemaView");
    }

    struct GeoArrowError error;
    result =
        GeoArrowMetadataViewInit(&type_result->metadata_view_,
                                 type_result->schema_view_.extension_metadata, &error);
    if (result != GEOARROW_OK) {
      return arrow::Status::Invalid(error.message);
    }

    return type_result;
  }

  static arrow::Status RegisterAll() {
    for (const auto& ext_name : all_ext_names()) {
      auto dummy_type = std::shared_ptr<VectorType>(new VectorType(ext_name));
      ARROW_RETURN_NOT_OK(arrow::RegisterExtensionType(dummy_type));
    }

    return arrow::Status::OK();
  }

  static arrow::Status UnregisterAll() {
    for (const auto& ext_name : all_ext_names()) {
      ARROW_RETURN_NOT_OK(arrow::UnregisterExtensionType(ext_name));
    }

    return arrow::Status::OK();
  }

  std::string extension_name() const override { return extension_name_; }

  bool ExtensionEquals(const arrow::ExtensionType& other) const override {
    return extension_name() == other.extension_name() &&
           Serialize() == other.Serialize() &&
           storage_type()->Equals(other.storage_type());
  }

  std::shared_ptr<arrow::Array> MakeArray(
      std::shared_ptr<arrow::ArrayData> data) const override {
    return nullptr;
  }

  arrow::Result<std::shared_ptr<arrow::DataType>> Deserialize(
      std::shared_ptr<arrow::DataType> storage_type,
      const std::string& serialized_data) const override {
    auto result = std::shared_ptr<VectorType>(
        new VectorType(extension_name(), serialized_data, storage_type));
    ARROW_RETURN_NOT_OK(result->PopulateTypeAndSchemaView());
    return result;
  }

  std::string Serialize() const override { return extension_metadata_; }

  std::string ToString() const override { return arrow::ExtensionType::ToString(); }

  const enum GeoArrowType GeoArrowType() const { return schema_view_.type; }
  const enum GeoArrowGeometryType GeometryType() const {
    return schema_view_.geometry_type;
  }
  const enum GeoArrowCoordType CoordType() const { return schema_view_.coord_type; }
  const enum GeoArrowDimensions Dimensions() const { return schema_view_.dimensions; }
  const enum GeoArrowEdgeType EdgeType() const { return metadata_view_.edge_type; }
  const enum GeoArrowCrsType CrsType() const { return metadata_view_.crs_type; }
  const std::string Crs() const {
    int64_t len = GeoArrowUnescapeCrs(metadata_view_.crs, nullptr, 0);
    char* out = reinterpret_cast<char*>(malloc(len));
    GeoArrowUnescapeCrs(metadata_view_.crs, out, len);
    std::string out_str(out, len);
    free(out);
    return out_str;
  }

  arrow::Result<std::shared_ptr<VectorType>> WithGeometryType(
      enum GeoArrowGeometryType geometry_type) const {
    return Make(geometry_type, Dimensions(), CoordType(), Serialize());
  }

  arrow::Result<std::shared_ptr<VectorType>> WithCoordType(
      enum GeoArrowCoordType coord_type) const {
    return Make(GeometryType(), Dimensions(), coord_type, Serialize());
  }

  arrow::Result<std::shared_ptr<VectorType>> WithDimensions(
      enum GeoArrowDimensions dimensions) const {
    return Make(GeometryType(), dimensions, CoordType(), Serialize());
  }

  arrow::Result<std::shared_ptr<VectorType>> WithEdgeType(
      enum GeoArrowEdgeType edge_type) {
    struct GeoArrowMetadataView metadata_view_copy = metadata_view_;
    metadata_view_copy.edge_type = edge_type;

    int64_t metadata_size = GeoArrowMetadataSerialize(&metadata_view_copy, nullptr, 0);
    char* out = reinterpret_cast<char*>(malloc(metadata_size));
    GeoArrowMetadataSerialize(&metadata_view_copy, out, metadata_size);
    std::string metadata(out, metadata_size);
    free(out);

    auto new_type = std::shared_ptr<VectorType>(
        new VectorType(extension_name(), metadata, storage_type()));
    new_type->metadata_view_ = metadata_view_copy;
    new_type->schema_view_ = schema_view_;
    return new_type;
  }

  arrow::Result<std::shared_ptr<VectorType>> WithCrs(
      const std::string& crs, enum GeoArrowCrsType crs_type = GEOARROW_CRS_TYPE_UNKNOWN) {
    struct GeoArrowMetadataView metadata_view_copy = metadata_view_;
    metadata_view_copy.crs.data = crs.data();
    metadata_view_copy.crs.n_bytes = crs.size();
    metadata_view_copy.crs_type = crs_type;

    int64_t metadata_size = GeoArrowMetadataSerialize(&metadata_view_copy, nullptr, 0);
    char* out = reinterpret_cast<char*>(malloc(metadata_size));
    GeoArrowMetadataSerialize(&metadata_view_copy, out, metadata_size);
    std::string metadata(out, metadata_size);
    free(out);

    auto new_type = std::shared_ptr<VectorType>(
        new VectorType(extension_name(), metadata, storage_type()));
    new_type->metadata_view_ = metadata_view_copy;
    new_type->schema_view_ = schema_view_;
    return new_type;
  }

 private:
  struct GeoArrowSchemaView schema_view_;
  struct GeoArrowMetadataView metadata_view_;
  std::string extension_name_;
  std::string extension_metadata_;

  VectorType(std::string extension_name, std::string extension_metadata = "",
             const std::shared_ptr<arrow::DataType> storage_type = arrow::null())
      : arrow::ExtensionType(storage_type),
        extension_name_(extension_name),
        extension_metadata_(extension_metadata) {}

  arrow::Status PopulateTypeAndSchemaView() {
    struct ArrowSchema schema;
    struct GeoArrowError error;
    ARROW_RETURN_NOT_OK(ExportType(*this, &schema));
    if (GeoArrowSchemaViewInit(&schema_view_, &schema, &error) != GEOARROW_OK) {
      schema.release(&schema);
      return arrow::Status::Invalid(error.message);
    }

    schema_view_.schema = nullptr;
    schema.release(&schema);

    if (GeoArrowMetadataViewInit(&metadata_view_, schema_view_.extension_metadata,
                                 &error) != GEOARROW_OK) {
      return arrow::Status::Invalid(error.message);
    }

    return arrow::Status::OK();
  }

  static std::vector<std::string> all_ext_names() {
    return {"geoarrow.wkb",         "geoarrow.point",      "geoarrow.linestring",
            "geoarrow.polygon",     "geoarrow.multipoint", "geoarrow.multilinestring",
            "geoarrow.multipolygon"};
  }
};

}  // namespace geoarrow
