import pandas as _pd
import pyarrow as _pa
from . import lib
from . import pyarrow as _ga


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
                "`geoarrow_type` must inherit from geoarrow.pyarrow.VectorType"
            )

    @classmethod
    def construct_array_type(cls):
        pass

    @classmethod
    def construct_from_string(cls, string):
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
        raise NotImplementedError()


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
