
#ifndef GEOARROW_GEOARROW_TYPES_H_INCLUDED
#define GEOARROW_GEOARROW_TYPES_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Extra guard for versions of Arrow without the canonical guard
#ifndef ARROW_FLAG_DICTIONARY_ORDERED

#ifndef ARROW_C_DATA_INTERFACE
#define ARROW_C_DATA_INTERFACE

#define ARROW_FLAG_DICTIONARY_ORDERED 1
#define ARROW_FLAG_NULLABLE 2
#define ARROW_FLAG_MAP_KEYS_SORTED 4

struct ArrowSchema {
  // Array type description
  const char* format;
  const char* name;
  const char* metadata;
  int64_t flags;
  int64_t n_children;
  struct ArrowSchema** children;
  struct ArrowSchema* dictionary;

  // Release callback
  void (*release)(struct ArrowSchema*);
  // Opaque producer-specific data
  void* private_data;
};

struct ArrowArray {
  // Array data description
  int64_t length;
  int64_t null_count;
  int64_t offset;
  int64_t n_buffers;
  int64_t n_children;
  const void** buffers;
  struct ArrowArray** children;
  struct ArrowArray* dictionary;

  // Release callback
  void (*release)(struct ArrowArray*);
  // Opaque producer-specific data
  void* private_data;
};

#endif  // ARROW_C_DATA_INTERFACE

#ifndef ARROW_C_STREAM_INTERFACE
#define ARROW_C_STREAM_INTERFACE

struct ArrowArrayStream {
  // Callback to get the stream type
  // (will be the same for all arrays in the stream).
  //
  // Return value: 0 if successful, an `errno`-compatible error code otherwise.
  //
  // If successful, the ArrowSchema must be released independently from the stream.
  int (*get_schema)(struct ArrowArrayStream*, struct ArrowSchema* out);

  // Callback to get the next array
  // (if no error and the array is released, the stream has ended)
  //
  // Return value: 0 if successful, an `errno`-compatible error code otherwise.
  //
  // If successful, the ArrowArray must be released independently from the stream.
  int (*get_next)(struct ArrowArrayStream*, struct ArrowArray* out);

  // Callback to get optional detailed error information.
  // This must only be called if the last stream operation failed
  // with a non-0 return code.
  //
  // Return value: pointer to a null-terminated character array describing
  // the last error, or NULL if no description is available.
  //
  // The returned pointer is only valid until the next operation on this stream
  // (including release).
  const char* (*get_last_error)(struct ArrowArrayStream*);

  // Release callback: release the stream's own resources.
  // Note that arrays returned by `get_next` must be individually released.
  void (*release)(struct ArrowArrayStream*);

  // Opaque producer-specific data
  void* private_data;
};

#endif  // ARROW_C_STREAM_INTERFACE
#endif  // ARROW_FLAG_DICTIONARY_ORDERED

/// \brief Return code for success
/// \ingroup geoarrow-utility
#define GEOARROW_OK 0

/// \brief Represents an errno-compatible error code
/// \ingroup geoarrow-utility
typedef int GeoArrowErrorCode;

struct GeoArrowError {
  char message[1024];
};

/// \brief A read-only view of a string
/// \ingroup geoarrow-utility
struct GeoArrowStringView {
  /// \brief Pointer to the beginning of the string. May be NULL if size_bytes is 0.
  /// there is no requirement that the strig is null-terminated.
  const char* data;

  /// \brief The size of the string in bytes
  int64_t size_bytes;
};

/// \brief A read-only view of a buffer
/// \ingroup geoarrow-utility
struct GeoArrowBufferView {
  /// \brief Pointer to the beginning of the string. May be NULL if size_bytes is 0.
  const uint8_t* data;

  /// \brief The size of the buffer in bytes
  int64_t size_bytes;
};

/// \brief Type identifier for types supported by this library
/// \ingroup geoarrow-schema
///
/// It is occasionally useful to represent each unique memory layout
/// with a single type identifier. These types include both the serialized
/// representations and the GeoArrow-native representations. Type identifiers
/// for GeoArrow-native representations can be decomposed into or reconstructed
/// from GeoArrowGeometryType, GeoArrowDimensions, and GeoArrowCoordType.
///
/// The values of this enum are chosen to support efficient decomposition
/// and/or reconstruction into the components that make up this value; however,
/// these values are not guaranteed to be stable.
enum GeoArrowType {
  GEOARROW_TYPE_UNINITIALIZED = 0,

