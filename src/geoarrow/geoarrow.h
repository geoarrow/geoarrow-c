
#ifndef GEOARROW_H_INCLUDED
#define GEOARROW_H_INCLUDED

#include "geoarrow/geoarrow_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \defgroup geoarrow geoarrow C library
///
/// Except where noted, objects are not thread-safe and clients should
/// take care to serialize accesses to methods.
///
/// Because this library is intended to be vendored, it provides full type
/// definitions and encourages clients to stack or statically allocate
/// where convenient.

/// \defgroup geoarrow-utility Utilities and error handling
///
/// The geoarrow C library follows the same error idioms as the nanoarrow C
/// library: GEOARROW_OK is returned on success, and a GeoArrowError is populated
/// with a null-terminated error message otherwise if there is an opportunity to
/// provide one. The provided GeoArrowError can always be NULL if a detailed message
/// is not important to the caller. Pointer output arguments are not modified unless
/// GEOARROW_OK is returned.
///
/// @{

/// \brief Return a version string in the form "major.minor.patch"
const char* GeoArrowVersion(void);

/// \brief Return an integer that can be used to compare versions sequentially
int GeoArrowVersionInt(void);

/// \brief Populate a GeoArrowError using a printf-style format string
GeoArrowErrorCode GeoArrowErrorSet(struct GeoArrowError* error, const char* fmt, ...);

/// \brief Parse a string into a double
GeoArrowErrorCode GeoArrowFromChars(const char* first, const char* last, double* out);

/// \brief Print a double to a buffer
int64_t GeoArrowPrintDouble(double f, uint32_t precision, char* result);

/// @}

/// \defgroup geoarrow-schema Data type creation and inspection
///
/// The ArrowSchema is the ABI-stable way to communicate type information using the
/// Arrow C Data interface. These functions export ArrowSchema objects or parse
/// their content into a more easily inspectable object. All unique memory layouts
/// have a GeoArrowType identifier, most of which can be decomposed into
/// GeoArrowGeometryType, GeoArrowDimensions, and GeoArrowCoordType.
///
/// In addition to memory layout, these functions provide a mechanism to serialize
/// and deserialize Arrow extension type information. The serialization format
/// is a JSON object and three keys are currently encoded: crs_type, crs, and
/// edge_type. The embedded parser is not a complete JSON parser and in some
/// circumstances will accept or transport invalid JSON without erroring.
///
/// Serializing extension type information into an ArrowSchema and parsing an
/// ArrowSchema is expensive and should be avoided where possible.
///
/// @{

/// \brief Initialize an ArrowSchema with a geoarrow storage type
GeoArrowErrorCode GeoArrowSchemaInit(struct ArrowSchema* schema, enum GeoArrowType type);

/// \brief Initialize an ArrowSchema with a geoarrow extension type
GeoArrowErrorCode GeoArrowSchemaInitExtension(struct ArrowSchema* schema,
                                              enum GeoArrowType type);

/// \brief Parse an ArrowSchema extension type into a GeoArrowSchemaView
GeoArrowErrorCode GeoArrowSchemaViewInit(struct GeoArrowSchemaView* schema_view,
                                         const struct ArrowSchema* schema,
                                         struct GeoArrowError* error);

/// \brief Parse an ArrowSchema storage type into a GeoArrowSchemaView
GeoArrowErrorCode GeoArrowSchemaViewInitFromStorage(
    struct GeoArrowSchemaView* schema_view, const struct ArrowSchema* schema,
    struct GeoArrowStringView extension_name, struct GeoArrowError* error);

/// \brief Initialize a GeoArrowSchemaView directly from a GeoArrowType identifier
GeoArrowErrorCode GeoArrowSchemaViewInitFromType(struct GeoArrowSchemaView* schema_view,
                                                 enum GeoArrowType type);

/// \brief Initialize a GeoArrowSchemaView directly from a GeoArrowType identifier
GeoArrowErrorCode GeoArrowMetadataViewInit(struct GeoArrowMetadataView* metadata_view,
                                           struct GeoArrowStringView metadata,
                                           struct GeoArrowError* error);

