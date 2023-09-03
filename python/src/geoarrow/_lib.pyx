
# cython: language_level = 3
# cython: linetrace=True

"""Low-level geoarrow Python bindings."""

from libc.stdint cimport uint8_t, int32_t, int64_t, uintptr_t
from cpython cimport Py_buffer, PyObject
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
                          ArrowArray* out, GeoArrowError* error) nogil
        int (*finish)(GeoArrowKernel* kernel, ArrowArray* out,
                      GeoArrowError* error) nogil
        void (*release)(GeoArrowKernel* kernel)
        void* private_data

    struct GeoArrowCoordView:
        const double* values[4]
        int64_t n_coords
        int32_t n_values
        int32_t coords_stride

    struct GeoArrowArrayView:
        GeoArrowSchemaView schema_view
        int64_t offset[4]
        int64_t length[4]
        const uint8_t* validity_bitmap
        int32_t n_offsets
        const int32_t* offsets[3]
        int32_t first_offset[3]
        int32_t last_offset[3]
        GeoArrowCoordView coords



    struct GeoArrowBuilder:
        pass


cdef extern from "geoarrow.h":
    GeoArrowErrorCode GeoArrowKernelInit(GeoArrowKernel* kernel, const char* name, const char* options)

    GeoArrowErrorCode GeoArrowArrayViewInitFromSchema(GeoArrowArrayView* array_view,
                                                      ArrowSchema* schema,
                                                      GeoArrowError* error)

    GeoArrowErrorCode GeoArrowArrayViewSetArray(GeoArrowArrayView* array_view,
                                                ArrowArray* array,
                                                GeoArrowError* error)

    GeoArrowErrorCode GeoArrowBuilderInitFromSchema(GeoArrowBuilder* builder,
                                                    ArrowSchema* schema,
                                                    GeoArrowError* error)

    GeoArrowErrorCode GeoArrowBuilderAppendBuffer(
        GeoArrowBuilder* builder, int64_t i, GeoArrowBufferView value)

    void GeoArrowBuilderReset(GeoArrowBuilder* builder)

    GeoArrowErrorCode GeoArrowBuilderFinish(GeoArrowBuilder* builder,
                                            ArrowArray* array,
                                            GeoArrowError* error)

cdef extern from "geoarrow_python.h":

    GeoArrowErrorCode GeoArrowBuilderSetPyBuffer(GeoArrowBuilder* builder, int64_t i, PyObject* obj,
                                                 const void* ptr, int64_t size)


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
        GeoArrowErrorCode InitStorageSchema(ArrowSchema* schema)

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


class GeoArrowCException(RuntimeError):

    def __init__(self, what, code, message=""):
        self.what = what
        self.code = code
        self.message = message

        if self.message == "":
            super().__init__(f"{self.what} failed ({self.code})")
        else:
            super().__init__(f"{self.what} failed ({self.code}): {self.message}")


cdef class Error:
    cdef GeoArrowError c_error

    def __cinit__(self):
        self.c_error.message[0] = 0

    def raise_message(self, what, code):
        raise GeoArrowCException(what, code, self.c_error.message.decode("UTF-8"))

    @staticmethod
    def raise_error(what, code):
        raise GeoArrowCException(what, code, "")


