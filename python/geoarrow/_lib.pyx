
# cython: language_level = 3

"""Low-level geoarrow Python bindings."""

from libc.stdint cimport uint8_t, int64_t, uintptr_t

cdef extern from "geoarrow_type.h":
    struct ArrowSchema:
        const char* format
        const char* name
        const char* metadata
        int64_t flags
        int64_t n_children
        ArrowSchema** children
        ArrowSchema* dictionary
        void (*release)(ArrowSchema*)
        void* private_data

    struct ArrowArray:
        int64_t length
        int64_t null_count
        int64_t offset
        int64_t n_buffers
        int64_t n_children
        const void** buffers
        ArrowArray** children
        ArrowArray* dictionary
        void (*release)(ArrowArray*)
        void* private_data

    ctypedef int GeoArrowErrorCode
    cdef int GEOARROW_OK

    cpdef enum GeoArrowGeometryType:
        GEOARROW_GEOMETRY_TYPE_GEOMETRY = 0
        GEOARROW_GEOMETRY_TYPE_POINT = 1
        GEOARROW_GEOMETRY_TYPE_LINESTRING = 2
        GEOARROW_GEOMETRY_TYPE_POLYGON = 3
        GEOARROW_GEOMETRY_TYPE_MULTIPOINT = 4
        GEOARROW_GEOMETRY_TYPE_MULTILINESTRING = 5
        GEOARROW_GEOMETRY_TYPE_MULTIPOLYGON = 6
        GEOARROW_GEOMETRY_TYPE_GEOMETRYCOLLECTION = 7

    cpdef enum GeoArrowDimensions:
        GEOARROW_DIMENSIONS_UNKNOWN = 0
        GEOARROW_DIMENSIONS_XY = 1
        GEOARROW_DIMENSIONS_XYZ = 2
        GEOARROW_DIMENSIONS_XYM = 3
        GEOARROW_DIMENSIONS_XYZM = 4

    cpdef enum GeoArrowCoordType:
        GEOARROW_COORD_TYPE_UNKNOWN = 0
        GEOARROW_COORD_TYPE_SEPARATE = 1
        GEOARROW_COORD_TYPE_INTERLEAVED = 2

    cpdef enum GeoArrowType:
        pass

    cpdef enum GeoArrowEdgeType:
        GEOARROW_EDGE_TYPE_PLANAR
        GEOARROW_EDGE_TYPE_SPHERICAL

    cpdef enum GeoArrowCrsType:
        GEOARROW_CRS_TYPE_NONE
        GEOARROW_CRS_TYPE_UNKNOWN
        GEOARROW_CRS_TYPE_PROJJSON

    struct GeoArrowError:
        char message[1024]

    struct GeoArrowStringView:
        const char* data
        int64_t size_bytes

    struct GeoArrowBufferView:
        const uint8_t* data
        int64_t size_bytes

    struct GeoArrowSchemaView:
        ArrowSchema* schema
        GeoArrowStringView extension_name
        GeoArrowStringView extension_metadata
        GeoArrowType type
        GeoArrowGeometryType geometry_type
        GeoArrowDimensions dimensions
        GeoArrowCoordType coord_type

    struct GeoArrowMetadataView:
        GeoArrowStringView metadata
        GeoArrowEdgeType edge_type
        GeoArrowCrsType crs_type
        GeoArrowStringView crs

    struct GeoArrowKernel:
        int (*start)(GeoArrowKernel* kernel, ArrowSchema* schema,
                     const char* options, ArrowSchema* out, GeoArrowError* error)
        int (*push_batch)(GeoArrowKernel* kernel, ArrowArray* array,
                          ArrowArray* out, GeoArrowError* error)
        int (*finish)(GeoArrowKernel* kernel, ArrowArray* out,
                      GeoArrowError* error)
        void (*release)(GeoArrowKernel* kernel)
        void* private_data

cdef extern from "geoarrow_type_inline.h":
    GeoArrowGeometryType GeoArrowGeometryTypeFromType(GeoArrowType type)
    GeoArrowDimensions GeoArrowDimensionsFromType(GeoArrowType type)
    GeoArrowCoordType GeoArrowCoordTypeFromType(GeoArrowType type)
    GeoArrowType GeoArrowMakeType(GeoArrowGeometryType geometry_type,
                                  GeoArrowDimensions dimensions,
                                  GeoArrowCoordType coord_type)

cdef extern from "geoarrow.h":
    GeoArrowErrorCode GeoArrowKernelInit(GeoArrowKernel* kernel, const char* name, const char* options)

    int GeoArrowSchemaInitExtension(ArrowSchema* schema, GeoArrowType type)

    int GeoArrowSchemaViewInit(GeoArrowSchemaView* schema_view,
                               ArrowSchema* schema,
                               GeoArrowError* error)

    int GeoArrowMetadataViewInit(GeoArrowMetadataView* metadata_view,
                                 GeoArrowStringView metadata,
                                 GeoArrowError* error)

    int64_t GeoArrowMetadataSerialize(const GeoArrowMetadataView* metadata_view,
                                      char* out, int64_t n)

    int GeoArrowSchemaSetMetadata(ArrowSchema* schema, const GeoArrowMetadataView* metadata_view)

    int64_t GeoArrowUnescapeCrs(GeoArrowStringView crs, char* out, int64_t n)


cdef class SchemaHolder:
    cdef ArrowSchema c_schema

    def __init__(self):
        self.c_schema.release = NULL

    def __del__(self):
        if self.c_schema.release != NULL:
          self.c_schema.release(&self.c_schema)

    def _addr(self):
        return <uintptr_t>&self.c_schema

cdef class ArrayHolder:
    cdef ArrowArray c_array

    def __init__(self):
        self.c_array.release = NULL

    def __del__(self):
        if self.c_array.release != NULL:
          self.c_array.release(&self.c_array)

    def _addr(self):
        return <uintptr_t>&self.c_array


cdef class Kernel:
    cdef GeoArrowKernel c_kernel

    def __init__(self, const char* name):
        cdef const char* cname = <const char*>name
        cdef int result = GeoArrowKernelInit(&self.c_kernel, NULL, NULL)
        if result != GEOARROW_OK:
            raise ValueError('GeoArrowKernelInit failed')

    def __del__(self):
        self.c_kernel.release(&self.c_kernel)

    def start(self, SchemaHolder schema, **kwargs):
        cdef GeoArrowError error
        out = SchemaHolder()
        cdef int result = self.c_kernel.start(&self.c_kernel, &schema.c_schema,
                                              NULL, &out.c_schema, &error)
        if result != GEOARROW_OK:
            raise ValueError('kernel.start() failed')

    def push_batch(self, ArrayHolder array):
        cdef GeoArrowError error
        out = ArrayHolder()
        cdef int result = self.c_kernel.push_batch(&self.c_kernel, &array.c_array,
                                                   &out.c_array, &error)
        if result != GEOARROW_OK:
            raise ValueError('kernel.push_batch() failed')

    def finish(self):
        cdef GeoArrowError error
        out = ArrayHolder()
        cdef int result = self.c_kernel.finish(&self.c_kernel, &out.c_array, &error)
        if result != GEOARROW_OK:
            raise ValueError('kernel.finish() failed')


def some_function():
    return True