/// \brief Serialize parsed metadata into JSON
int64_t GeoArrowMetadataSerialize(const struct GeoArrowMetadataView* metadata_view,
                                  char* out, int64_t n);

/// \brief Update extension metadata associated with an existing ArrowSchema
GeoArrowErrorCode GeoArrowSchemaSetMetadata(
    struct ArrowSchema* schema, const struct GeoArrowMetadataView* metadata_view);

/// \brief Update extension metadata associated with an existing ArrowSchema
/// based on the extension metadata of another
GeoArrowErrorCode GeoArrowSchemaSetMetadataFrom(struct ArrowSchema* schema,
                                                const struct ArrowSchema* schema_src);

/// \brief Set a GeoArrowMetadatView with the Crs definition of OGC:CRS84,
/// the most commonly used CRS definition for longitude/latitude.
void GeoArrowMetadataSetLonLat(struct GeoArrowMetadataView* metadata_view);

/// \brief Unescape a coordinate reference system value
///
/// The crs member of the GeoArrowMetadataView is a view into the extension metadata;
/// however, in some cases this will be a quoted string (i.e., `"OGC:CRS84"`) and in
/// others it will be a JSON object (i.e., PROJJSON like
/// `{"some key": "some value", ..}`). When passing this string elsewhere, you will
/// almost always want the quoted value to be unescaped (i.e., the JSON string value),
/// but the JSON object to remain as-is. GeoArrowUnescapeCrs() performs this logic
/// based on the value of the first character.
int64_t GeoArrowUnescapeCrs(struct GeoArrowStringView crs, char* out, int64_t n);

/// @}

/// \defgroup geoarrow-array_view Array inspection
///
/// The GeoArrowArrayView is the primary means by which an ArrowArray of a
/// valid type can be inspected. The GeoArrowArrayView is intended to be
/// initialized once for a given type and re-used for multiple arrays
/// (e.g., in a stream).
///
/// @{

/// \brief Initialize a GeoArrowArrayView from a GeoArrowType identifier
GeoArrowErrorCode GeoArrowArrayViewInitFromType(struct GeoArrowArrayView* array_view,
                                                enum GeoArrowType type);

/// \brief Initialize a GeoArrowArrayView from an ArrowSchema
GeoArrowErrorCode GeoArrowArrayViewInitFromSchema(struct GeoArrowArrayView* array_view,
                                                  const struct ArrowSchema* schema,
                                                  struct GeoArrowError* error);

/// \brief Populate the members of the GeoArrowArrayView from an ArrowArray
GeoArrowErrorCode GeoArrowArrayViewSetArray(struct GeoArrowArrayView* array_view,
                                            const struct ArrowArray* array,
                                            struct GeoArrowError* error);

/// @}

/// \defgroup geoarrow-builder Array creation
///
/// The GeoArrowBuilder supports creating GeoArrow-encoded arrays. There are
/// three ways to do so:
///
/// - Build the individual buffers yourself and transfer ownership to the
///   array for each using GeoArrowBuilderSetOwnedBuffer()
/// - Append the appropriate values to each buffer in-place using
///   GeoArrowBuilderAppendBuffer()
/// - Use GeoArrowBuilderInitVisitor() and let the visitor build the buffers
///   for you.
///
/// For all methods you can re-use the builder object for multiple batches
/// and call GeoArrowBuilderFinish() multiple times. You should
/// use the same mechanism for building an array when reusing a builder
/// object.
///
/// The GeoArrowBuilder models GeoArrow arrays as a sequence of buffers numbered
/// from the outer array inwards. The 0th buffer is always the validity buffer
/// and can be omitted for arrays that contain no null features. This is followed
/// by between 0 (point) and 3 (multipolygon) int32 offset buffers and between
/// 1 (interleaved) and 4 (xyzm struct) double buffers representing coordinate
/// values. The GeoArrowBuilder omits validity buffers for inner arrays since
/// the GeoArrow specification states that these arrays must contain zero nulls.
///
/// @{

/// \brief Initialize memory for a GeoArrowBuilder based on a GeoArrowType identifier
GeoArrowErrorCode GeoArrowBuilderInitFromType(struct GeoArrowBuilder* builder,
                                              enum GeoArrowType type);

