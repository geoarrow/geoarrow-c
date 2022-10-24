
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
      enum GeoArrowCoordType coord_type = GEOARROW_COORD_TYPE_SEPARATE) {
    return Make(GeoArrowMakeType(geometry_type, dimensions, coord_type));
  }

  static arrow::Result<std::shared_ptr<VectorType>> Make(enum GeoArrowType type) {
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

 private:
  struct GeoArrowSchemaView schema_view_;
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
    return arrow::Status::OK();
  }

  static std::vector<std::string> all_ext_names() {
    return {"geoarrow.wkb",         "geoarrow.point",      "geoarrow.linestring",
            "geoarrow.polygon",     "geoarrow.multipoint", "geoarrow.multilinestring",
            "geoarrow.multipolygon"};
  }
};

}  // namespace geoarrow
