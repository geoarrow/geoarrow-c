
#include <arrow/array.h>
#include <arrow/c/bridge.h>
#include <arrow/extension_type.h>
#include <arrow/type.h>

#include "geoarrow.hpp"

namespace geoarrow {

namespace arrow {

class GeometryExtensionType : public ::arrow::ExtensionType {
 public:
  GeometryExtensionType(const GeometryExtensionType& other)
      : GeometryExtensionType(other.storage_type(), other.type_) {}

  static ::arrow::Result<std::shared_ptr<GeometryExtensionType>> Make(
      enum GeoArrowGeometryType geometry_type,
      enum GeoArrowDimensions dimensions = GEOARROW_DIMENSIONS_XY,
      enum GeoArrowCoordType coord_type = GEOARROW_COORD_TYPE_SEPARATE,
      const std::string& metadata = "") {
    return Make(GeoArrowMakeType(geometry_type, dimensions, coord_type), metadata);
  }

  static ::arrow::Result<std::shared_ptr<GeometryExtensionType>> Make(
      enum GeoArrowType type, const std::string& metadata = "") {
    struct ArrowSchema schema;
    int result = GeoArrowSchemaInit(&schema, type);
    if (result != GEOARROW_OK) {
      return ::arrow::Status::Invalid("Invalid input GeoArrow type");
    }

    auto maybe_arrow_type = ::arrow::ImportType(&schema);
    ARROW_RETURN_NOT_OK(maybe_arrow_type);

    auto geoarrow_type = GeometryDataType::Make(type, metadata);
    if (!geoarrow_type.valid()) {
      return ::arrow::Status::Invalid(geoarrow_type.error());
    }

    return std::make_shared<GeometryExtensionType>(maybe_arrow_type.ValueUnsafe(),
                                                   geoarrow_type);
  }

  static ::arrow::Status RegisterAll() {
    for (const auto& ext_name : all_ext_names()) {
      auto dummy_type =
          std::make_shared<GeometryExtensionType>(::arrow::null(), ext_name);
      ARROW_RETURN_NOT_OK(::arrow::RegisterExtensionType(dummy_type));
    }

    return ::arrow::Status::OK();
  }

  static ::arrow::Status UnregisterAll() {
    for (const auto& ext_name : all_ext_names()) {
      ARROW_RETURN_NOT_OK(::arrow::UnregisterExtensionType(ext_name));
    }

    return ::arrow::Status::OK();
  }

  std::string extension_name() const override { return extension_name_; }

  bool ExtensionEquals(const ::arrow::ExtensionType& other) const override {
    return extension_name() == other.extension_name() &&
           Serialize() == other.Serialize() &&
           storage_type()->Equals(other.storage_type());
  }

  std::shared_ptr<::arrow::Array> MakeArray(
      std::shared_ptr<::arrow::ArrayData> data) const override {
    return nullptr;
  }

  ::arrow::Result<std::shared_ptr<::arrow::DataType>> Deserialize(
      std::shared_ptr<::arrow::DataType> storage_type,
      const std::string& serialized_data) const override {
    struct ArrowSchema schema;
    struct GeoArrowError error;
    ARROW_RETURN_NOT_OK(ExportType(*storage_type, &schema));

    auto geoarrow_type =
        GeometryDataType::Make(&schema, extension_name_, serialized_data);
    if (!geoarrow_type.valid()) {
      return ::arrow::Status::Invalid(geoarrow_type.error());
    }

    return std::make_shared<GeometryExtensionType>(storage_type, geoarrow_type);
  }

  std::string Serialize() const override { return type_.extension_metadata(); }

  const GeometryDataType& GeoArrowType() const { return type_; }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithGeometryType(
      enum GeoArrowGeometryType geometry_type) {
    return std::make_shared<GeometryExtensionType>(storage_type(),
                                                   type_.WithGeometryType(geometry_type));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithCoordType(
      enum GeoArrowCoordType coord_type) const {
    return std::make_shared<GeometryExtensionType>(storage_type(),
                                                   type_.WithCoordType(coord_type));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithDimensions(
      enum GeoArrowDimensions dimensions) const {
    return std::make_shared<GeometryExtensionType>(storage_type(),
                                                   type_.WithDimensions(dimensions));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithEdgeType(
      enum GeoArrowEdgeType edge_type) {
    return std::make_shared<GeometryExtensionType>(storage_type(),
                                                   type_.WithEdgeType(edge_type));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithCrs(
      const std::string& crs, enum GeoArrowCrsType crs_type = GEOARROW_CRS_TYPE_UNKNOWN) {
    return std::make_shared<GeometryExtensionType>(storage_type(),
                                                   type_.WithCrs(crs, crs_type));
  }

 private:
  GeometryDataType type_;
  std::string extension_name_;

  GeometryExtensionType(const std::shared_ptr<::arrow::DataType>& storage_type,
                        const GeometryDataType& type)
      : ::arrow::ExtensionType(storage_type),
        type_(type),
        extension_name_(type.extension_name()) {}

  GeometryExtensionType(
      const std::shared_ptr<::arrow::DataType>& storage_type = ::arrow::null(),
      std::string extension_name = "")
      : ::arrow::ExtensionType(storage_type), extension_name_(extension_name) {}

  static std::vector<std::string> all_ext_names() {
    return {"geoarrow.wkb",         "geoarrow.point",      "geoarrow.linestring",
            "geoarrow.polygon",     "geoarrow.multipoint", "geoarrow.multilinestring",
            "geoarrow.multipolygon"};
  }
};

}  // namespace arrow

}  // namespace geoarrow