/// \brief Initialize memory for a GeoArrowBuilder based on an ArrowSchema
GeoArrowErrorCode GeoArrowBuilderInitFromSchema(struct GeoArrowBuilder* builder,
                                                const struct ArrowSchema* schema,
                                                struct GeoArrowError* error);

/// \brief Reserve additional space for a buffer in a GeoArrowBuilder
GeoArrowErrorCode GeoArrowBuilderReserveBuffer(struct GeoArrowBuilder* builder, int64_t i,
                                               int64_t additional_size_bytes);

/// \brief Append data to a buffer in a GeoArrowBuilder without checking if a reserve
/// is needed
static inline void GeoArrowBuilderAppendBufferUnsafe(struct GeoArrowBuilder* builder,
                                                     int64_t i,
                                                     struct GeoArrowBufferView value);

/// \brief Append data to a buffer in a GeoArrowBuilder
static inline GeoArrowErrorCode GeoArrowBuilderAppendBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value);

/// \brief Replace a buffer with one whose lifecycle is externally managed.
GeoArrowErrorCode GeoArrowBuilderSetOwnedBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value,
    void (*custom_free)(uint8_t* ptr, int64_t size, void* private_data),
    void* private_data);

/// \brief Finish an ArrowArray containing the built input
///
/// This function can be called more than once to support multiple batches.
GeoArrowErrorCode GeoArrowBuilderFinish(struct GeoArrowBuilder* builder,
                                        struct ArrowArray* array,
                                        struct GeoArrowError* error);

/// \brief Free resources held by a GeoArrowBuilder
void GeoArrowBuilderReset(struct GeoArrowBuilder* builder);

/// @}

/// \defgroup geoarrow-kernels Transform Arrays
///
/// The GeoArrow C library provides limited support for transforming arrays.
/// Notably, it provides support for parsing WKT and WKB into GeoArrow
/// native encoding and serializing GeoArrow arrays to WKT and/or WKB.
///
/// The GeoArrowKernel is a generalization of the compute operations available
/// in this build of the GeoArrow C library. Two types of kernels are implemented:
/// scalar and aggregate. Scalar kernels always output an `ArrowArray` of the same
/// length as the input from `push_batch()` and do not output an `ArrowArray` from
/// `finish()`; aggregate kernels do not output an `ArrowArray` from `push_batch()`
/// and output a single `ArrowArray` from `finish()` with no constraint on the length
/// of the array that is produced. For both kernel types, the `ArrowSchema` of the
/// output is returned by the `start()` method, where `options` (serialized in the
/// same form as the `ArrowSchema` metadata member) can also be passed. Current
/// implementations do not validate options except to the extent needed to avoid
/// a crash.
///
/// This is intended to minimize the number of patterns needed in wrapper code rather than
/// be a perfect abstraction of a compute function. Similarly, these kernels are optimized
/// for type coverage rather than performance.
///
/// - void: Scalar kernel that outputs a null array of the same length as the input
///   for each batch.
/// - void_agg: Aggregate kernel that outputs a null array of length 1 for any number
///   of inputs.
/// - visit_void_agg: Aggregate kernel that visits every coordinate of every feature
///   of the input, outputting a null array of length 1 for any number of inputs.
///   This is useful for validating well-known text and well-known binary as it will
///   error for input that cannot be visited completely.
/// - as_wkt: Scalar kernel that outputs the well-known text version of the input
///   as faithfully as possible (including transferring metadata from the input).
///   Arrays with valid `GeoArrowType`s are supported.
/// - as_wkb: Scalar kernel that outputs the well-known binary version of the input
///   as faithfully as possible (including transferring metadata from the input).
///   Arrays with valid `GeoArrowType`s are supported.
/// - as_geoarrow: Scalar kernel that outputs the GeoArrow version of the input
///   as faithfully as possible (including transferring metadata from the input).
///   Arrays with valid `GeoArrowType`s are supported. The type of the output is
///   controlled by the `type` option, specified as a `GeoArrowType` cast to integer.
/// - format_wkt: A variation on as_wkt that supports options `precision`
///   and `max_element_size_bytes`. This kernel is lazy and does not visit an entire
///   feature beyond that required for `max_element_size_bytes`.
/// - unique_geometry_types_agg: An aggregate kernel that collects unique geometry
///   types in the input. The output is a single int32 array of ISO WKB type codes.
/// - box: A scalar kernel that returns the 2-dimensional bounding box by feature.
///   the output bounding box is represented as a struct array with column order
///   xmin, xmax, ymin, ymax. Null features are recorded as a null item in the
///   output; empty features are recorded as Inf, -Inf, Inf, -Inf.
/// - box_agg: An aggregate kernel that returns the 2-dimensional bounding box
///   containing all features of the input in the same form as the box kernel.
///   the result is always length one and is never null. For the purposes of this
///   kernel, nulls are treated as empty.
///
/// @{

