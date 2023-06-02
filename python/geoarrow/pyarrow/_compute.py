from concurrent.futures import ThreadPoolExecutor, wait

import pyarrow as pa
import pyarrow.compute as pc

from ..lib import GeometryType, Dimensions, CoordType, EdgeType
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


def infer_type_common(obj, coord_type=None, promote_multi=False):
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
        dimensions = Dimensions.XYZ
    elif has_m:
        dimensions = Dimensions.XYM
    else:
        dimensions = Dimensions.XY

    unique_geom_types = types[0].unique().to_pylist()
    if len(unique_geom_types) == 1:
        geometry_type = unique_geom_types[0]
    elif all(
        t in (GeometryType.POINT, GeometryType.MULTIPOINT) for t in unique_geom_types
    ):
        geometry_type = GeometryType.MULTIPOINT
    elif all(
        t in (GeometryType.LINESTRING, GeometryType.MULTILINESTRING)
        for t in unique_geom_types
    ):
        geometry_type = GeometryType.MULTILINESTRING
    elif all(
        t in (GeometryType.POLYGON, GeometryType.MULTIPOLYGON)
        for t in unique_geom_types
    ):
        geometry_type = GeometryType.MULTIPOLYGON
    else:
        return (
            _type.wkb()
            .with_edge_type(obj.type.edge_type)
            .with_crs(obj.type.crs, obj.type.crs_type)
        )

    if promote_multi and geometry_type <= GeometryType.POLYGON:
        geometry_type += 3

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


def as_geoarrow(obj, type=None, coord_type=None, promote_multi=False):
    obj = obj_as_array_or_chunked(obj)

    if type is None:
        type = infer_type_common(
            obj, coord_type=coord_type, promote_multi=promote_multi
        )

    if obj.type.id == type.id:
        return obj

    if type.extension_name == "geoarrow.wkt":
        return push_all(Kernel.as_wkt, obj)
    elif type._extension_name == "geoarrow.wkb":
        return push_all(Kernel.as_wkb, obj)
    else:
        return push_all(Kernel.as_geoarrow, obj, args={"type_id": type.id})


def format_wkt(obj, significant_digits=None, max_element_size_bytes=None):
    return push_all(
        Kernel.format_wkt,
        obj,
        args={
            "significant_digits": significant_digits,
            "max_element_size_bytes": max_element_size_bytes,
        },
    )


def _box_point_struct(storage):
    arrays = storage.flatten()
    return pa.StructArray.from_arrays(
        [arrays[0], arrays[0], arrays[1], arrays[1]],
        names=["xmin", "xmax", "ymin", "ymax"],
    )


def box(obj):
    obj = obj_as_array_or_chunked(obj)

    # Spherical edges aren't supported by this algorithm
    if obj.type.edge_type == EdgeType.SPHERICAL:
        raise TypeError("Can't compute box of type with spherical edges")

    # Optimization: a box of points is just x, x, y, y with zero-copy
    # if the coord type is struct
    if obj.type.coord_type == CoordType.SEPARATE and len(obj) > 0:
        if obj.type.geometry_type == GeometryType.POINT and isinstance(
            obj, pa.ChunkedArray
        ):
            chunks = [_box_point_struct(chunk.storage) for chunk in obj.chunks]
            return pa.chunked_array(chunks)
        elif obj.type.geometry_type == GeometryType.POINT:
            return _box_point_struct(obj.storage)

    return push_all(Kernel.box, obj)


def _box_agg_point_struct(arrays):
    out = [list(pc.min_max(array).values()) for array in arrays]
    out_dict = {
        "xmin": out[0][0].as_py(),
        "xmax": out[0][1].as_py(),
        "ymin": out[1][0].as_py(),
        "ymax": out[1][1].as_py(),
    }

    # Apparently pyarrow reorders dict keys when inferring scalar types?
    return pa.scalar(
        out_dict, pa.struct([(nm, pa.float64()) for nm in out_dict.keys()])
    )


def box_agg(obj):
    obj = obj_as_array_or_chunked(obj)

    # Spherical edges aren't supported by this algorithm
    if obj.type.edge_type == EdgeType.SPHERICAL:
        raise TypeError("Can't compute box of type with spherical edges")

    # Optimization: pyarrow's minmax kernel is fast and we can use it if we have struct
    # coords. So far, only a measurable improvement for points.
    if obj.type.coord_type == CoordType.SEPARATE and len(obj) > 0:
        if obj.type.geometry_type == GeometryType.POINT and isinstance(
            obj, pa.ChunkedArray
        ):
            chunks = [chunk.storage.flatten()[:2] for chunk in obj.chunks]
            chunked_x = pa.chunked_array([chunk[0] for chunk in chunks])
            chunked_y = pa.chunked_array([chunk[1] for chunk in chunks])
            return _box_agg_point_struct([chunked_x, chunked_y])
        elif obj.type.geometry_type == GeometryType.POINT:
            return _box_agg_point_struct(obj.storage.flatten()[:2])

    return push_all(Kernel.box_agg, obj, is_agg=True)[0]


def _rechunk_max_bytes_internal(obj, max_bytes, chunks):
    n = len(obj)
    if n == 0:
        return

    if obj.nbytes <= max_bytes or n <= 1:
        chunks.append(obj)
    else:
        _rechunk_max_bytes_internal(obj[: (n // 2)], max_bytes, chunks)
        _rechunk_max_bytes_internal(obj[(n // 2) :], max_bytes, chunks)


def rechunk(obj, max_bytes):
    obj = obj_as_array_or_chunked(obj)
    chunks = []

    if isinstance(obj, pa.ChunkedArray):
        for chunk in obj.chunks:
            _rechunk_max_bytes_internal(chunk, max_bytes, chunks)
    else:
        _rechunk_max_bytes_internal(obj, max_bytes, chunks)

    return pa.chunked_array(chunks, type=obj.type)


def with_coord_type(obj, coord_type):
    return as_geoarrow(obj, coord_type=coord_type)


def with_edge_type(obj, edge_type):
    obj = obj_as_array_or_chunked(obj)
    new_type = obj.type.with_edge_type(edge_type)
    return new_type.wrap_array(obj.storage)


def with_crs(obj, crs, crs_type=None):
    obj = obj_as_array_or_chunked(obj)
    new_type = obj.type.with_crs(crs, crs_type)
    return new_type.wrap_array(obj.storage)


def with_dimensions(obj, dimensions):
    obj = as_geoarrow(obj)
    if dimensions == obj.type.dimensions:
        return obj

    new_type = obj.type.with_dimensions(dimensions)
    return as_geoarrow(obj, type=new_type)


def with_geometry_type(obj, geometry_type):
    obj = as_geoarrow(obj)
    if geometry_type == obj.type.geometry_type:
        return obj

    new_type = obj.type.with_geometry_type(geometry_type)
    return as_geoarrow(obj, type=new_type)
