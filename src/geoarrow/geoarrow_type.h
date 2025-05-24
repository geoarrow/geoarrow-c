
#ifndef GEOARROW_GEOARROW_TYPES_H_INCLUDED
#define GEOARROW_GEOARROW_TYPES_H_INCLUDED

#include <stdint.h>

#include "geoarrow/geoarrow_config.h"

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

#define _GEOARROW_CONCAT(x, y) x##y
#define _GEOARROW_MAKE_NAME(x, y) _GEOARROW_CONCAT(x, y)

#define _GEOARROW_RETURN_NOT_OK_IMPL(NAME, EXPR) \
  do {                                           \
    const int NAME = (EXPR);                     \
    if (NAME) return NAME;                       \
  } while (0)

/// \brief Macro helper for error handling
/// \ingroup geoarrow-utility
#define GEOARROW_RETURN_NOT_OK(EXPR) \
  _GEOARROW_RETURN_NOT_OK_IMPL(_GEOARROW_MAKE_NAME(errno_status_, __COUNTER__), EXPR)

#define GEOARROW_UNUSED(expr) ((void)expr)

// This section remaps the non-prefixed symbols to the prefixed symbols so that
// code written against this build can be used independent of the value of
// GEOARROW_NAMESPACE.
#ifdef GEOARROW_NAMESPACE

#define GeoArrowVersion _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowVersion)
#define GeoArrowVersionInt _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowVersionInt)
#define GeoArrowErrorSet _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowErrorSet)
#define GeoArrowFromChars _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowFromChars)
#define GeoArrowPrintDouble _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowPrintDouble)
#define GeoArrowSchemaInit _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowSchemaInit)
#define GeoArrowSchemaInitExtension \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowSchemaInitExtension)
#define GeoArrowSchemaViewInit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowSchemaViewInit)
#define GeoArrowSchemaViewInitFromStorage \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowSchemaViewInitFromStorage)
#define GeoArrowSchemaViewInitFromType \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowSchemaViewInitFromType)
#define GeoArrowMetadataViewInit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowMetadataViewInit)
#define GeoArrowMetadataSerialize \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowMetadataSerialize)
#define GeoArrowSchemaSetMetadata \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowSchemaSetMetadata)
#define GeoArrowSchemaSetMetadataFrom \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowSchemaSetMetadataFrom)
#define GeoArrowMetadataSetLonLat \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowMetadataSetLonLat)
#define GeoArrowUnescapeCrs _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowUnescapeCrs)
#define GeoArrowArrayViewInitFromType \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayViewInitFromType)
#define GeoArrowArrayViewInitFromSchema \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayViewInitFromSchema)
#define GeoArrowArrayViewSetArray \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayViewSetArray)
#define GeoArrowBuilderInitFromType \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderInitFromType)
#define GeoArrowBuilderInitFromSchema \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderInitFromSchema)
#define GeoArrowBuilderReserveBuffer \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderReserveBuffer)
#define GeoArrowBuilderAppendBufferUnsafe \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderAppendBufferUnsafe)
#define GeoArrowBuilderAppendBuffer \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderAppendBuffer)
#define GeoArrowBuilderSetOwnedBuffer \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderSetOwnedBuffer)
#define GeoArrowBuilderFinish \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderFinish)
#define GeoArrowBuilderReset _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowBuilderReset)
#define GeoArrowKernelInit _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowKernelInit)
#define GeoArrowGeometryInit _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryInit)
#define GeoArrowGeometryReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryReset)
#define GeoArrowGeometryShallowCopy \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryShallowCopy)
#define GeoArrowGeometryDeepCopy \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryDeepCopy)
#define GeoArrowGeometryResizeNodes \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryResizeNodes)
#define GeoArrowGeometryAppendNode \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryAppendNode)
#define GeoArrowGeometryViewVisit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryViewVisit)
#define GeoArrowGeometryVisit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryVisit)
#define GeoArrowGeometryInitVisitor \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowGeometryInitVisitor)
#define GeoArrowVisitorInitVoid \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowVisitorInitVoid)
#define GeoArrowArrayViewVisitNative \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayViewVisitNative)
#define GeoArrowNativeWriterInit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowNativeWriterInit)
#define GeoArrowNativeWriterInitVisitor \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowNativeWriterInitVisitor)
#define GeoArrowNativeWriterFinish \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowNativeWriterFinish)
#define GeoArrowNativeWriterReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowNativeWriterReset)
#define GeoArrowWKTWriterInit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKTWriterInit)
#define GeoArrowWKTWriterInitVisitor \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKTWriterInitVisitor)
#define GeoArrowWKTWriterFinish \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKTWriterFinish)
#define GeoArrowWKTWriterReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKTWriterReset)
#define GeoArrowWKTReaderInit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKTReaderInit)
#define GeoArrowWKTReaderVisit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKTReaderVisit)
#define GeoArrowWKTReaderReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKTReaderReset)
#define GeoArrowWKBWriterInit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBWriterInit)
#define GeoArrowWKBWriterInitVisitor \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBWriterInitVisitor)
#define GeoArrowWKBWriterFinish \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBWriterFinish)
#define GeoArrowWKBWriterReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBWriterReset)
#define GeoArrowWKBReaderInit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBReaderInit)
#define GeoArrowWKBReaderVisit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBReaderVisit)
#define GeoArrowWKBReaderRead \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBReaderRead)
#define GeoArrowWKBReaderReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowWKBReaderReset)
#define GeoArrowArrayReaderInitFromType \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayReaderInitFromType)
#define GeoArrowArrayReaderInitFromSchema \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayReaderInitFromSchema)
#define GeoArrowArrayReaderSetArray \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayReaderSetArray)
#define GeoArrowArrayReaderVisit \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayReaderVisit)
#define GeoArrowArrayReaderArrayView \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayReaderArrayView)
#define GeoArrowArrayReaderReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayReaderReset)
#define GeoArrowArrayWriterInitFromType \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayWriterInitFromType)
#define GeoArrowArrayWriterInitFromSchema \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayWriterInitFromSchema)
#define GeoArrowArrayWriterSetPrecision \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayWriterSetPrecision)
#define GeoArrowArrayWriterSetFlatMultipoint \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayWriterSetFlatMultipoint)
#define GeoArrowArrayWriterInitVisitor \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayWriterInitVisitor)
#define GeoArrowArrayWriterFinish \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayWriterFinish)
#define GeoArrowArrayWriterReset \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowArrayWriterReset)
#define GeoArrowFromChars _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowFromChars)
#define GeoArrowd2sexp_buffered_n \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowd2sexp_buffered_n)
#define GeoArrowd2sfixed_buffered_n \
  _GEOARROW_MAKE_NAME(GEOARROW_NAMESPACE, GeoArrowd2sfixed_buffered_n)
