import pyarrow as pa
import pytest

import geoarrow.pyarrow as ga
import geoarrow.pyarrow._kernel as _kernel
import geoarrow.pyarrow._compute as _compute


def test_set_max_workers():
    current_max_workers = _compute.set_max_workers(123)
    assert _compute._max_workers == 123
    assert _compute.set_max_workers(current_max_workers) == 123


def test_as_array_or_chunked():
    wkt_array = ga.array(["POINT (0 1)"])
    assert _compute.obj_as_array_or_chunked(wkt_array) is wkt_array

    wkt_chunked = pa.chunked_array([wkt_array])
    assert _compute.obj_as_array_or_chunked(wkt_chunked)

    wkt_array2 = _compute.obj_as_array_or_chunked(["POINT (0 1)"])
    assert wkt_array2.storage == wkt_array.storage

    wkt_array3 = _compute.obj_as_array_or_chunked(wkt_array.storage)
    assert wkt_array3.storage == wkt_array.storage


def test_push_all():
    current_max_workers = _compute.set_max_workers(1)

    wkt_array = ga.array(["POINT (0 1)"])
    wkt_chunked0 = pa.chunked_array([], type=wkt_array.type)
    wkt_chunked1 = pa.chunked_array([wkt_array])
    wkt_chunked2 = pa.chunked_array([wkt_array, ga.array(["POINT (2 3)"])])

    result_array = _compute.push_all(_kernel.Kernel.as_wkt, wkt_array)
    assert result_array.storage == wkt_array.storage

    result_chunked0 = _compute.push_all(_kernel.Kernel.as_wkt, wkt_chunked0)
    assert result_chunked0 == wkt_chunked0

    result_chunked1 = _compute.push_all(_kernel.Kernel.as_wkt, wkt_chunked1)
    for result_chunk, wkt_chunk in zip(result_chunked1.chunks, wkt_chunked1.chunks):
        assert result_chunk.storage == wkt_chunk.storage

    result_chunked2 = _compute.push_all(_kernel.Kernel.as_wkt, wkt_chunked2)
    for result_chunk, wkt_chunk in zip(result_chunked2.chunks, wkt_chunked2.chunks):
        assert result_chunk.storage == wkt_chunk.storage

    _compute.set_max_workers(2)
    result_chunked2 = _compute.push_all(_kernel.Kernel.as_wkt, wkt_chunked2)
    for result_chunk, wkt_chunk in zip(result_chunked2.chunks, wkt_chunked2.chunks):
        assert result_chunk.storage == wkt_chunk.storage

    _compute.set_max_workers(current_max_workers)


def test_parse_all():
    assert _compute.parse_all(["POINT (0 1)"]) is None
    with pytest.raises(ValueError):
        _compute.parse_all(["not valid wkt"])

    geoarrow_array = ga.array(["POINT (0 1)"]).as_geoarrow(ga.point())
    assert _compute.parse_all(geoarrow_array) is None


def test_as_wkt():
    wkt_array = ga.array(["POINT (0 1)"])
    assert _compute.as_wkt(wkt_array) is wkt_array

    assert _compute.as_wkt(wkt_array.as_wkb()).storage == wkt_array.storage


def test_as_wkb():
    wkb_array = ga.array(["POINT (0 1)"]).as_wkb()
    assert _compute.as_wkb(wkb_array) is wkb_array

    assert _compute.as_wkb(wkb_array.as_wkt()).storage == wkb_array.storage


def test_format_wkt():
    wkt_array = ga.array(["POINT (0 1)"])
    assert _compute.format_wkt(wkt_array, max_element_size_bytes=5) == pa.array(
        ["POINT"]
    )


def test_unique_geometry_types():
    ga_array = ga.array(pa.array([], type=pa.utf8())).as_geoarrow(ga.point())
    out = _compute.unique_geometry_types(ga_array).flatten()
    assert out[0] == pa.array([ga.GeometryType.POINT], type=pa.int32())
    assert out[1] == pa.array([ga.Dimensions.XY], type=pa.int32())

    wkt_array = ga.array(
        [
            "POINT ZM (0 1 2 3)",
            "LINESTRING M (0 0 0, 1 1 1)",
            "POLYGON Z ((0 0 0, 1 0 0, 0 1 0, 0 0 0))",
            "MULTIPOINT (0 1)",
        ]
    )

    out = _compute.unique_geometry_types(wkt_array).flatten()
    assert out[0] == pa.array(
        [
            ga.GeometryType.MULTIPOINT,
            ga.GeometryType.POLYGON,
            ga.GeometryType.LINESTRING,
            ga.GeometryType.POINT,
        ],
        type=pa.int32(),
    )

    assert out[1] == pa.array(
        [
            ga.Dimensions.XY,
            ga.Dimensions.XYZ,
            ga.Dimensions.XYM,
            ga.Dimensions.XYZM,
        ],
        type=pa.int32(),
    )


