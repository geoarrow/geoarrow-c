import numpy as np
import pandas as _pd
import pyarrow as _pa
from . import lib
from . import pyarrow as _ga


class GeoArrowExtensionScalar(bytes):
    """Scalar type for GeoArrowExtensionArray

    This is a generic Scalar implementation for a "Geometry". It is currently implemented
    as an immutable subclass of bytes whose value is the well-known binary representation
    of the geometry.
    """

    def __new__(cls, obj, index=None):
        if isinstance(obj, GeoArrowExtensionScalar):
            bytes_value = bytes(obj)
        elif isinstance(obj, bytes):
            bytes_value = obj
        elif isinstance(obj, str):
            wkb_array = _ga.as_wkb([obj])
            bytes_value = wkb_array[0].as_py()
        else:
            wkb_array = _ga.as_wkb(obj[index : (index + 1)])
            bytes_value = wkb_array[0].as_py()

        return super().__new__(cls, bytes_value)

    def __str__(self):
        return self.wkt

    def __repr__(self):
        wkt_array = _ga.format_wkt(
            _ga.array([self]), significant_digits=7, max_element_size_bytes=1024
        )
        return f'GeoArrowExtensionScalar("{wkt_array[0].as_py()}")'

    @property
    def wkt(self):
        """The well-known text representation of this feature."""
        wkt_array = _ga.format_wkt(_ga.array([self]))
        return wkt_array[0].as_py()

    @property
    def wkb(self):
        """The well-known binary representation of this feature."""
        return bytes(self)

    def __eq__(self, other):
        return isinstance(other, GeoArrowExtensionScalar) and bytes(other) == bytes(
            self
        )


class GeoArrowExtensionArray(_pd.api.extensions.ExtensionArray):
    def __init__(self, obj, type=None):
        if type is not None:
            self._dtype = GeoArrowExtensionDtype(type)
            arrow_type = _ga.VectorType._from_ctype(self._dtype._parent)
            self._data = _ga.array(obj, arrow_type)
        else:
            self._data = _ga.array(obj)
            self._dtype = GeoArrowExtensionDtype(self._data.type)

    @classmethod
    def _from_sequence(cls, scalars, *, dtype=None, copy=False):
        return GeoArrowExtensionArray(scalars, type=dtype)

    @classmethod
    def _from_sequence_of_strings(cls, strings, *, dtype=None, copy=False):
        return GeoArrowExtensionArray(strings, dtype)

    def __getitem__(self, item):
        if isinstance(item, int):
            return GeoArrowExtensionScalar(self._data, item)
        elif isinstance(item, slice):
            return GeoArrowExtensionArray(self._data[item])
        elif isinstance(item, list):
            return GeoArrowExtensionArray(self._data[_pa.array(item)])
        else:
            return GeoArrowExtensionArray(self._data.filter(item))

    def __len__(self):
        return len(self._data)

    def __eq__(self, other):
        if isinstance(other, GeoArrowExtensionScalar):
            array = _pa.array(item == other for item in self)
        else:
            array = _pa.array(item == other_item for item, other_item in zip(self, other))

        return array.to_numpy(zero_copy_only=False)

    @property
    def dtype(self):
        return self._dtype

    def nbytes(self):
        return self._data.nbytes

    def take(self, i):
        return GeoArrowExtensionArray(self._data.take(i), self._dtype)

    def isna(self):
        out = self._data.is_null()
        if isinstance(out, _pa.ChunkedArray):
            return out.to_numpy()
        else:
            return out.to_numpy(zero_copy_only=False)

    def copy(self):
        return GeoArrowExtensionArray._from_sequence(self, dtype=self._dtype)

    @classmethod
    def _concat_same_type(cls, to_concat):
        items = list(to_concat)
        if len(items) == 0:
            return GeoArrowExtensionArray([], _ga.wkb())
        if len(items) == 1:
            return items[0]

        types = [item.type for item in to_concat]
        common_type = _ga.vector_type_common(types)

        chunks = []
        for item in to_concat:
            data = item._data
            if isinstance(data, _pa.ChunkedArray):
                for chunk in data.chunks:
                    chunks.append(chunk)
            else:
                chunks.append(item)

        if all(type == common_type for type in types):
            return GeoArrowExtensionArray(_pa.chunked_array(chunks, common_type))
        else:
            chunks = [_ga.as_geoarrow(chunk, type=common_type) for chunk in chunks]
            return GeoArrowExtensionArray(_pa.chunked_array(chunks, common_type))

    def __arrow_array__(self, type=None):
        if type is None or type == self._data.type:
            return self._data

        raise ValueError(
            f"Can't export GeoArrowExtensionArray with type {str(self.dtype)} as {str(type)}"
        )

    def to_numpy(self, dtype=None, copy=False, na_value=None):
        if dtype is None:
            raise TypeError("to_numpy() with dtype != None not supported")
        if dtype is not None:
            raise TypeError("to_numpy() with na_value != None not supported")

        if isinstance(self._data, _pa.ChunkedArray):
            return self._data.to_numpy()
        else:
            return self._data.to_numpy(zero_copy_only=False, writable=copy)