  GEOARROW_TYPE_WKB = 100001,
  GEOARROW_TYPE_LARGE_WKB = 100002,

  GEOARROW_TYPE_WKT = 100003,
  GEOARROW_TYPE_LARGE_WKT = 100004,

  GEOARROW_TYPE_POINT = 1,
  GEOARROW_TYPE_LINESTRING = 2,
  GEOARROW_TYPE_POLYGON = 3,
  GEOARROW_TYPE_MULTIPOINT = 4,
  GEOARROW_TYPE_MULTILINESTRING = 5,
  GEOARROW_TYPE_MULTIPOLYGON = 6,

  GEOARROW_TYPE_POINT_Z = 1001,
  GEOARROW_TYPE_LINESTRING_Z = 1002,
  GEOARROW_TYPE_POLYGON_Z = 1003,
  GEOARROW_TYPE_MULTIPOINT_Z = 1004,
  GEOARROW_TYPE_MULTILINESTRING_Z = 1005,
  GEOARROW_TYPE_MULTIPOLYGON_Z = 1006,

  GEOARROW_TYPE_POINT_M = 2001,
  GEOARROW_TYPE_LINESTRING_M = 2002,
  GEOARROW_TYPE_POLYGON_M = 2003,
  GEOARROW_TYPE_MULTIPOINT_M = 2004,
  GEOARROW_TYPE_MULTILINESTRING_M = 2005,
  GEOARROW_TYPE_MULTIPOLYGON_M = 2006,

  GEOARROW_TYPE_POINT_ZM = 3001,
  GEOARROW_TYPE_LINESTRING_ZM = 3002,
  GEOARROW_TYPE_POLYGON_ZM = 3003,
  GEOARROW_TYPE_MULTIPOINT_ZM = 3004,
  GEOARROW_TYPE_MULTILINESTRING_ZM = 3005,
  GEOARROW_TYPE_MULTIPOLYGON_ZM = 3006,

  GEOARROW_TYPE_INTERLEAVED_POINT = 10001,
  GEOARROW_TYPE_INTERLEAVED_LINESTRING = 10002,
  GEOARROW_TYPE_INTERLEAVED_POLYGON = 10003,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOINT = 10004,
  GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING = 10005,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON = 10006,
  GEOARROW_TYPE_INTERLEAVED_POINT_Z = 11001,
  GEOARROW_TYPE_INTERLEAVED_LINESTRING_Z = 11002,
  GEOARROW_TYPE_INTERLEAVED_POLYGON_Z = 11003,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_Z = 11004,
  GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_Z = 11005,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_Z = 11006,
  GEOARROW_TYPE_INTERLEAVED_POINT_M = 12001,
  GEOARROW_TYPE_INTERLEAVED_LINESTRING_M = 12002,
  GEOARROW_TYPE_INTERLEAVED_POLYGON_M = 12003,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_M = 12004,
  GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_M = 12005,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_M = 12006,
  GEOARROW_TYPE_INTERLEAVED_POINT_ZM = 13001,
  GEOARROW_TYPE_INTERLEAVED_LINESTRING_ZM = 13002,
  GEOARROW_TYPE_INTERLEAVED_POLYGON_ZM = 13003,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOINT_ZM = 13004,
  GEOARROW_TYPE_INTERLEAVED_MULTILINESTRING_ZM = 13005,
  GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_ZM = 13006
};

/// \brief Geometry type identifiers supported by GeoArrow
/// \ingroup geoarrow-schema
///
/// The values of this enum are intentionally chosen to be equivalent to
/// well-known binary type identifiers.
enum GeoArrowGeometryType {
  GEOARROW_GEOMETRY_TYPE_GEOMETRY = 0,
  GEOARROW_GEOMETRY_TYPE_POINT = 1,
  GEOARROW_GEOMETRY_TYPE_LINESTRING = 2,
  GEOARROW_GEOMETRY_TYPE_POLYGON = 3,
  GEOARROW_GEOMETRY_TYPE_MULTIPOINT = 4,
  GEOARROW_GEOMETRY_TYPE_MULTILINESTRING = 5,
  GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON = 6,
  GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION = 7
};

