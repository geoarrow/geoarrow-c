
# cython: language_level = 3

"""Low-level geoarrow Python bindings."""

from libc.stdint cimport uint8_t, int64_t, uintptr_t
from libcpp cimport bool
from libcpp.string cimport string

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


cdef extern from "geoarrow.h":
    GeoArrowErrorCode GeoArrowKernelInit(GeoArrowKernel* kernel, const char* name, const char* options)


cdef extern from "geoarrow.hpp" namespace "geoarrow":
    cdef cppclass VectorType:
        VectorType() except +
        VectorType(const VectorType& x) except +
        void MoveFrom(VectorType* other)

        bool valid()
        string error()
        string extension_name()
        string extension_metadata()
        GeoArrowType id()
        GeoArrowGeometryType geometry_type()
        GeoArrowDimensions dimensions()
        GeoArrowCoordType coord_type()
        int num_dimensions()
        GeoArrowEdgeType edge_type()
        GeoArrowCrsType crs_type()
        string crs()

        VectorType WithGeometryType(GeoArrowGeometryType geometry_type)
        VectorType WithCoordType(GeoArrowCoordType coord_type)
        VectorType WithDimensions(GeoArrowDimensions dimensions)
        VectorType WithEdgeType(GeoArrowEdgeType edge_type)
        VectorType WithCrs(const string& crs, GeoArrowCrsType crs_type)

        GeoArrowErrorCode InitSchema(ArrowSchema* schema)

        @staticmethod
        VectorType Make0 "Make"(GeoArrowGeometryType geometry_type,
                                GeoArrowDimensions dimensions,
                                GeoArrowCoordType coord_type,
                                const string& metadata)

        @staticmethod
        VectorType Make1 "Make"(ArrowSchema* schema)

        @staticmethod
        VectorType Make2 "Make"(ArrowSchema* schema, const string& extension_name,
                                const string& metadata)


cdef class SchemaHolder:
    cdef ArrowSchema c_schema

    def __init__(self):
        self.c_schema.release = NULL

    def __del__(self):
        if self.c_schema.release != NULL:
          self.c_schema.release(&self.c_schema)

    def _addr(self):
        return <uintptr_t>&self.c_schema

    def is_valid(self):
        return self.c_schema.release != NULL

    def release(self):
        if self.c_schema.release == NULL:
            raise ValueError('Schema is already released')
        self.c_schema.release(&self.c_schema)


cdef class ArrayHolder:
    cdef ArrowArray c_array

    def __init__(self):
        self.c_array.release = NULL

    def __del__(self):
        if self.c_array.release != NULL:
          self.c_array.release(&self.c_array)

    def _addr(self):
        return <uintptr_t>&self.c_array

    def is_valid(self):
        return self.c_array.release != NULL

    def release(self):
        if self.c_array.release == NULL:
            raise ValueError('Array is already released')
        self.c_array.release(&self.c_array)


cdef class CVectorType:
    cdef VectorType c_vector_type

    def __init__(self):
        pass

    @staticmethod
    cdef _move_from_ctype(VectorType* c_vector_type):
        if not c_vector_type.valid():
            raise ValueError(c_vector_type.error().decode("UTF-8"))
        out = CVectorType()
        out.c_vector_type.MoveFrom(c_vector_type)
        return out

    @property
    def id(self):
        return self.c_vector_type.id()

    @property
    def geometry_type(self):
        return self.c_vector_type.geometry_type()

    @property
    def dimensions(self):
        return self.c_vector_type.dimensions()

    @property
    def coord_type(self):
        return self.c_vector_type.coord_type()

    @property
    def extension_name(self):
        return self.c_vector_type.extension_name().decode("UTF-8")

    @property
    def extension_metadata(self):
        return self.c_vector_type.extension_metadata()

    @property
    def edge_type(self):
        return self.c_vector_type.edge_type()

    @property
    def crs_type(self):
        return self.c_vector_type.crs_type()

    @property
    def crs(self):
        return self.c_vector_type.crs()

    def with_geometry_type(self, GeoArrowGeometryType geometry_type):
        cdef VectorType ctype = self.c_vector_type.WithGeometryType(geometry_type)
        return CVectorType._move_from_ctype(&ctype)

    def with_dimensions(self, GeoArrowDimensions dimensions):
        cdef VectorType ctype = self.c_vector_type.WithDimensions(dimensions)
        return CVectorType._move_from_ctype(&ctype)

    def with_coord_type(self, GeoArrowCoordType coord_type):
        cdef VectorType ctype = self.c_vector_type.WithCoordType(coord_type)
        return CVectorType._move_from_ctype(&ctype)

    def with_edge_type(self, GeoArrowEdgeType edge_type):
        cdef VectorType ctype = self.c_vector_type.WithEdgeType(edge_type)
        return CVectorType._move_from_ctype(&ctype)

    def with_crs(self, string crs, GeoArrowCrsType crs_type):
        cdef VectorType ctype = self.c_vector_type.WithCrs(crs, crs_type)
        return CVectorType._move_from_ctype(&ctype)

    def __eq__(self, other):
        if not isinstance(other, CVectorType):
            return False
        if self.id != other.id or self.edge_type != other.edge_type:
            return False
        if self.crs_type == other.crs_type and self.crs != other.crs:
            return False

        return self.crs_type == other.crs_type

    def to_schema(self):
        out = SchemaHolder()
        cdef int result = self.c_vector_type.InitSchema(&out.c_schema)
        if result != GEOARROW_OK:
            raise ValueError("InitSchema() failed")
        return out

    @staticmethod
    def Make(GeoArrowGeometryType geometry_type,
             GeoArrowDimensions dimensions,
             GeoArrowCoordType coord_type,
             metadata=b''):
        cdef VectorType ctype = VectorType.Make0(geometry_type, dimensions, coord_type, metadata)
        return CVectorType._move_from_ctype(&ctype)

    @staticmethod
    def FromExtension(SchemaHolder schema):
        cdef VectorType ctype = VectorType.Make1(&schema.c_schema)
        return CVectorType._move_from_ctype(&ctype)

    @staticmethod
    def FromStorage(SchemaHolder schema, string extension_name, string extension_metadata):
        cdef VectorType ctype = VectorType.Make2(&schema.c_schema, extension_name, extension_metadata)
        return CVectorType._move_from_ctype(&ctype)

cdef class Kernel:
    cdef GeoArrowKernel c_kernel

    def __init__(self, const char* name):
        cdef const char* cname = <const char*>name
        cdef int result = GeoArrowKernelInit(&self.c_kernel, cname, NULL)
        if result != GEOARROW_OK:
            raise ValueError('GeoArrowKernelInit() failed')

    def __del__(self):
        self.c_kernel.release(&self.c_kernel)

    def start(self, SchemaHolder schema, const char* options):
        cdef GeoArrowError error
        out = SchemaHolder()
        cdef int result = self.c_kernel.start(&self.c_kernel, &schema.c_schema,
                                              options, &out.c_schema, &error)
        if result != GEOARROW_OK:
            raise ValueError(error.message)

        return out

    def push_batch(self, ArrayHolder array):
        cdef GeoArrowError error
        out = ArrayHolder()
        cdef int result = self.c_kernel.push_batch(&self.c_kernel, &array.c_array,
                                                   &out.c_array, &error)
        if result != GEOARROW_OK:
            raise ValueError(error.message)

        return out

    def finish(self):
        cdef GeoArrowError error
        out = ArrayHolder()
        cdef int result = self.c_kernel.finish(&self.c_kernel, &out.c_array, &error)
        if result != GEOARROW_OK:
            raise ValueError(error.message)