/// \brief Initialize memory for a GeoArrowKernel
///
/// If GEOARROW_OK is returned, the caller is responsible for calling the embedded
/// release callback to free any resources that were allocated.
GeoArrowErrorCode GeoArrowKernelInit(struct GeoArrowKernel* kernel, const char* name,
                                     const char* options);

/// @}

/// \defgroup geoarrow-geometry Zero-copy friendly scalar geometries
///
/// The GeoArrowGeometry, GeoArrowGeometry, and GeoArrowGeometryNode form the
/// basis for iterating over single geometries in the GeoArrow C library. Whereas
/// GeoArrow allows a more efficient implementation of many algorithms by treating
/// Arrays as a whole, sometimes the concept of a scalar is needed to interact
/// with other libraries or reduce the complexity of an operation.
///
/// - A GeoArrowGeometryNode is a view of a coordinate sequence (point, linestring, or
///   polygon ring) or size (with children immediately following in depth-first order).
/// - A GeoArrowGeometry is a view of a contiguous sequence of GeoArrowGeometryNodes,
///   and owns neither the array of nodes nor the underlying coordinates.
/// - A GeoArrowGeometry owns its array of nodes and optionally the underlying
///   coordinates.
///
/// This approach is friendly to iteration over a potentially many items with few
/// if any dynamic allocations.
///
/// @{

/// \brief Initialize geometry for a GeoArrowGeometry
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowGeometryReset.
GeoArrowErrorCode GeoArrowGeometryInit(struct GeoArrowGeometry* geom);

/// \brief Free memory associated with a GeoArrowGeometry
void GeoArrowGeometryReset(struct GeoArrowGeometry* geom);

/// \brief Populate the nodes of a GeoArrowGeometry from a GeoArrowGeometryView
///
/// Copies nodes from src into a previously initialized GeoArrowGeometry. On success
/// the destination owns its nodes but not any underlying coordinates.
GeoArrowErrorCode GeoArrowGeometryShallowCopy(struct GeoArrowGeometryView src,
                                              struct GeoArrowGeometry* dst);

/// \brief Populate the coords and nodes of a GeoArrowGeometry from a GeoArrowGeometryView
///
/// Copies nodes and coords from src into a previously initialized GeoArrowGeometry. On
/// success the destination owns its nodes but and any underlying coordinates.
GeoArrowErrorCode GeoArrowGeometryDeepCopy(struct GeoArrowGeometryView src,
                                           struct GeoArrowGeometry* dst);

/// \brief Resize the nodes list
///
/// This can be used to truncate the nodes list to zero before populating
/// its contents with another value. Use GeoArrowGeometryResizeNodesInline()
/// when calling this in a loop.
GeoArrowErrorCode GeoArrowGeometryResizeNodes(struct GeoArrowGeometry* geom,
                                              int64_t size_nodes);

/// \brief Append a node to the nodes list and initialize its contents
///
/// This can be used to truncate the nodes list to zero before populating
/// its contents with another value. Use GeoArrowGeometryAppendNodeInline()
/// when calling this in a loop.
GeoArrowErrorCode GeoArrowGeometryAppendNode(struct GeoArrowGeometry* geom,
                                             struct GeoArrowGeometryNode** out);