/// \brief Dimension combinations supported by GeoArrow
/// \ingroup geoarrow-schema
enum GeoArrowDimensions {
  GEOARROW_DIMENSIONS_UNKNOWN = 0,
  GEOARROW_DIMENSIONS_XY = 1,
  GEOARROW_DIMENSIONS_XYZ = 2,
  GEOARROW_DIMENSIONS_XYM = 3,
  GEOARROW_DIMENSIONS_XYZM = 4
};

/// \brief Coordinate types supported by GeoArrow
/// \ingroup geoarrow-schema
enum GeoArrowCoordType {
  GEOARROW_COORD_TYPE_UNKNOWN = 0,
  GEOARROW_COORD_TYPE_SEPARATE = 1,
  GEOARROW_COORD_TYPE_INTERLEAVED = 2
};

/// \brief Edge types/interpolations supported by GeoArrow
/// \ingroup geoarrow-schema
enum GeoArrowEdgeType { GEOARROW_EDGE_TYPE_PLANAR, GEOARROW_EDGE_TYPE_SPHERICAL };

/// \brief Coordinate reference system types supported by GeoArrow
/// \ingroup geoarrow-schema
enum GeoArrowCrsType {
  GEOARROW_CRS_TYPE_NONE,
  GEOARROW_CRS_TYPE_UNKNOWN,
  GEOARROW_CRS_TYPE_PROJJSON
};

/// \brief Parsed view of an ArrowSchema representation of a GeoArrowType
///
/// This structure can be initialized from an ArrowSchema or a GeoArrowType.
/// It provides a structured view of memory held by an ArrowSchema or other
/// object but does not hold any memory of its own.
struct GeoArrowSchemaView {
  /// \brief The optional ArrowSchema used to populate these values
  struct ArrowSchema* schema;

  /// \brief The Arrow extension name for this type
  struct GeoArrowStringView extension_name;

  /// \brief The serialized extension metadata for this type
  ///
  /// May be NULL if there is no metadata (i.e., the JSON object representing
  /// this type would have no keys/values).
  struct GeoArrowStringView extension_metadata;

  /// \brief The GeoArrowType representing this memory layout
  enum GeoArrowType type;

  /// \brief The GeoArrowGeometryType representing this memory layout
  enum GeoArrowGeometryType geometry_type;

  /// \brief The GeoArrowDimensions representing this memory layout
  enum GeoArrowDimensions dimensions;

  /// \brief The GeoArrowCoordType representing this memory layout
  enum GeoArrowCoordType coord_type;
};

/// \brief Parsed view of GeoArrow extension metadata
struct GeoArrowMetadataView {
  /// \brief A view of the serialized metadata if this was used to populate the view
  struct GeoArrowStringView metadata;

  /// \brief The GeoArrowEdgeType represented by metadata
  enum GeoArrowEdgeType edge_type;

  /// \brief The GeoArrowEdgeType represented by metadata
  enum GeoArrowCrsType crs_type;

  /// \brief The CRS represented by metadata
  ///
  /// Because this value is a view of memory from within a JSON metadata string,
  /// it may contain the outer quotes and have escaped quotes inside it. Use
  /// GeoArrowUnescapeCrs() to sanitize this value if you need to pass it elsewhere.
  struct GeoArrowStringView crs;
};

/// \brief Union type representing a pointer to modifiable data
/// \ingroup geoarrow-builder
union GeoArrowWritableBufferViewData {
  void* data;
  char* as_char;
  uint8_t* as_uint8;
  int32_t* as_int32;
  double* as_double;
};

/// \brief A view of a modifiable buffer
/// \ingroup geoarrow-builder
struct GeoArrowWritableBufferView {
  /// \brief Pointer to the beginning of the data. May be NULL if capacity_bytes is 0.
  union GeoArrowWritableBufferViewData data;

  /// \brief The size of the buffer in bytes
  int64_t size_bytes;

