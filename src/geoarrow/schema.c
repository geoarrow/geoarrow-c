
#include <errno.h>
#include <string.h>

#include "nanoarrow.h"

#include "geoarrow.h"

static GeoArrowErrorCode GeoArrowSchemaInitCoordStruct(struct ArrowSchema* schema,
                                                       const char* dims) {
  int n_dims = strlen(dims);
  char dim_name[] = {'\0', '\0'};
  NANOARROW_RETURN_NOT_OK(ArrowSchemaInitFromType(schema, NANOARROW_TYPE_STRUCT));
  NANOARROW_RETURN_NOT_OK(ArrowSchemaAllocateChildren(schema, n_dims));
  for (int i = 0; i < n_dims; i++) {
    dim_name[0] = dims[i];
    NANOARROW_RETURN_NOT_OK(
        ArrowSchemaInitFromType(schema->children[i], NANOARROW_TYPE_DOUBLE));
    NANOARROW_RETURN_NOT_OK(ArrowSchemaSetName(schema->children[i], dim_name));
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowSchemaInitListStruct(struct ArrowSchema* schema,
                                                      const char* dims, int n,
                                                      const char** child_names) {
  if (n == 0) {
    return GeoArrowSchemaInitCoordStruct(schema, dims);
  } else {
    ArrowSchemaInit(schema);
    NANOARROW_RETURN_NOT_OK(ArrowSchemaSetFormat(schema, "+l"));
    NANOARROW_RETURN_NOT_OK(ArrowSchemaAllocateChildren(schema, 1));
    NANOARROW_RETURN_NOT_OK(
        GeoArrowSchemaInitListStruct(schema->children[0], dims, n - 1, child_names + 1));
    return ArrowSchemaSetName(schema->children[0], child_names[0]);
  }
}

#define CHILD_NAMES_LINESTRING \
  (const char*[]) { "vertices" }
#define CHILD_NAMES_POLYGON \
  (const char*[]) { "rings", "vertices" }
#define CHILD_NAMES_MULTIPOINT \
  (const char*[]) { "points" }
#define CHILD_NAMES_MULTILINESTRING \
  (const char*[]) { "linestrings", "vertices" }
#define CHILD_NAMES_MULTIPOLYGON \
  (const char*[]) { "polygons", "rings", "vertices" }

GeoArrowErrorCode GeoArrowSchemaInit(struct ArrowSchema* schema, enum GeoArrowType type) {
  schema->release = NULL;

  switch (type) {
    case GEOARROW_TYPE_WKB:
      return ArrowSchemaInitFromType(schema, NANOARROW_TYPE_BINARY);
    case GEOARROW_TYPE_LARGE_WKB:
      return ArrowSchemaInitFromType(schema, NANOARROW_TYPE_LARGE_BINARY);

    case GEOARROW_TYPE_POINT:
      return GeoArrowSchemaInitCoordStruct(schema, "xy");
    case GEOARROW_TYPE_LINESTRING:
      return GeoArrowSchemaInitListStruct(schema, "xy", 1, CHILD_NAMES_LINESTRING);
    case GEOARROW_TYPE_MULTIPOINT:
      return GeoArrowSchemaInitListStruct(schema, "xy", 1, CHILD_NAMES_MULTIPOINT);
    case GEOARROW_TYPE_POLYGON:
      return GeoArrowSchemaInitListStruct(schema, "xy", 2, CHILD_NAMES_POLYGON);
    case GEOARROW_TYPE_MULTILINESTRING:
      return GeoArrowSchemaInitListStruct(schema, "xy", 2, CHILD_NAMES_MULTILINESTRING);
    case GEOARROW_TYPE_MULTIPOLYGON:
      return GeoArrowSchemaInitListStruct(schema, "xy", 3, CHILD_NAMES_MULTIPOLYGON);

    case GEOARROW_TYPE_POINT_Z:
      return GeoArrowSchemaInitCoordStruct(schema, "xyz");
    case GEOARROW_TYPE_LINESTRING_Z:
      return GeoArrowSchemaInitListStruct(schema, "xyz", 1, CHILD_NAMES_LINESTRING);
    case GEOARROW_TYPE_MULTIPOINT_Z:
      return GeoArrowSchemaInitListStruct(schema, "xyz", 1, CHILD_NAMES_MULTIPOINT);
    case GEOARROW_TYPE_POLYGON_Z:
      return GeoArrowSchemaInitListStruct(schema, "xyz", 2, CHILD_NAMES_POLYGON);
    case GEOARROW_TYPE_MULTILINESTRING_Z:
      return GeoArrowSchemaInitListStruct(schema, "xyz", 2, CHILD_NAMES_MULTILINESTRING);
    case GEOARROW_TYPE_MULTIPOLYGON_Z:
      return GeoArrowSchemaInitListStruct(schema, "xyz", 3, CHILD_NAMES_MULTIPOLYGON);

    case GEOARROW_TYPE_POINT_M:
      return GeoArrowSchemaInitCoordStruct(schema, "xym");
    case GEOARROW_TYPE_LINESTRING_M:
      return GeoArrowSchemaInitListStruct(schema, "xym", 1, CHILD_NAMES_LINESTRING);
    case GEOARROW_TYPE_MULTIPOINT_M:
      return GeoArrowSchemaInitListStruct(schema, "xym", 1, CHILD_NAMES_MULTIPOINT);
    case GEOARROW_TYPE_POLYGON_M:
      return GeoArrowSchemaInitListStruct(schema, "xym", 2, CHILD_NAMES_POLYGON);
    case GEOARROW_TYPE_MULTILINESTRING_M:
      return GeoArrowSchemaInitListStruct(schema, "xym", 2, CHILD_NAMES_MULTILINESTRING);
    case GEOARROW_TYPE_MULTIPOLYGON_M:
      return GeoArrowSchemaInitListStruct(schema, "xym", 3, CHILD_NAMES_MULTIPOLYGON);

    case GEOARROW_TYPE_POINT_ZM:
      return GeoArrowSchemaInitCoordStruct(schema, "xyzm");
    case GEOARROW_TYPE_LINESTRING_ZM:
      return GeoArrowSchemaInitListStruct(schema, "xyzm", 1, CHILD_NAMES_LINESTRING);
    case GEOARROW_TYPE_MULTIPOINT_ZM:
      return GeoArrowSchemaInitListStruct(schema, "xyzm", 1, CHILD_NAMES_MULTIPOINT);
    case GEOARROW_TYPE_POLYGON_ZM:
      return GeoArrowSchemaInitListStruct(schema, "xyzm", 2, CHILD_NAMES_POLYGON);
    case GEOARROW_TYPE_MULTILINESTRING_ZM:
      return GeoArrowSchemaInitListStruct(schema, "xyzm", 2, CHILD_NAMES_MULTILINESTRING);
    case GEOARROW_TYPE_MULTIPOLYGON_ZM:
      return GeoArrowSchemaInitListStruct(schema, "xyzm", 3, CHILD_NAMES_MULTIPOLYGON);

    default:
      break;
  }

  return ENOTSUP;
}

GeoArrowErrorCode GeoArrowSchemaInitExtension(struct ArrowSchema* schema,
                                              enum GeoArrowType type) {
  const char* ext_type = GeoArrowExtensionNameFromType(type);
  if (ext_type == NULL) {
    return EINVAL;
  }

  struct ArrowBuffer metadata;
  NANOARROW_RETURN_NOT_OK(ArrowMetadataBuilderInit(&metadata, NULL));
  int result = ArrowMetadataBuilderAppend(
      &metadata, ArrowCharView("ARROW:extension:name"), ArrowCharView(ext_type));
  if (result != NANOARROW_OK) {
    ArrowBufferReset(&metadata);
    return result;
  }

  result = GeoArrowSchemaInit(schema, type);
  if (result != NANOARROW_OK) {
    ArrowBufferReset(&metadata);
    return result;
  }

  result = ArrowSchemaSetMetadata(schema, (const char*)metadata.data);
  ArrowBufferReset(&metadata);
  return result;
}