#endif

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
  /// there is no requirement that the string is null-terminated.
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

  GEOARROW_TYPE_BOX = 990,
  GEOARROW_TYPE_BOX_Z = 1990,
  GEOARROW_TYPE_BOX_M = 2990,
  GEOARROW_TYPE_BOX_ZM = 3990,

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
  GEOARROW_TYPE_INTERLEAVED_MULTIPOLYGON_ZM = 13006,

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
  GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION = 7,
  GEOARROW_GEOMETRY_TYPE_BOX = 990
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
enum GeoArrowEdgeType {
  GEOARROW_EDGE_TYPE_PLANAR,
  GEOARROW_EDGE_TYPE_SPHERICAL,
  GEOARROW_EDGE_TYPE_VINCENTY,
  GEOARROW_EDGE_TYPE_THOMAS,
  GEOARROW_EDGE_TYPE_ANDOYER,
  GEOARROW_EDGE_TYPE_KARNEY
};

/// \brief Coordinate reference system types supported by GeoArrow
/// \ingroup geoarrow-schema
enum GeoArrowCrsType {
  GEOARROW_CRS_TYPE_NONE,
  GEOARROW_CRS_TYPE_UNKNOWN,
  GEOARROW_CRS_TYPE_PROJJSON,
  GEOARROW_CRS_TYPE_WKT2_2019,
  GEOARROW_CRS_TYPE_AUTHORITY_CODE,
  GEOARROW_CRS_TYPE_SRID
};

/// \brief Flag to indicate that coordinates must be endian-swapped before being
/// interpreted on the current platform
#define GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN 0x01