  /// \brief The modifiable capacity of the buffer in bytes
  int64_t capacity_bytes;
};

/// \brief A generic view of coordinates from a GeoArrow array
///
/// This view is capable of representing both struct and interleaved coordinates.
/// Use GEOARROW_COORD_VIEW_VALUE() to generically access an ordinate.
struct GeoArrowCoordView {
  /// \brief Pointers to the beginning of each coordinate buffer
  ///
  /// May be NULL if n_coords is 0. For interleaved coordinates, these
  /// will point to the first n_values elements of the same buffer.
  const double* values[4];

  /// \brief The number of coordinates in this view
  int64_t n_coords;

  /// \brief The number of pointers in the values array (i.e., number of dimensions)
  int32_t n_values;

  /// \brief The number of elements to advance a given value pointer to the next ordinate
  ///
  /// For interleaved coordinates, coords_stride will equal n_values; for
  /// struct coordinates, coords_stride will be 1.
  int32_t coords_stride;
};

/// \brief A generic view of a writable vector of coordinates
///
/// This view is capable of representing both struct and interleaved coordinates.
/// Use GEOARROW_COORD_VIEW_VALUE() to generically access or set an ordinate
/// from a pointer to this view.
struct GeoArrowWritableCoordView {
  /// \brief Pointers to the beginning of each coordinate buffer
  double* values[4];

  /// \brief The number of coordinates in this view
  int64_t size_coords;

  /// \brief The modifiable number of coordinates in this view
  int64_t capacity_coords;

  /// \brief The number of pointers in the values array (i.e., number of dimensions)
  int32_t n_values;

  /// \brief The number of elements to advance a given value pointer to the next ordinate
  int32_t coords_stride;
};

/// \brief Generically get or set an ordinate from a GeoArrowWritableCoordView or
/// a GeoArrowCoordView.
#define GEOARROW_COORD_VIEW_VALUE(coords_, row_, col_) \
  (coords_)->values[(col_)][(row_) * (coords_)->coords_stride]

struct GeoArrowArrayView {
  struct GeoArrowSchemaView schema_view;
  int64_t offset;
  int64_t length;
  const uint8_t* validity_bitmap;
  int32_t n_offsets;
  const int32_t* offsets[3];
  int32_t first_offset[3];
  int32_t last_offset[3];
  struct GeoArrowCoordView coords;
};

struct GeoArrowWritableArrayView {
  struct GeoArrowSchemaView schema_view;
  int64_t length;
  int64_t n_buffers;
  int32_t n_offsets;
  struct GeoArrowWritableBufferView buffers[8];
  struct GeoArrowWritableCoordView coords;
};

struct GeoArrowBuilder {
  struct GeoArrowWritableArrayView view;
  void* private_data;
};

struct GeoArrowVisitor {
  int (*reserve_coord)(struct GeoArrowVisitor* v, int64_t n);
  int (*reserve_feat)(struct GeoArrowVisitor* v, int64_t n);

  int (*feat_start)(struct GeoArrowVisitor* v);
  int (*null_feat)(struct GeoArrowVisitor* v);
  int (*geom_start)(struct GeoArrowVisitor* v, enum GeoArrowGeometryType geometry_type,
                    enum GeoArrowDimensions dimensions);
  int (*ring_start)(struct GeoArrowVisitor* v);
  int (*coords)(struct GeoArrowVisitor* v, const struct GeoArrowCoordView* coords);
  int (*ring_end)(struct GeoArrowVisitor* v);
  int (*geom_end)(struct GeoArrowVisitor* v);
  int (*feat_end)(struct GeoArrowVisitor* v);

  struct GeoArrowError* error;

  void* private_data;
};

struct GeoArrowKernel {
  int (*start)(struct GeoArrowKernel* kernel, struct ArrowSchema* schema,
               const char* options, struct ArrowSchema* out, struct GeoArrowError* error);
  int (*push_batch)(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                    struct ArrowArray* out, struct GeoArrowError* error);
  int (*finish)(struct GeoArrowKernel* kernel, struct ArrowArray* out,
                struct GeoArrowError* error);
  void (*release)(struct GeoArrowKernel* kernel);
  void* private_data;
};

#ifdef __cplusplus
}
#endif

#endif
