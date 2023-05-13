from concurrent.futures import ThreadPoolExecutor, wait

import pyarrow as pa

from ..lib import GeometryType, Dimensions, CoordType
from . import _type
from ._array import array
from ._kernel import Kernel

_max_workers = 1


def set_max_workers(max_workers=None):
    global _max_workers
    if max_workers is None:
        max_workers = pa.cpu_count()

    prev = _max_workers
    _max_workers = max_workers
    return prev


def obj_as_array_or_chunked(obj_in):
    if (
        isinstance(obj_in, pa.Array) or isinstance(obj_in, pa.ChunkedArray)
    ) and isinstance(obj_in.type, _type.VectorType):
        return obj_in
    else:
        return array(obj_in, validate=False)


def construct_kernel_and_push1(kernel_constructor, obj, args):
    kernel = kernel_constructor(obj.type, **args)
    return kernel.push(obj)


def push_all(
    kernel_constructor, obj, args=None, is_agg=False, max_workers=None, result=True
):
    if args is None:
        args = {}

    if is_agg:
        kernel = kernel_constructor(obj.type, **args)
        kernel.push(obj)
        return kernel.finish()

    if max_workers is None:
        max_workers = _max_workers

    if isinstance(obj, pa.Array) or obj.num_chunks <= 1 or max_workers == 1:
        return construct_kernel_and_push1(kernel_constructor, obj, args)

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = []
        for chunk in obj.chunks:
            future = executor.submit(
                construct_kernel_and_push1, kernel_constructor, chunk, args
            )
            futures.append(future)

        # TODO: This doesn't cancel on Control-C properly
        done, not_done = 0, futures
        while not_done:
            done, not_done = wait(futures, 0.5, return_when="FIRST_EXCEPTION")

        if result:
            chunks_out = [future.result() for future in futures]
            return pa.chunked_array(chunks_out)


def parse_all(obj):
    obj = obj_as_array_or_chunked(obj)

    # Non-wkb or wkt types are a no-op here since they don't need parsing
    if isinstance(obj.type, _type.WkbType) or isinstance(obj.type, _type.WktType):
        push_all(Kernel.visit_void_agg, obj, result=False)

    return None


def unique_geometry_types(obj):
    obj = obj_as_array_or_chunked(obj)
    out_type = pa.struct([("geometry_type", pa.int32()), ("dimensions", pa.int32())])

    if not isinstance(obj.type, _type.WktType) and not isinstance(
        obj.type, _type.WkbType
    ):
        return pa.array(
            [
                {
                    "geometry_type": obj.type.geometry_type,
                    "dimensions": obj.type.dimensions,
                }
            ],
            type=out_type,
        )

    result = push_all(Kernel.unique_geometry_types_agg, obj, is_agg=True)

    # Kernel currently returns ISO code integers (e.g., 2002):
    # convert to struct(geometry_type, dimensions)
    py_geometry_types = []
    for item in result:
        item_int = item.as_py()
        if item_int >= 3000:
            dimensions = Dimensions.XYZM
            item_int -= 3000
        elif item_int >= 2000:
            dimensions = Dimensions.XYM
            item_int -= 2000
        elif item_int >= 1000:
            dimensions = Dimensions.XYZ
            item_int -= 1000
        else:
            dimensions = Dimensions.XY

        py_geometry_types.append({"geometry_type": item_int, "dimensions": dimensions})

    return pa.array(py_geometry_types, type=out_type)


def infer_type_common(obj, coord_type=None):
    obj = obj_as_array_or_chunked(obj)

    if not isinstance(obj.type, _type.WktType) and not isinstance(
        obj.type, _type.WkbType
    ):
        if coord_type is None:
            return obj.type
        else:
            return obj.type.with_coord_type(coord_type)

    if coord_type is None:
        coord_type = CoordType.SEPARATE

    types = unique_geometry_types(obj)
    if len(types) == 0:
        # Not ideal: we probably want a _type.empty() that keeps the CRS
        return pa.null()

    types = types.flatten()

    unique_dims = types[1].unique().to_pylist()
    has_z = any(dim in (Dimensions.XYZ, Dimensions.XYZM) for dim in unique_dims)
    has_m = any(dim in (Dimensions.XYM, Dimensions.XYZM) for dim in unique_dims)
    if has_z and has_m:
        dimensions = Dimensions.XYZM
    elif has_z:
        dimensions = Dimensions.XYM
    elif has_m:
        dimensions = Dimensions.XYZ
    else:
        dimensions = Dimensions.XY

    unique_geom_types = types[0].unique().to_pylist()
    if len(unique_geom_types) == 1:
        geometry_type = unique_geom_types[0]
    elif all(unique_geom_types in (GeometryType.POINT, GeometryType.MULTIPOINT)):
        geometry_type = GeometryType.MULTIPOINT
    elif all(
        unique_geom_types in (GeometryType.LINESTRING, GeometryType.MULTILINESTRING)
    ):
        geometry_type = GeometryType.MULTILINESTRING
    elif all(unique_geom_types in (GeometryType.POLYGON, GeometryType.MULTIPOLYGON)):
        geometry_type = GeometryType.MULTIPOLYGON
    else:
        return _type.wkb().with_crs(obj.type.crs, obj.type.crs_type)

    return _type.vector_type(
        geometry_type,
        dimensions,
        coord_type,
        edge_type=obj.type.edge_type,
        crs=obj.type.crs,
        crs_type=obj.type.crs_type,
    )


def as_wkt(obj):
    obj = obj_as_array_or_chunked(obj)

    if isinstance(obj.type, _type.WktType):
        return obj

    return push_all(Kernel.as_wkt, obj)


def as_wkb(obj):
    obj = obj_as_array_or_chunked(obj)

    if isinstance(obj.type, _type.WkbType):
        return obj

    return push_all(Kernel.as_wkb, obj)


def format_wkt(obj, significant_digits=None, max_element_size_bytes=None):
    return push_all(
        Kernel.format_wkt,
        obj,
        args={
            "significant_digits": significant_digits,
            "max_element_size_bytes": max_element_size_bytes,
        },
    )