/// \brief Export a GeoArrowGeometryView using a GeoArrowVisitor
GeoArrowErrorCode GeoArrowGeometryViewVisit(struct GeoArrowGeometryView geometry,
                                            struct GeoArrowVisitor* v);

/// \brief Export a GeoArrowGeometry using a GeoArrowVisitor
GeoArrowErrorCode GeoArrowGeometryVisit(const struct GeoArrowGeometry* geom,
                                        struct GeoArrowVisitor* v);

/// \brief Build a GeoArrowGeometry using a visitor
void GeoArrowGeometryInitVisitor(struct GeoArrowGeometry* geom,
                                 struct GeoArrowVisitor* v);

/// @}

/// \defgroup geoarrow-visitor Low-level reader/visitor interfaces
///
/// The GeoArrow specification defines memory layouts for many types.
/// Whereas it is more performant to write dedicated conversions
/// between each source and destination type, the number of conversions
/// required prohibits a compact and maintainable general-purpose
/// library.
///
/// Instead, we define the GeoArrowVisitor and provide a means
/// by which to "visit" each feature in an array of geometries for every
/// supported type. Conversely, we provide a GeoArrowVisitor implementation
/// to create arrays of each supported type upon visitation of an arbitrary
/// source. This design also facilitates reusing the readers and writers
/// provided here by other libraries.
///
/// @{

/// \brief Initialize a GeoArrowVisitor with a visitor that does nothing
void GeoArrowVisitorInitVoid(struct GeoArrowVisitor* v);

/// \brief Visit the features of a native GeoArrowArrayView
///
/// The caller must have initialized the GeoArrowVisitor with the appropriate
/// writer before calling this function. This only works with GeoArrowArrayView
/// instances pointing to native arrays, even though the GeoArrowArrayView can
/// handle other types of arrays. Use the GeoArrowArrayReader for arbitrary input.
GeoArrowErrorCode GeoArrowArrayViewVisitNative(const struct GeoArrowArrayView* array_view,
                                               int64_t offset, int64_t length,
                                               struct GeoArrowVisitor* v);

/// \brief GeoArrow native array writer
///
/// This writer writes the "native" memory layouts (i.e., nested lists of
/// coordinates) implemented as a visitor.
struct GeoArrowNativeWriter {
  /// \brief Implementation-specific details
  void* private_data;
};

/// \brief Initialize the memory of a GeoArrowNativeWriter
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowNativeWriterReset().
GeoArrowErrorCode GeoArrowNativeWriterInit(struct GeoArrowNativeWriter* writer,
                                           enum GeoArrowType type);

/// \brief Populate a GeoArrowVisitor pointing to this writer
GeoArrowErrorCode GeoArrowNativeWriterInitVisitor(struct GeoArrowNativeWriter* writer,
                                                  struct GeoArrowVisitor* v);

/// \brief Finish an ArrowArray containing elements from the visited input
///
/// This function can be called more than once to support multiple batches.
GeoArrowErrorCode GeoArrowNativeWriterFinish(struct GeoArrowNativeWriter* writer,
                                             struct ArrowArray* array,
                                             struct GeoArrowError* error);

/// \brief Free resources held by a GeoArrowNativeWriter
void GeoArrowNativeWriterReset(struct GeoArrowNativeWriter* writer);

/// \brief Well-known text writer
///
/// This struct also contains options for well-known text serialization.
/// These options can be modified from the defaults after
/// GeoArrowWKTWriterInit() and before GeoArrowWKTWriterInitVisitor().
///
/// Note that whether or not GeoArrow was compiled with ryu has a significant
/// impact on the output: notably, ryu is locale-independent and much faster.
/// GeoArrow can fall back on using snprintf(); however, this will result in
/// invalid WKT for locales other than the C locale.
struct GeoArrowWKTWriter {
  /// \brief The number of significant digits to include in the output (default: 16)
  int precision;

  /// \brief Set to 0 to use the verbose (but more valid) MULTIPOINT
  /// representation (i.e., MULTIPOINT((0 1), (2 3)))). Defaults to 1 (because
  /// this was the default GEOS behaviour at the time this was written).
  int use_flat_multipoint;