cdef class SchemaHolder:
    cdef ArrowSchema c_schema

    def __init__(self):
        self.c_schema.release = NULL

    def __dealloc__(self):
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

    def __dealloc__(self):
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

    def __repr__(self):
        ext_name = self.extension_name
        spherical = self.edge_type == GEOARROW_EDGE_TYPE_SPHERICAL
        interleaved = self.coord_type == GEOARROW_COORD_TYPE_INTERLEAVED

        if self.dimensions == GEOARROW_DIMENSIONS_XYZM:
            dims = '_zm'
        elif self.dimensions == GEOARROW_DIMENSIONS_XYZ:
            dims = '_z'
        elif self.dimensions == GEOARROW_DIMENSIONS_XYM:
            dims = '_m'
        else:
            dims = ''

        if spherical and interleaved:
            type_prefix = 'spherical interleaved '
        elif spherical:
            type_prefix = 'spherical '
        elif interleaved:
            type_prefix = 'interleaved '
        else:
            type_prefix = ''

        if self.crs_type == GEOARROW_CRS_TYPE_PROJJSON:
            crs = f' <PROJJSON:{self.crs.decode("UTF-8")}>'
        elif self.crs_type == GEOARROW_CRS_TYPE_UNKNOWN:
            crs = f' <{self.crs.decode("UTF-8")}>'
        else:
            crs = ''

        if len(crs) > 40:
            crs = crs[:36] + '...>'

        return f'{type_prefix}{ext_name}{dims}{crs}'

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

    def to_storage_schema(self):
        out = SchemaHolder()
        cdef int result = self.c_vector_type.InitStorageSchema(&out.c_schema)
        if result != GEOARROW_OK:
            raise ValueError("InitStorageSchema() failed")
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

