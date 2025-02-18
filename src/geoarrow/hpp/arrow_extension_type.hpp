
#include <arrow/array.h>
#include <arrow/c/bridge.h>
#include <arrow/extension_type.h>
#include <arrow/type.h>

#include "geoarrow/geoarrow.hpp"

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
    try {
      auto geoarrow_type =
          GeometryDataType::Make(geometry_type, dimensions, coord_type, metadata);
      return Make(geoarrow_type, metadata);
    } catch (Exception& e) {
      return ::arrow::Status::Invalid(e.what());
    }
  }

  static ::arrow::Result<std::shared_ptr<GeometryExtensionType>> Make(
      enum GeoArrowType type, const std::string& metadata = "") {
    try {
      auto geoarrow_type = GeometryDataType::Make(type);
      return Make(std::move(geoarrow_type), metadata);
    } catch (Exception& e) {
      return ::arrow::Status::Invalid(e.what());
    }
  }

  static ::arrow::Result<std::shared_ptr<GeometryExtensionType>> Make(
      GeometryDataType type, const std::string& metadata = "") {
    try {
      type = GeometryDataType::Make(type.id(), metadata);
      internal::SchemaHolder schema{};
      type.InitStorageSchema(&schema.schema);
      ARROW_ASSIGN_OR_RAISE(auto storage_type, ::arrow::ImportType(&schema.schema));
      return FromStorageAndType(storage_type, std::move(type));
    } catch (Exception& e) {
      return ::arrow::Status::Invalid(e.what());
    }
  }

  static ::arrow::Status RegisterAll() {
    for (const auto& ext_name : all_ext_names()) {
      auto dummy_type = DummyType(ext_name);
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

  std::string ToString(bool show_metadata = false) const override {
    GEOARROW_UNUSED(show_metadata);
    return std::string("GeometryExtensionType(") + type_.ToString() + ")";
  }

  std::shared_ptr<::arrow::Array> MakeArray(
      std::shared_ptr<::arrow::ArrayData> data) const override {
    return std::make_shared<::arrow::ExtensionArray>(data);
  }

  ::arrow::Result<std::shared_ptr<::arrow::DataType>> Deserialize(
      std::shared_ptr<::arrow::DataType> storage_type,
      const std::string& serialized_data) const override {
    internal::SchemaHolder schema;
    ARROW_RETURN_NOT_OK(ExportType(*storage_type, &schema.schema));

    try {
      auto geoarrow_type =
          GeometryDataType::Make(&schema.schema, extension_name_, serialized_data);
      return FromStorageAndType(storage_type, geoarrow_type);
    } catch (::geoarrow::Exception& e) {
      return ::arrow::Status::Invalid(e.what());
    }
  }

  std::string Serialize() const override { return type_.extension_metadata(); }

  const GeometryDataType& GeoArrowType() const { return type_; }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithGeometryType(
      enum GeoArrowGeometryType geometry_type) {
    return FromStorageAndType(storage_type(), type_.WithGeometryType(geometry_type));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithCoordType(
      enum GeoArrowCoordType coord_type) const {
    return FromStorageAndType(storage_type(), type_.WithCoordType(coord_type));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithDimensions(
      enum GeoArrowDimensions dimensions) const {
    return FromStorageAndType(storage_type(), type_.WithDimensions(dimensions));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithEdgeType(
      enum GeoArrowEdgeType edge_type) {
    return FromStorageAndType(storage_type(), type_.WithEdgeType(edge_type));
  }

  ::arrow::Result<std::shared_ptr<GeometryExtensionType>> WithCrs(
      const std::string& crs, enum GeoArrowCrsType crs_type = GEOARROW_CRS_TYPE_UNKNOWN) {
    return FromStorageAndType(storage_type(), type_.WithCrs(crs, crs_type));
  }

 private:
  GeometryDataType type_;
  std::string extension_name_;

  static std::shared_ptr<GeometryExtensionType> FromStorageAndType(
      const std::shared_ptr<::arrow::DataType>& storage_type,
      const GeometryDataType& type) {
    return std::shared_ptr<GeometryExtensionType>(
        new GeometryExtensionType(storage_type, type));
  }

  static std::shared_ptr<GeometryExtensionType> DummyType(std::string extension_name) {
    return std::shared_ptr<GeometryExtensionType>(
        new GeometryExtensionType(::arrow::null(), extension_name));
  }

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
