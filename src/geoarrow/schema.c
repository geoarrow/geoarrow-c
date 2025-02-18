
#include <errno.h>
#include <string.h>

#include "nanoarrow/nanoarrow.h"

#include "geoarrow/geoarrow.h"

static GeoArrowErrorCode GeoArrowSchemaInitCoordFixedSizeList(struct ArrowSchema* schema,
                                                              const char* dims) {
  int64_t n_dims = strlen(dims);
  ArrowSchemaInit(schema);
  NANOARROW_RETURN_NOT_OK(ArrowSchemaSetTypeFixedSize(
      schema, NANOARROW_TYPE_FIXED_SIZE_LIST, (int32_t)n_dims));
  NANOARROW_RETURN_NOT_OK(ArrowSchemaSetName(schema->children[0], dims));
  NANOARROW_RETURN_NOT_OK(ArrowSchemaSetType(schema->children[0], NANOARROW_TYPE_DOUBLE));

  // Set child field non-nullable
  schema->children[0]->flags = 0;

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowSchemaInitCoordStruct(struct ArrowSchema* schema,
                                                       const char* dims) {
  int64_t n_dims = strlen(dims);
  char dim_name[] = {'\0', '\0'};

  NANOARROW_RETURN_NOT_OK(ArrowSchemaInitFromType(schema, NANOARROW_TYPE_STRUCT));
  NANOARROW_RETURN_NOT_OK(ArrowSchemaAllocateChildren(schema, n_dims));

  for (int64_t i = 0; i < n_dims; i++) {
    dim_name[0] = dims[i];
    NANOARROW_RETURN_NOT_OK(
        ArrowSchemaInitFromType(schema->children[i], NANOARROW_TYPE_DOUBLE));
    NANOARROW_RETURN_NOT_OK(ArrowSchemaSetName(schema->children[i], dim_name));
    // Set child non-nullable
    schema->children[i]->flags = 0;
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowSchemaInitRect(struct ArrowSchema* schema,
                                                const char* dims) {
  int64_t n_dims = strlen(dims);
  char dim_name_min[] = {'\0', 'm', 'i', 'n', '\0'};
  char dim_name_max[] = {'\0', 'm', 'a', 'x', '\0'};

  NANOARROW_RETURN_NOT_OK(ArrowSchemaInitFromType(schema, NANOARROW_TYPE_STRUCT));
  NANOARROW_RETURN_NOT_OK(ArrowSchemaAllocateChildren(schema, n_dims * 2));

  for (int64_t i = 0; i < n_dims; i++) {
    dim_name_min[0] = dims[i];
    NANOARROW_RETURN_NOT_OK(
        ArrowSchemaInitFromType(schema->children[i], NANOARROW_TYPE_DOUBLE));
    NANOARROW_RETURN_NOT_OK(ArrowSchemaSetName(schema->children[i], dim_name_min));

    dim_name_max[0] = dims[i];
    NANOARROW_RETURN_NOT_OK(
        ArrowSchemaInitFromType(schema->children[n_dims + i], NANOARROW_TYPE_DOUBLE));
    NANOARROW_RETURN_NOT_OK(
        ArrowSchemaSetName(schema->children[n_dims + i], dim_name_max));

    // Set children non-nullable
    schema->children[i]->flags = 0;
    schema->children[i + n_dims]->flags = 0;
  }

  return GEOARROW_OK;
}

static GeoArrowErrorCode GeoArrowSchemaInitListOf(struct ArrowSchema* schema,
                                                  enum GeoArrowCoordType coord_type,
                                                  const char* dims, int n,
                                                  const char** child_names) {
  if (n == 0) {
    switch (coord_type) {
      case GEOARROW_COORD_TYPE_SEPARATE:
        return GeoArrowSchemaInitCoordStruct(schema, dims);
      case GEOARROW_COORD_TYPE_INTERLEAVED:
        return GeoArrowSchemaInitCoordFixedSizeList(schema, dims);
      default:
        return EINVAL;
    }
  } else {
    ArrowSchemaInit(schema);
    NANOARROW_RETURN_NOT_OK(ArrowSchemaSetFormat(schema, "+l"));
    NANOARROW_RETURN_NOT_OK(ArrowSchemaAllocateChildren(schema, 1));
    NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitListOf(schema->children[0], coord_type,
                                                     dims, n - 1, child_names + 1));
    NANOARROW_RETURN_NOT_OK(ArrowSchemaSetName(schema->children[0], child_names[0]));

    // Set child field non-nullable
    schema->children[0]->flags = 0;

    return NANOARROW_OK;
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

    case GEOARROW_TYPE_WKT:
      return ArrowSchemaInitFromType(schema, NANOARROW_TYPE_STRING);
    case GEOARROW_TYPE_LARGE_WKT:
      return ArrowSchemaInitFromType(schema, NANOARROW_TYPE_LARGE_STRING);

    default:
      break;
  }

  enum GeoArrowDimensions dimensions = GeoArrowDimensionsFromType(type);
  enum GeoArrowCoordType coord_type = GeoArrowCoordTypeFromType(type);
  enum GeoArrowGeometryType geometry_type = GeoArrowGeometryTypeFromType(type);

  const char* dims;
  switch (dimensions) {
    case GEOARROW_DIMENSIONS_XY:
      dims = "xy";
      break;
    case GEOARROW_DIMENSIONS_XYZ:
      dims = "xyz";
      break;
    case GEOARROW_DIMENSIONS_XYM:
      dims = "xym";
      break;
    case GEOARROW_DIMENSIONS_XYZM:
      dims = "xyzm";
      break;
    default:
      return EINVAL;
  }

  switch (geometry_type) {
    case GEOARROW_GEOMETRY_TYPE_BOX:
      switch (coord_type) {
        case GEOARROW_COORD_TYPE_SEPARATE:
          NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitRect(schema, dims));
          break;
        default:
          return EINVAL;
      }
      break;

    case GEOARROW_GEOMETRY_TYPE_POINT:
      switch (coord_type) {
        case GEOARROW_COORD_TYPE_SEPARATE:
          NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitCoordStruct(schema, dims));
          break;
        case GEOARROW_COORD_TYPE_INTERLEAVED:
          NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitCoordFixedSizeList(schema, dims));
          break;
        default:
          return EINVAL;
      }
      break;

    case GEOARROW_GEOMETRY_TYPE_LINESTRING:
      NANOARROW_RETURN_NOT_OK(
          GeoArrowSchemaInitListOf(schema, coord_type, dims, 1, CHILD_NAMES_LINESTRING));
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOINT:
      NANOARROW_RETURN_NOT_OK(
          GeoArrowSchemaInitListOf(schema, coord_type, dims, 1, CHILD_NAMES_MULTIPOINT));
      break;
    case GEOARROW_GEOMETRY_TYPE_POLYGON:
      NANOARROW_RETURN_NOT_OK(
          GeoArrowSchemaInitListOf(schema, coord_type, dims, 2, CHILD_NAMES_POLYGON));
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTILINESTRING:
      NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitListOf(schema, coord_type, dims, 2,
                                                       CHILD_NAMES_MULTILINESTRING));
      break;
    case GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON:
      NANOARROW_RETURN_NOT_OK(GeoArrowSchemaInitListOf(schema, coord_type, dims, 3,
                                                       CHILD_NAMES_MULTIPOLYGON));
      break;

    default:
      return ENOTSUP;
  }

  return NANOARROW_OK;
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

  result = ArrowMetadataBuilderAppend(
      &metadata, ArrowCharView("ARROW:extension:metadata"), ArrowCharView("{}"));
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