  /// \brief Constrain the maximum size of each element in the returned array
  ///
  /// Use -1 to denote an unlimited size for each element. When the limit is
  /// reached or shortly after, the called handler method will return EAGAIN,
  /// after which it is safe to call feat_end to end the feature. This ensures
  /// that a finite amount of input is consumed if this elemtn is set.
  int64_t max_element_size_bytes;

  /// \brief Implementation-specific details
  void* private_data;
};

/// \brief Initialize the memory of a GeoArrowWKTWriter
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowWKTWriterReset().
GeoArrowErrorCode GeoArrowWKTWriterInit(struct GeoArrowWKTWriter* writer);

/// \brief Populate a GeoArrowVisitor pointing to this writer
void GeoArrowWKTWriterInitVisitor(struct GeoArrowWKTWriter* writer,
                                  struct GeoArrowVisitor* v);

/// \brief Finish an ArrowArray containing elements from the visited input
///
/// This function can be called more than once to support multiple batches.
GeoArrowErrorCode GeoArrowWKTWriterFinish(struct GeoArrowWKTWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error);

/// \brief Free resources held by a GeoArrowWKTWriter
void GeoArrowWKTWriterReset(struct GeoArrowWKTWriter* writer);

/// \brief Well-known text reader
struct GeoArrowWKTReader {
  void* private_data;
};

/// \brief Initialize the memory of a GeoArrowWKTReader
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowWKTReaderReset().
GeoArrowErrorCode GeoArrowWKTReaderInit(struct GeoArrowWKTReader* reader);

/// \brief Visit well-known text
///
/// The caller must have initialized the GeoArrowVisitor with the appropriate
/// writer before calling this function.
GeoArrowErrorCode GeoArrowWKTReaderVisit(struct GeoArrowWKTReader* reader,
                                         struct GeoArrowStringView s,
                                         struct GeoArrowVisitor* v);

/// \brief Free resources held by a GeoArrowWKTReader
void GeoArrowWKTReaderReset(struct GeoArrowWKTReader* reader);

/// \brief ISO well-known binary writer
struct GeoArrowWKBWriter {
  /// \brief Implmentation-specific data
  void* private_data;
};

/// \brief Initialize the memory of a GeoArrowWKBWriter
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowWKBWriterReset().
GeoArrowErrorCode GeoArrowWKBWriterInit(struct GeoArrowWKBWriter* writer);

/// \brief Populate a GeoArrowVisitor pointing to this writer
void GeoArrowWKBWriterInitVisitor(struct GeoArrowWKBWriter* writer,
                                  struct GeoArrowVisitor* v);

/// \brief Finish an ArrowArray containing elements from the visited input
///
/// This function can be called more than once to support multiple batches.
GeoArrowErrorCode GeoArrowWKBWriterFinish(struct GeoArrowWKBWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error);

/// \brief Free resources held by a GeoArrowWKBWriter
void GeoArrowWKBWriterReset(struct GeoArrowWKBWriter* writer);

/// \brief Well-known binary (ISO or EWKB) reader
struct GeoArrowWKBReader {
  /// \brief Implmentation-specific data
  void* private_data;
};

/// \brief Initialize the memory of a GeoArrowWKBReader
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowWKBReaderReset().
GeoArrowErrorCode GeoArrowWKBReaderInit(struct GeoArrowWKBReader* reader);

