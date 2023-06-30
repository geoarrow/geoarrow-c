import pandas as pd
import pyarrow as pa
from . import pyarrow as ga


@pd.api.extensions.register_series_accessor("geoarrow")
class GeoArrowAccessor:
    def __init__(self, pandas_obj):
        self._obj = pandas_obj

    def as_geoarrow(self, type=None, coord_type=None):
        array_or_chunked = ga.as_geoarrow(self._obj, type=type, coord_type=coord_type)
        return pd.Series(
            array_or_chunked,
            index=self._obj.index,
            dtype=pd.ArrowDtype(array_or_chunked.type),
        )