class GeoArrowExtensionDtype(_pd.api.extensions.ExtensionDtype):
    def __init__(self, parent):
        if isinstance(parent, _ga.VectorType):
            self._parent = parent._type
        elif isinstance(parent, lib.CVectorType):
            self._parent = parent
        elif isinstance(parent, GeoArrowExtensionDtype):
            self._parent = parent._parent
        else:
            raise TypeError(
                "`geoarrow_type` must inherit from geoarrow.pyarrow.VectorType, "
                "geoarrow.CVectorType, or geoarrow.pandas.GeoArrowExtensionDtype"
            )

    @property
    def type(self):
        return GeoArrowExtensionScalar

    @classmethod
    def construct_array_type(cls):
        return GeoArrowExtensionArray

    @classmethod
    def construct_from_string(cls, string):
        # TODO: The current string output is not quite sufficient since it does
        # not communicate geometry type or dimensions
        raise NotImplementedError()

    def __repr__(self):
        return f"{type(self).__name__}({repr(self._parent)})"

    def __str__(self):
        ext_name = self._parent.extension_name
        ext_meta = self._parent.extension_metadata.decode("UTF-8")
        if ext_meta == "{}":
            return f"{ext_name}"
        else:
            return f"{ext_name}[{ext_meta}]"

    def __hash__(self):
        return hash(str(self))

    @property
    def name(self):
        return str(self)

    def __from_arrow__(self, array):
        return GeoArrowExtensionArray(array)


@_pd.api.extensions.register_series_accessor("geoarrow")
class GeoArrowAccessor:
    def __init__(self, pandas_obj):
        self._obj = pandas_obj

    def _wrap_series(self, array_or_chunked):
        return _pd.Series(
            array_or_chunked,
            index=self._obj.index,
            dtype=_pd.ArrowDtype(array_or_chunked.type),
        )

    def _obj_is_geoarrow(self):
        return isinstance(self._obj.dtype, _pd.ArrowDtype) and isinstance(
            self._obj.dtype.pyarrow_dtype, _ga.VectorType
        )

    def parse_all(self):
        _ga.parse_all(self._obj)
        return self._obj

    def as_wkt(self):
        return self._wrap_series(_ga.as_wkt(self._obj))

    def as_wkb(self):
        return self._wrap_series(_ga.as_wkb(self._obj))

    def format_wkt(self, significant_digits=None, max_element_size_bytes=None):
        if not self._obj_is_geoarrow():
            raise TypeError("Can't format_wkt() a non-geoarrow Series")

        array_or_chunked = _ga.format_wkt(
            _pa.array(self._obj),
            significant_digits=significant_digits,
            max_element_size_bytes=max_element_size_bytes,
        )
        return self._wrap_series(array_or_chunked)

    def format_wkb(self):
        if not self._obj_is_geoarrow():
            raise TypeError("Can't format_wkb() a non-geoarrow Series")

        array_or_chunked = _ga.as_wkb(_pa.array(self._obj))
        if isinstance(array_or_chunked, _pa.ChunkedArray):
            storage = [chunk.storage for chunk in array_or_chunked.chunks]
            return self._wrap_series(_pa.chunked_array(storage, _pa.binary()))
        else:
            return self._wrap_series(array_or_chunked.storage)

    def as_geoarrow(self, type=None, coord_type=None):
        array_or_chunked = _ga.as_geoarrow(self._obj, type=type, coord_type=coord_type)
        return self._wrap_series(array_or_chunked)

    def bounds(self):
        array_or_chunked = _ga.box(self._obj)
        if isinstance(array_or_chunked, _pa.ChunkedArray):
            flattened = [chunk.flatten() for chunk in array_or_chunked.chunks]
            seriesish = [
                _pa.chunked_array(item, _pa.float64()) for item in zip(*flattened)
            ]
        else:
            seriesish = array_or_chunked.flatten()

        return _pd.DataFrame(
            {
                "xmin": seriesish[0],
                "xmax": seriesish[1],
                "ymin": seriesish[2],
                "ymax": seriesish[3],
            },
            index=self._obj.index,
        )

    def total_bounds(self):
        struct_scalar1 = _ga.box_agg(self._obj)
        return _pd.DataFrame({k: [v] for k, v in struct_scalar1.as_py().items()})

    def with_coord_type(self, coord_type):
        return self._wrap_series(_ga.with_coord_type(self._obj, coord_type))

    def with_edge_type(self, edge_type):
        return self._wrap_series(_ga.with_edge_type(self._obj, edge_type))

    def with_crs(self, crs, crs_type=None):
        return self._wrap_series(_ga.with_crs(self._obj, crs=crs, crs_type=crs_type))

    def with_dimensions(self, dimensions):
        return self._wrap_series(_ga.with_dimensions(self._obj, dimensions))

    def with_geometry_type(self, geometry_type):
        return self.with_geometry_type(_ga.with_coord_type(self._obj, geometry_type))

    def point_coords(self, dimensions=None):
        return self.point_coords(_ga.with_coord_type(self._obj, dimensions))