/// \brief Visit well-known binary
///
/// The caller must have initialized the GeoArrowVisitor with the appropriate
/// writer before calling this function.
GeoArrowErrorCode GeoArrowWKBReaderVisit(struct GeoArrowWKBReader* reader,
                                         struct GeoArrowBufferView src,
                                         struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowWKBReaderRead(struct GeoArrowWKBReader* reader,
                                        struct GeoArrowBufferView src,
                                        struct GeoArrowGeometryView* out,
                                        struct GeoArrowError* error);

/// \brief Free resources held by a GeoArrowWKBWriter
void GeoArrowWKBReaderReset(struct GeoArrowWKBReader* reader);

/// \brief Array reader for any geoarrow extension array
struct GeoArrowArrayReader {
  void* private_data;
};

/// \brief Initialize a GeoArrowArrayReader from a GeoArrowType
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowArrayReaderReset().
GeoArrowErrorCode GeoArrowArrayReaderInitFromType(struct GeoArrowArrayReader* reader,
                                                  enum GeoArrowType type);

/// \brief Initialize a GeoArrowArrayReader from an ArrowSchema
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowArrayReaderReset().
GeoArrowErrorCode GeoArrowArrayReaderInitFromSchema(struct GeoArrowArrayReader* reader,
                                                    const struct ArrowSchema* schema,
                                                    struct GeoArrowError* error);

/// \brief Set a GeoArrowArray to read
GeoArrowErrorCode GeoArrowArrayReaderSetArray(struct GeoArrowArrayReader* reader,
                                              const struct ArrowArray* array,
                                              struct GeoArrowError* error);

/// \brief Visit a GeoArrowArray
///
/// The caller must have initialized the GeoArrowVisitor with the appropriate
/// writer before calling this function.
GeoArrowErrorCode GeoArrowArrayReaderVisit(struct GeoArrowArrayReader* reader,
                                           int64_t offset, int64_t length,
                                           struct GeoArrowVisitor* v);

/// \brief Get a GeoArrowArrayView
///
/// If there is a GeoArrowArrayView underlying this GeoArrowArrayReader, populates
/// out with the internal pointer. Returns an error code if there is no GeoArrowArrayView
/// corresponding to this array.
GeoArrowErrorCode GeoArrowArrayReaderArrayView(struct GeoArrowArrayReader* reader,
                                               const struct GeoArrowArrayView** out);

/// \brief Free resources held by a GeoArrowArrayReader
void GeoArrowArrayReaderReset(struct GeoArrowArrayReader* reader);

/// \brief Generc GeoArrow array writer
struct GeoArrowArrayWriter {
  void* private_data;
};

/// \brief Initialize the memory of a GeoArrowArrayWriter from a GeoArrowType
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowWKTWriterReset().
GeoArrowErrorCode GeoArrowArrayWriterInitFromType(struct GeoArrowArrayWriter* writer,
                                                  enum GeoArrowType type);

/// \brief Initialize the memory of a GeoArrowArrayWriter from an ArrowSchema
///
/// If GEOARROW_OK is returned, the caller is responsible for calling
/// GeoArrowWKTWriterReset().
GeoArrowErrorCode GeoArrowArrayWriterInitFromSchema(struct GeoArrowArrayWriter* writer,
                                                    const struct ArrowSchema* schema);

/// \brief Set the precision to use for array writers writing to WKT
///
/// Returns EINVAL for precision values that are not valid or if the writer
/// is not writing to WKT. Must be called before GeoArrowArrayWriterInitVisitor().
/// The default precision value is 16. See GeoArrowWKTWriter for details.
GeoArrowErrorCode GeoArrowArrayWriterSetPrecision(struct GeoArrowArrayWriter* writer,
                                                  int precision);

/// \brief Set the MULTIPOINT output mode when writing to WKT
///
/// Returns EINVAL if the writer is not writing to WKT. Must be called before
/// GeoArrowArrayWriterInitVisitor(). The default value is 1. See GeoArrowWKTWriter for
/// details.
GeoArrowErrorCode GeoArrowArrayWriterSetFlatMultipoint(struct GeoArrowArrayWriter* writer,
                                                       int flat_multipoint);

/// \brief Populate a GeoArrowVisitor pointing to this writer
GeoArrowErrorCode GeoArrowArrayWriterInitVisitor(struct GeoArrowArrayWriter* writer,
                                                 struct GeoArrowVisitor* v);

/// \brief Finish an ArrowArray containing elements from the visited input
///
/// This function can be called more than once to support multiple batches.
GeoArrowErrorCode GeoArrowArrayWriterFinish(struct GeoArrowArrayWriter* writer,
                                            struct ArrowArray* array,
                                            struct GeoArrowError* error);

/// \brief Free resources held by a GeoArrowArrayWriter
void GeoArrowArrayWriterReset(struct GeoArrowArrayWriter* writer);

/// @}

#ifdef __cplusplus
}
#endif

#include "geoarrow/geoarrow_type_inline.h"

#endif