def test_infer_type_common():
    empty = ga.wkt().wrap_array(pa.array([], type=pa.utf8()))
    common = _compute.infer_type_common(empty)
    assert common == pa.null()

    already_geoarrow = ga.array(["POINT (0 1)"]).as_geoarrow(ga.point())
    common = _compute.infer_type_common(already_geoarrow)
    assert common.id == already_geoarrow.type.id
    common_interleaved = _compute.infer_type_common(
        already_geoarrow, coord_type=ga.CoordType.INTERLEAVED
    )
    assert (
        common_interleaved.id
        == already_geoarrow.type.with_coord_type(ga.CoordType.INTERLEAVED).id
    )

    point = ga.wkt().with_crs("EPSG:1234").wrap_array(pa.array(["POINT (0 1)"]))
    common = _compute.infer_type_common(point)
    assert common.id == ga.point().id
    assert common.crs == "EPSG:1234"

    common_promote_multi = _compute.infer_type_common(point, promote_multi=True)
    assert common_promote_multi.id == ga.multipoint().id

    point_z_and_zm = ga.array(["POINT (0 1)", "POINT ZM (0 1 2 3)"])
    common = _compute.infer_type_common(point_z_and_zm)
    assert common.id == ga.point().with_dimensions(ga.Dimensions.XYZM).id

    point_m_and_z = ga.array(["POINT M (0 1 2)", "POINT Z (0 1 2)"])
    common = _compute.infer_type_common(point_m_and_z)
    assert common.id == ga.point().with_dimensions(ga.Dimensions.XYZM).id

    mixed = (
        ga.wkt()
        .with_crs("EPSG:1234")
        .wrap_array(pa.array(["POINT (0 1)", "LINESTRING (0 1, 2 3)"]))
    )
    common = _compute.infer_type_common(mixed)
    assert common.id == ga.wkb().id
    assert common.crs == "EPSG:1234"

    point_and_multi = ga.array(["POINT (0 1)", "MULTIPOINT (2 3)"])
    common = _compute.infer_type_common(point_and_multi)
    assert common.id == ga.multipoint().id

    linestring_and_multi = ga.array(
        ["LINESTRING (0 1, 2 3)", "MULTILINESTRING ((0 1, 2 3))"]
    )
    common = _compute.infer_type_common(linestring_and_multi)
    assert common.id == ga.multilinestring().id

    polygon_and_multi = ga.array(
        ["POLYGON ((0 0, 0 1, 1 0, 0 0))", "MULTIPOLYGON (((0 0, 0 1, 1 0, 0 0)))"]
    )
    common = _compute.infer_type_common(polygon_and_multi)
    assert common.id == ga.multipolygon().id


def test_as_geoarrow():
    array = _compute.as_geoarrow(["POINT (0 1)"])
    assert array.type.id == ga.point().id

    array2 = _compute.as_geoarrow(array)
    assert array2 is array

    array2 = _compute.as_geoarrow(array, coord_type=ga.CoordType.INTERLEAVED)
    assert array2.type.id == ga.point().with_coord_type(ga.CoordType.INTERLEAVED).id

    array = _compute.as_geoarrow(["POINT (0 1)"], coord_type=ga.CoordType.INTERLEAVED)
    assert array.type.id == ga.point().with_coord_type(ga.CoordType.INTERLEAVED).id

    array = _compute.as_geoarrow(["POINT (0 1)"], type=ga.multipoint())
    assert array.type.id == ga.multipoint().id

    array = _compute.as_geoarrow(["POINT (0 1)", "LINESTRING (0 1, 2 3)"])
    assert array.type.id == ga.wkb().id


def test_box():
    wkt_array = ga.array(["POINT (0 1)", "POINT (2 3)"])
    box = _compute.box(wkt_array)
    assert box[0].as_py() == {"xmin": 0, "xmax": 0, "ymin": 1, "ymax": 1}
    assert box[1].as_py() == {"xmin": 2, "xmax": 2, "ymin": 3, "ymax": 3}

    # Test optmizations that zero-copy rearrange the points
    array = _compute.as_geoarrow(["POINT (0 1)", "POINT (2 3)"])
    box2 = _compute.box(array)
    assert box2 == box

    chunked_array = pa.chunked_array([array])
    box2 = _compute.box(chunked_array)
    assert box2 == pa.chunked_array([box])

    # Make sure spherical edges error
    with pytest.raises(TypeError):
        wkt_spherical = _compute.with_edge_type(wkt_array, ga.EdgeType.SPHERICAL)
        _compute.box(wkt_spherical)