/// \brief Generic Geometry node representation
///
/// This structure represents a generic view on a geometry, inspired by DuckDB-spatial's
/// sgl::geometry. The ownership of this struct is typically managed by a
/// GeoArrowGeometryRoot or a generic sequence type (e.g., std::vector). Its design allows
/// for efficient iteration over a wide variety of underlying structures without the need
/// for recursive structures (but allowing for recursive iteration where required).
///
/// A typical geometry is represented by one or more GeoArrowGeometryNodes arranged
/// sequentially in memory depth-first such that a node is followed by its children (if
/// any), then any remaining siblings from a common parent (if any), and so on. Nodes
/// should be passed by pointer such that a function can iterate over children; however,
/// function signatures should make this expectation clear.
///
/// This structure is packed such that it is pointer-aligned and occupies 64 bytes.
struct GeoArrowGeometryNode {
  /// \brief Coordinate data
  ///
  /// Each pointer in coords points to the first coordinate in the sequence when
  /// geometry_type is GEOARROW_GEOMETRY_TYPE_POINT or GEOARROW_GEOMETRY_TYPE_LINESTRING
  /// ordered according to the dimensions specified in dimensions.
  ///
  /// The pointers need not be aligned. The data type must be a float, double, or signed
  /// 32-bit integer, communicated by a parent structure. Producers should produce double
  /// coordinates unless absolutely necessary; consumers may choose to only support double
  /// coordinates. Unless specified by a parent structure coordinates are C doubles.
  ///
  /// For dimension j, it must be safe to access the range
  /// [coords[j], coords[j] + size * stride[j] + sizeof(T)].
  ///
  /// The pointers in coords must never be NULL. Empty dimensions must point to a valid
  /// address whose value is NaN (for floating point types) or the most negative possible
  /// value (for integer types). This is true even when size is 0 (i.e., it must always be
  /// safe to access at least one value).
  const uint8_t* coords[4];

  /// \brief Number of bytes between adjacent coordinate values in coords, respectively
  ///
  /// The number of bytes to advance each pointer in coords when moving to the next
  /// coordinate. This allow representing a wide variety of coordinate layouts:
  ///
  /// - Interleaved coordinates: coord_stride is n_dimensions * sizeof(T). For
  ///   example, interleaved XY coordinates with double precision would have
  ///   coords set to {data, data + 8, &kNaN, &kNaN} and coord_stride set to
  ///   {16, 16, 0, 0}
  /// - Separated coordinates: coord_stride is sizeof(T). For example, separated
  ///   XY coordinates would have coords set to {x, y, &kNaN, &kNaN} and
  ///   coord_stride set to {8, 8, 0, 0}.
  /// - A constant value: coord_stride is 0. For example, the value 30, 10 as
  ///   a constant would have coords set to {&thirty, &ten, &kNaN, &kNaN} and
  ///   coord_stride set to {0, 0, 0, 0}.
  /// - WKB values with constant length packed end-to-end contiguously in memory
  ///   (e.g., in an Arrow array as the data buffer in an Array that does not
  ///   contain nulls): coord_stride is the size of one WKB item. For example,
  ///   an Arrow array of XY points would have coords set to {data + 1 + 4,
  ///   data + 1 + 4 + 8, &kNaN, &kNaN} and stride set to {21, 21, 0, 0}.
  /// - Any of the above but reversed (by pointing to the last coordinate
  ///   and setting the stride to a negative value).
  int32_t coord_stride[4];

  /// \brief The number of coordinates or children in this geometry
  ///
  /// When geometry_type is GEOARROW_GEOMETRY_TYPE_POINT or
  /// GEOARROW_GEOMETRY_TYPE_LINESTRING, the number of coordinates in the sequence.
  /// Otherwise, the number of child geometries.
  uint32_t size;

  /// \brief The GeoArrowGeometryType of this geometry
  ///
  /// For the purposes of this structure, rings of a polygon are considered a
  /// GEOARROW_GEOMETRY_TYPE_LINESTRING. The value GEOARROW_GEOMETRY_TYPE_UNINITIALIZED
  /// can be used to communicate an invalid or null value but must set size to zero.
  uint8_t geometry_type;

  /// \brief The GeoArrowDimensions
  uint8_t dimensions;

  /// \brief Flags
  ///
  /// The only currently supported flag is GEOARROW_GEOMETRY_NODE_FLAG_SWAP_ENDIAN
  /// to indicate that coords must be endian-swapped before being interpreted
  /// on the current platform.
  uint8_t flags;

  /// \brief The recursion level
  ///
  /// A level of 0 represents the root geometry and is incremented for
  /// child geometries (e.g., polygon ring or child of a multi geometry
  /// or collection).
  uint8_t level;

  /// \brief User data
  ///
  /// The user data is an opportunity for the producer to attach additional
  /// information to a node or for the consumer to cache information during
  /// processing. The producer nor the consumer must not rely on the value of this
  /// pointer for memory management (i.e., bookkeeping details must be handled
  /// elsewhere).
  const void* user_data;
};

