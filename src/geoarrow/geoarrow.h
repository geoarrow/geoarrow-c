
#ifndef GEOARROW_H_INCLUDED
#define GEOARROW_H_INCLUDED

#include <stdint.h>

#include "geoarrow_type.h"

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


/// \defgroup geoarrow-errors Error Handling
///
/// The geoarrow C library follows the same error idioms as the nanoarrow C
/// library: GEOARROW_OK is returned on success, and a GeoArrowError is populated
/// with a null-terminated error message otherwise if there is an opportunity to
/// provide one. The provided GeoArrowError can always be NULL if a detailed message
/// is not important to the caller. Pointer output arguments are not modified unless
/// GEOARROW_OK is returned.
///
/// @{

/// \brief Populate a GeoArrowError using a printf-style format string
GeoArrowErrorCode GeoArrowErrorSet(struct GeoArrowError* error, const char* fmt, ...);

/// @}

/// \defgroup geoarrow-schema Schema creation and inspection
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
                                         struct ArrowSchema* schema,
                                         struct GeoArrowError* error);

/// \brief Parse an ArrowSchema storage type into a GeoArrowSchemaView
GeoArrowErrorCode GeoArrowSchemaViewInitFromStorage(
    struct GeoArrowSchemaView* schema_view, struct ArrowSchema* schema,
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

/// \brief Deprecated function used for backward compatability with very early
/// versions of geoarrow
GeoArrowErrorCode GeoArrowSchemaSetMetadataDeprecated(
    struct ArrowSchema* schema, const struct GeoArrowMetadataView* metadata_view);

/// \brief Update extension metadata associated with an existing ArrowSchema
/// based on the extension metadata of another
GeoArrowErrorCode GeoArrowSchemaSetMetadataFrom(struct ArrowSchema* schema,
                                                struct ArrowSchema* schema_src);

/// \brief Unescape a coordinate reference system value
///
/// The crs member of the GeoArrowMetadataView is a view into the extension metadata;
/// however, in some cases this will be a quoted string (i.e., `"EPSG:4326"`) and in
/// others it will be a JSON object (i.e., PROJJSON like
/// `{"some key": "some value", ..}`). When passing this string elsewhere, you will
/// almost always want the quoted value to be unescaped (i.e., the JSON string value),
/// but the JSON object to remain as-is. GeoArrowUnescapeCrs() performs this logic
/// based on the value of the first character.
int64_t GeoArrowUnescapeCrs(struct GeoArrowStringView crs, char* out, int64_t n);

/// @}

GeoArrowErrorCode GeoArrowArrayViewInitFromType(struct GeoArrowArrayView* array_view,
                                                enum GeoArrowType type);

GeoArrowErrorCode GeoArrowArrayViewInitFromSchema(struct GeoArrowArrayView* array_view,
                                                  struct ArrowSchema* schema,
                                                  struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowArrayViewSetArray(struct GeoArrowArrayView* array_view,
                                            struct ArrowArray* array,
                                            struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowArrayViewVisit(struct GeoArrowArrayView* array_view,
                                         int64_t offset, int64_t length,
                                         struct GeoArrowVisitor* v);

void GeoArrowVisitorInitVoid(struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowKernelInit(struct GeoArrowKernel* kernel, const char* name,
                                     const char* options);

struct GeoArrowWKTWriter {
  int significant_digits;
  int use_flat_multipoint;
  int64_t max_element_size_bytes;
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKTWriterInit(struct GeoArrowWKTWriter* writer);

void GeoArrowWKTWriterInitVisitor(struct GeoArrowWKTWriter* writer,
                                  struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowWKTWriterFinish(struct GeoArrowWKTWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error);

void GeoArrowWKTWriterReset(struct GeoArrowWKTWriter* writer);

struct GeoArrowWKTReader {
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKTReaderInit(struct GeoArrowWKTReader* reader);

GeoArrowErrorCode GeoArrowWKTReaderVisit(struct GeoArrowWKTReader* reader,
                                         struct GeoArrowStringView s,
                                         struct GeoArrowVisitor* v);

void GeoArrowWKTReaderReset(struct GeoArrowWKTReader* reader);

struct GeoArrowWKBWriter {
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKBWriterInit(struct GeoArrowWKBWriter* writer);

void GeoArrowWKBWriterInitVisitor(struct GeoArrowWKBWriter* writer,
                                  struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowWKBWriterFinish(struct GeoArrowWKBWriter* writer,
                                          struct ArrowArray* array,
                                          struct GeoArrowError* error);

void GeoArrowWKBWriterReset(struct GeoArrowWKBWriter* writer);

struct GeoArrowWKBReader {
  void* private_data;
};

GeoArrowErrorCode GeoArrowWKBReaderInit(struct GeoArrowWKBReader* reader);

void GeoArrowWKBReaderReset(struct GeoArrowWKBReader* reader);

GeoArrowErrorCode GeoArrowWKBReaderVisit(struct GeoArrowWKBReader* reader,
                                         struct GeoArrowBufferView src,
                                         struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowBuilderInitFromType(struct GeoArrowBuilder* builder,
                                              enum GeoArrowType type);

GeoArrowErrorCode GeoArrowBuilderInitFromSchema(struct GeoArrowBuilder* builder,
                                                struct ArrowSchema* schema,
                                                struct GeoArrowError* error);

GeoArrowErrorCode GeoArrowBuilderReserveBuffer(struct GeoArrowBuilder* builder, int64_t i,
                                               int64_t additional_size_bytes);

static inline int GeoArrowBuilderBufferCheck(struct GeoArrowBuilder* builder, int64_t i,
                                             int64_t additional_size_bytes);

static inline void GeoArrowBuilderAppendBufferUnsafe(struct GeoArrowBuilder* builder,
                                                     int64_t i,
                                                     struct GeoArrowBufferView value);

static inline GeoArrowErrorCode GeoArrowBuilderAppendBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value);

GeoArrowErrorCode GeoArrowBuilderSetOwnedBuffer(
    struct GeoArrowBuilder* builder, int64_t i, struct GeoArrowBufferView value,
    void (*custom_free)(uint8_t* ptr, int64_t size, void* private_data),
    void* private_data);

GeoArrowErrorCode GeoArrowBuilderInitVisitor(struct GeoArrowBuilder* builder,
                                             struct GeoArrowVisitor* v);

GeoArrowErrorCode GeoArrowBuilderFinish(struct GeoArrowBuilder* builder,
                                        struct ArrowArray* array,
                                        struct GeoArrowError* error);

void GeoArrowBuilderReset(struct GeoArrowBuilder* builder);

#ifdef __cplusplus
}
#endif

#include "geoarrow_type_inline.h"

#endif
