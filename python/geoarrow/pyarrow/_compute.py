import json

import pyarrow as pa

from . import _type
from ._array import array
from ._kernel import Kernel


def _geometry_column_from_schema(schema):
    geo = schema.metadata[b"geo"] if b"geo" in schema.metadata else "{}"
    geo = json.loads(geo)

    if "primary_column" in geo and geo["primary_column"] in schema.names:
        return geo["primary_column"]
    elif "geometry" in schema.names:
        return "geometry"

    for i, col_type in enumerate(schema.types):
        if isinstance(col_type, _type.VectorType):
            return schema.names[i]

    raise ValueError(f"Can't guess geometry column from schema:\n{schema}")


def _obj_as_array_or_chunked(obj_in):
    if isinstance(obj_in, pa.Array) or isinstance(obj_in, pa.ChunkedArray) and isinstance(obj_in.type, _type.VectorType):
        return obj_in
    elif isinstance(obj_in, pa.RecordBatch) or isinstance(obj_in, pa.Table):
        geo_col_name = _geometry_column_from_schema(obj_in.schema)
        return _obj_as_array_or_chunked(obj_in.column(geo_col_name))
    else:
        return array(obj_in)

def assert_valid(obj):
    obj = _obj_as_array_or_chunked(obj)

    # Non-wkb or wkt types
    if isinstance(obj, _type.WkbType) or isinstance(obj, _type.WktType):
        validator = Kernel.visit_void_agg(obj.type)
        validator.push(obj)
    return None