cdef class CKernel:
    cdef GeoArrowKernel c_kernel
    cdef object cname_str

    def __init__(self, const char* name):
        cdef const char* cname = <const char*>name
        self.cname_str = cname.decode("UTF-8")
        cdef int result = GeoArrowKernelInit(&self.c_kernel, cname, NULL)
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowKernelInit('{self.cname_str}'>)", result)

    def __dealloc__(self):
        if self.c_kernel.release != NULL:
            self.c_kernel.release(&self.c_kernel)

    def start(self, SchemaHolder schema, const char* options):
        cdef Error error = Error()
        out = SchemaHolder()
        cdef int result = self.c_kernel.start(&self.c_kernel, &schema.c_schema,
                                              options, &out.c_schema, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::start()", result)

        return out

    def push_batch(self, ArrayHolder array):
        cdef Error error = Error()
        out = ArrayHolder()
        cdef int result
        with nogil:
            result = self.c_kernel.push_batch(&self.c_kernel, &array.c_array,
                                              &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::push_batch()", result)

        return out

    def finish(self):
        cdef Error error = Error()
        out = ArrayHolder()
        cdef int result
        with nogil:
            result = self.c_kernel.finish(&self.c_kernel, &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::finish()", result)

    def push_batch_agg(self, ArrayHolder array):
        cdef Error error = Error()
        cdef int result = self.c_kernel.push_batch(&self.c_kernel, &array.c_array,
                                                   NULL, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::push_batch()", result)

    def finish_agg(self):
        cdef Error error = Error()
        out = ArrayHolder()
        cdef int result = self.c_kernel.finish(&self.c_kernel, &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message(f"GeoArrowKernel<{self.cname_str}>::finish()", result)

        return out


cdef class CArrayView:
    cdef GeoArrowArrayView c_array_view
    cdef object _base

    def __init__(self, ArrayHolder array, SchemaHolder schema):
        self._base = array

        cdef Error error = Error()
        cdef int result = GeoArrowArrayViewInitFromSchema(&self.c_array_view, &schema.c_schema, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message("GeoArrowArrayViewInitFromSchema()", result)

        result = GeoArrowArrayViewSetArray(&self.c_array_view, &array.c_array, &error.c_error)
        if result != GEOARROW_OK:
            raise ValueError(error.message.decode('UTF-8'))

    def buffers(self):
        buffers = []
        cdef int64_t length

        # Validity not quite implemented
        buf = None
        buffers.append(buf)

        if self.c_array_view.n_offsets > 0:
            buf = CArrayViewBuffer(
                self,
                <uintptr_t>self.c_array_view.offsets[0],
                4,
                self.c_array_view.offset[0] + self.c_array_view.length[0] + 1,
                'i'
            )
            buffers.append(buf)

        if self.c_array_view.n_offsets > 1:
            for i in range(self.c_array_view.n_offsets - 1):
                length = self.c_array_view.last_offset[i]
                buf = CArrayViewBuffer(
                    self,
                    <uintptr_t>self.c_array_view.offsets[i + 1],
                    4,
                    length + 1,
                    'i'
                )
            buffers.append(buf)

        cdef GeoArrowCoordView* coords = &self.c_array_view.coords
        if coords.coords_stride == 1:
            for i in range(coords.n_values):
                buf = CArrayViewBuffer(
                    self,
                    <uintptr_t>coords.values[i],
                    8,
                    coords.n_coords,
                    'd'
                )
                buffers.append(buf)
        elif coords.coords_stride == coords.n_values:
            buf = CArrayViewBuffer(
                self,
                <uintptr_t>coords.values[0],
                8,
                coords.n_coords * coords.n_values,
                'd'
            )
            buffers.append(buf)
        else:
            raise NotImplementedError('Unknown coord type')

        return buffers


cdef class CArrayViewBuffer:
    cdef object _base
    cdef void* _ptr
    cdef Py_ssize_t _item_size
    cdef Py_ssize_t _shape
    cdef str _format

    def __init__(self, base, uintptr_t ptr, item_size_bytes, length_elements, format):
        self._base = base
        self._ptr = <void*>ptr
        self._item_size = item_size_bytes
        self._shape = length_elements
        self._format = format

    def __getbuffer__(self, Py_buffer *buffer, int flags):
        buffer.buf = self._ptr

        if self._format == 'i':
            buffer.format = 'i'
        elif self._format == 'd':
            buffer.format = 'd'
        else:
            buffer.format = NULL

        buffer.internal = NULL
        buffer.itemsize = self._item_size
        buffer.len = self._shape * self._item_size
        buffer.ndim = 1
        buffer.obj = self
        buffer.readonly = 1
        buffer.shape = &self._shape
        buffer.strides = &self._item_size
        buffer.suboffsets = NULL

    def __releasebuffer__(self, Py_buffer *buffer):
        pass


cdef class CBuilder:
    cdef GeoArrowBuilder c_builder
    cdef SchemaHolder _schema

    def __init__(self, SchemaHolder schema):
        self._schema = schema
        cdef Error error = Error()
        cdef int result = GeoArrowBuilderInitFromSchema(&self.c_builder, &schema.c_schema, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message("GeoArrowBuilderInitFromSchema()", result)

    def __dealloc__(self):
        GeoArrowBuilderReset(&self.c_builder)

    def set_buffer_uint8(self, int64_t i, object obj):
        cdef const unsigned char[:] view = memoryview(obj)
        cdef int result = GeoArrowBuilderSetPyBuffer(&self.c_builder, i, <PyObject*>obj, &(view[0]), view.shape[0])
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowBuilderSetPyBuffer()", result)

    def set_buffer_int32(self, int64_t i, object obj):
        cdef const int32_t[:] view = memoryview(obj)
        cdef int result = GeoArrowBuilderSetPyBuffer(&self.c_builder, i, <PyObject*>obj, &(view[0]), view.shape[0] * 4)
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowBuilderSetPyBuffer()", result)

    def set_buffer_double(self, int64_t i, object obj):
        cdef const double[:] view = memoryview(obj)
        cdef int result = GeoArrowBuilderSetPyBuffer(&self.c_builder, i, <PyObject*>obj, &(view[0]), view.shape[0] * 8)
        if result != GEOARROW_OK:
            Error.raise_error("GeoArrowBuilderSetPyBuffer()", result)

    @property
    def schema(self):
        return self._schema

    def finish(self):
        out = ArrayHolder()
        cdef Error error = Error()
        cdef int result = GeoArrowBuilderFinish(&self.c_builder, &out.c_array, &error.c_error)
        if result != GEOARROW_OK:
            error.raise_message("GeoArrowBuilderFinish()", result)
        return out