/// \brief View of a geometry represented by a sequence of GeoArrowGeometryNode
///
/// This struct owns neither the array of nodes nor the array(s) of coordinates.
struct GeoArrowGeometryView {
  /// \brief A pointer to the root geometry node
  ///
  /// The memory is managed by the producer of the struct (e.g., a WKBReader
  /// will hold the array of GeoArrowGeometryNode and populate this struct
  /// to communicate the result.
  const struct GeoArrowGeometryNode* root;

  /// \brief The number of valid nodes in the root array
  ///
  /// This can be used when iterating over the geometry to ensure the sizes of
  /// the children are correctly set.
  int64_t size_nodes;
};

/// \brief Variant of the GeoArrowGeometry that owns its GeoArrowGeometryNode and/or
/// its coordinates
struct GeoArrowGeometry {
  /// \brief A pointer to the root geometry node
  struct GeoArrowGeometryNode* root;

  /// \brief The number of valid nodes in the root array
  ///
  /// This can be used when iterating over the geometry to ensure the sizes of
  /// the children are correctly set.
  int64_t size_nodes;

  /// \brief The number of allocated nodes in the root array
  int64_t capacity_nodes;

  /// \brief Opaque data
  void* private_data;
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
/// \ingroup geoarrow-array_view
///
/// This view is capable of representing both struct and interleaved coordinates.
/// Use GEOARROW_COORD_VIEW_VALUE() to generically access an ordinate.
struct GeoArrowCoordView {
  /// \brief Pointers to the beginning of each coordinate buffer
  ///
  /// May be NULL if n_coords is 0. For interleaved coordinates, these
  /// will point to the first n_values elements of the same buffer.
  const double* values[8];

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
  double* values[8];

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
/// \ingroup geoarrow-array_view
#define GEOARROW_COORD_VIEW_VALUE(coords_, row_, col_) \
  (coords_)->values[(col_)][(row_) * (coords_)->coords_stride]

/// \brief A parsed view of memory from a GeoArrow-encoded array
/// \ingroup geoarrow-array_view
///
/// This definition may change to more closely match the GeoArrowWritableArrayView
/// in the future.
struct GeoArrowArrayView {
  /// \brief Type information for the array represented by this view
  struct GeoArrowSchemaView schema_view;

  /// \brief The logical offset to apply into each level of nesting
  int64_t offset[4];

  /// \brief The number of elements in each level of nesting
  int64_t length[4];

  /// \brief The validity bitmap for this array
  const uint8_t* validity_bitmap;

  /// \brief The number of offset buffers for the type represented by this array
  int32_t n_offsets;

  /// \brief Pointers to the beginning of each offset buffer
  const int32_t* offsets[3];

  /// \brief The first offset value in each offset buffer
  int32_t first_offset[3];

  /// \brief The last offset value in each offset buffer
  int32_t last_offset[3];

  /// \brief For serialized types, a pointer to the start of the data buffer
  const uint8_t* data;

  /// \brief Generic view of the coordinates in this array
  struct GeoArrowCoordView coords;
};

/// \brief Structured view of writable memory managed by the GeoArrowBuilder
/// \ingroup geoarrow-builder
struct GeoArrowWritableArrayView {
  /// \brief Type information for the array being built
  struct GeoArrowSchemaView schema_view;

  /// \brief The number of elements that have been added to this array
  int64_t length;

  /// \brief The number of buffers required to represent this type
  int64_t n_buffers;

  /// \brief The number of offset buffers for the array being built
  int32_t n_offsets;

  /// \brief Views into writable memory managed by the GeoArrowBuilder
  struct GeoArrowWritableBufferView buffers[9];

  /// \brief View of writable coordinate memory managed by the GeoArrowBuilder
  struct GeoArrowWritableCoordView coords;
};

/// \brief Builder for GeoArrow-encoded arrays
/// \ingroup geoarrow-builder
struct GeoArrowBuilder {
  /// \brief Structured view of the memory managed privately in private_data
  struct GeoArrowWritableArrayView view;