def test_box_agg():
    wkt_array = ga.array(["POINT (0 1)", "POINT (2 3)"])
    box = _compute.box_agg(wkt_array)
    assert box.as_py() == {"xmin": 0, "xmax": 2, "ymin": 1, "ymax": 3}

    # Test optmization that uses pyarrow.compute.min_max()
    array = _compute.as_geoarrow(["POINT (0 1)", "POINT (2 3)"])
    box2 = _compute.box_agg(array)
    assert box2 == box

    chunked_array = pa.chunked_array([array])
    box2 = _compute.box_agg(chunked_array)
    assert box2 == box

    # Make sure spherical edges error
    with pytest.raises(TypeError):
        wkt_spherical = _compute.with_edge_type(wkt_array, ga.EdgeType.SPHERICAL)
        _compute.box(wkt_spherical)


def test_rechunk_max_bytes():
    wkt_array = ga.array(
        ["LINESTRING (0 1, 2 3, 4 5)", "LINESTRING (0 1, 2 3)", "POINT (0 1)"]
    )
    not_rechunked = _compute.rechunk(wkt_array, max_bytes=1000)
    assert isinstance(not_rechunked, pa.ChunkedArray)
    assert not_rechunked.chunks[0] == wkt_array

    rechunked = _compute.rechunk(wkt_array, max_bytes=4)
    assert isinstance(rechunked, pa.ChunkedArray)
    assert rechunked.num_chunks == 3
    assert len(rechunked) == 3

    rechunked_chunked_array = _compute.rechunk(
        pa.chunked_array([wkt_array]), max_bytes=4
    )
    assert rechunked_chunked_array == rechunked

    rechunked_with_empty_chunk = _compute.rechunk(
        pa.chunked_array(wkt_array[:0]), max_bytes=4
    )
    assert rechunked_with_empty_chunk == pa.chunked_array([], type=wkt_array.type)


def test_with_edge_type():
    wkt_array = ga.array(["POINT (0 1)", "POINT (2 3)"])
    spherical = _compute.with_edge_type(wkt_array, ga.EdgeType.SPHERICAL)
    assert spherical.type.edge_type == ga.EdgeType.SPHERICAL

    planar = _compute.with_edge_type(spherical, ga.EdgeType.PLANAR)
    assert planar.type.edge_type == ga.EdgeType.PLANAR


def test_with_crs():
    wkt_array = ga.array(["POINT (0 1)", "POINT (2 3)"])
    crsified = _compute.with_crs(wkt_array, "EPSG:1234")
    assert crsified.type.crs == "EPSG:1234"

    crsnope = _compute.with_crs(crsified, None)
    assert crsnope.type.crs == ""
    assert crsnope.type.crs_type == ga.CrsType.NONE


def test_with_coord_type():
    wkt_array = ga.array(["POINT (0 1)", "POINT (2 3)"])
    with_interleaved = _compute.with_coord_type(wkt_array, ga.CoordType.INTERLEAVED)
    assert with_interleaved.type.coord_type == ga.CoordType.INTERLEAVED

    with_struct = _compute.with_coord_type(with_interleaved, ga.CoordType.SEPARATE)
    assert with_struct.type.coord_type == ga.CoordType.SEPARATE


def test_with_dimensions():
    wkt_array = ga.array(["POINT (0 1)", "POINT (2 3)"])
    xyz = _compute.with_dimensions(wkt_array, ga.Dimensions.XYZ)
    assert xyz.type.dimensions == ga.Dimensions.XYZ
    assert _compute.as_wkt(xyz).storage[0].as_py() == "POINT Z (0 1 nan)"

    xyz2 = _compute.with_dimensions(xyz, ga.Dimensions.XYZ)
    assert xyz2 == xyz


def test_with_geometry_type():
    wkt_array = ga.array(["POINT (0 1)", "POINT (2 3)"])
    point = _compute.as_geoarrow(wkt_array)
    multipoint = _compute.with_geometry_type(point, ga.GeometryType.MULTIPOINT)
    assert multipoint.type.geometry_type == ga.GeometryType.MULTIPOINT

    multipoint2 = _compute.with_geometry_type(multipoint, ga.GeometryType.MULTIPOINT)
    assert multipoint2 == multipoint

    point2 = _compute.with_geometry_type(multipoint2, ga.GeometryType.POINT)
    assert point2 == point