  /// \brief Implementation-specific data
  void* private_data;
};

/// \brief Visitor for an array of geometries
/// \ingroup geoarrow-visitor
///
/// A structure of function pointers and implementation-specific data used
/// to allow geometry input from an abstract source. The visitor itself
/// does not have a release callback and is not responsible for the
/// lifecycle of any of its members. The order of method calls is essentially
/// the same as the order these pieces of information would be encountered
/// when parsing well-known text or well-known binary.
///
/// Implementations should perform enough checks to ensure that they do not
/// crash if a reader calls its methods in an unexpected order; however, they
/// are free to generate non-sensical output in this case.
///
/// For example: visiting the well-known text "MULTIPOINT (0 1, 2 3)" would
/// result in the following visitor calls:
///
/// - feat_start
/// - geom_start(GEOARROW_GEOMETRY_TYPE_MULTIPOINT, GEOARROW_DIMENSIONS_XY)
/// - geom_start(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY)
/// - coords(0 1)
/// - geom_end()
/// - geom_start(GEOARROW_GEOMETRY_TYPE_POINT, GEOARROW_DIMENSIONS_XY)
/// - coords(2 3)
/// - geom_end()
/// - geom_end()
/// - feat_end()
///
/// Most visitor implementations consume the entire input; however, some
/// return early once they have all the information they need to compute
/// a value for a given feature. In this case, visitors return EAGAIN
/// and readers must pass this value back to the caller who in turn must
/// provide a call to feat_end() to finish the feature.
struct GeoArrowVisitor {
  /// \brief Called when starting to iterate over a new feature
  int (*feat_start)(struct GeoArrowVisitor* v);

  /// \brief Called after feat_start for a null_feature
  int (*null_feat)(struct GeoArrowVisitor* v);

  /// \brief Called after feat_start for a new geometry
  ///
  /// Every non-null feature will have at least one call to geom_start.
  /// Collections (including multi-geometry types) will have nested calls to geom_start.
  int (*geom_start)(struct GeoArrowVisitor* v, enum GeoArrowGeometryType geometry_type,
                    enum GeoArrowDimensions dimensions);

  /// \brief For polygon geometries, called after geom_start at the beginning of a ring
  int (*ring_start)(struct GeoArrowVisitor* v);

  /// \brief Called when a sequence of coordinates is encountered
  ///
  /// This callback may be called more than once (i.e., readers are free to chunk
  /// coordinates however they see fit). The GeoArrowCoordView may represent
  /// either interleaved of struct coordinates depending on the reader implementation.
  int (*coords)(struct GeoArrowVisitor* v, const struct GeoArrowCoordView* coords);

  /// \brief For polygon geometries, called at the end of a ring
  ///
  /// Every call to ring_start must have a matching call to ring_end
  int (*ring_end)(struct GeoArrowVisitor* v);

  /// \brief Called at the end of a geometry
  ///
  /// Every call to geom_start must have a matching call to geom_end.
  int (*geom_end)(struct GeoArrowVisitor* v);

  /// \brief Called at the end of a feature, including null features
  ///
  /// Every call to feat_start must have a matching call to feat_end.
  int (*feat_end)(struct GeoArrowVisitor* v);

  /// \brief Opaque visitor-specific data
  void* private_data;

  /// \brief The error into which the reader and/or visitor can place a detailed
  /// message.
  ///
  /// When a visitor is initializing callbacks and private_data it should take care
  /// to not change the value of error. This value can be NULL.
  struct GeoArrowError* error;
};

/// \brief Generalized compute kernel
///
/// Callers are responsible for calling the release callback when finished
/// using the kernel.
struct GeoArrowKernel {
  /// \brief Called before any batches are pushed to compute the output schema
  /// based on the input schema.
  int (*start)(struct GeoArrowKernel* kernel, struct ArrowSchema* schema,
               const char* options, struct ArrowSchema* out, struct GeoArrowError* error);

  /// \brief Push a batch into the kernel
  ///
  /// Scalar kernels will populate out with the compute result; aggregate kernels
  /// will not.
  int (*push_batch)(struct GeoArrowKernel* kernel, struct ArrowArray* array,
                    struct ArrowArray* out, struct GeoArrowError* error);

  /// \brief Compute the final result
  ///
  /// For aggregate kernels, compute the result based on previous batches.
  /// In theory, aggregate kernels should allow more than one call to
  /// finish; however, this is not tested in any existing code.
  int (*finish)(struct GeoArrowKernel* kernel, struct ArrowArray* out,
                struct GeoArrowError* error);

  /// \brief Release resources held by the kernel
  ///
  /// Implementations must set the kernel->release member to NULL.
  void (*release)(struct GeoArrowKernel* kernel);

  /// \brief Opaque, implementation-specific data
  void* private_data;
};

#ifdef __cplusplus
}
#endif

#endif
