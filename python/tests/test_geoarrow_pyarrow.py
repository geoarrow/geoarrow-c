
import sys

import pyarrow as pa
import pytest

import geoarrow.lib as lib
import geoarrow.pyarrow as ga

def test_vector_type_basic():
    ctype = lib.CVectorType.Make(
        ga.GeometryType.POINT,
        ga.Dimensions.XY,
        ga.CoordType.SEPARATE
    )

    pa_type = ga.PointType(ctype)

    assert pa_type.geometry_type == ga.GeometryType.POINT
    assert pa_type.dimensions == ga.Dimensions.XY
    assert pa_type.coord_type == ga.CoordType.SEPARATE

    expected_storage = pa.struct(
        [pa.field('x', pa.float64()), pa.field('y', pa.float64())]
    )
    assert pa_type.storage_type == expected_storage

    with pytest.raises(ValueError):
        ga.LinestringType(ctype)

def test_vector_type_with():
    ctype = lib.CVectorType.Make(
        ga.GeometryType.POINT,
        ga.Dimensions.XY,
        ga.CoordType.SEPARATE
    )

    type_obj = ga.PointType(ctype)

    type_linestring = type_obj.with_geometry_type(ga.GeometryType.LINESTRING)
    assert type_linestring.geometry_type == ga.GeometryType.LINESTRING

    type_xyz = type_obj.with_dimensions(ga.Dimensions.XYZ)
    assert type_xyz.dimensions == ga.Dimensions.XYZ

    type_interleaved = type_obj.with_coord_type(ga.CoordType.INTERLEAVED)
    assert type_interleaved.coord_type == ga.CoordType.INTERLEAVED

    type_spherical = type_obj.with_edge_type(ga.EdgeType.SPHERICAL)
    assert type_spherical.edge_type == ga.EdgeType.SPHERICAL

    type_crs = type_obj.with_crs('EPSG:1234', ga.CrsType.UNKNOWN)
    assert type_crs.crs_type == ga.CrsType.UNKNOWN
    assert type_crs.crs == 'EPSG:1234'

def test_constructors():
    assert ga.wkb().extension_name == 'geoarrow.wkb'
    assert ga.large_wkb().extension_name == 'geoarrow.wkb'
    assert ga.wkt().extension_name == 'geoarrow.wkt'
    assert ga.large_wkt().extension_name == 'geoarrow.wkt'
    assert ga.point().extension_name == 'geoarrow.point'
    assert ga.linestring().extension_name == 'geoarrow.linestring'
    assert ga.polygon().extension_name == 'geoarrow.polygon'
    assert ga.multipoint().extension_name == 'geoarrow.multipoint'
    assert ga.multilinestring().extension_name == 'geoarrow.multilinestring'
    assert ga.multipolygon().extension_name == 'geoarrow.multipolygon'

    generic = ga.vector_type(
        ga.GeometryType.POINT,
        ga.Dimensions.XYZ,
        ga.CoordType.INTERLEAVED,
        ga.EdgeType.SPHERICAL,
        'EPSG:1234'
    )
    assert generic.geometry_type == ga.GeometryType.POINT
    assert generic.dimensions == ga.Dimensions.XYZ
    assert generic.coord_type == ga.CoordType.INTERLEAVED
    assert generic.edge_type == ga.EdgeType.SPHERICAL
    assert generic.crs == 'EPSG:1234'
    assert generic.crs_type == ga.CrsType.UNKNOWN

def test_register_extension_types():
    # Unregistering once is ok
    ga.unregister_extension_types(lazy=False)

    # Unregistering twice with lazy=True is ok
    ga.unregister_extension_types(lazy=True)

    # Unregistering twice with lazy=False is not
    with pytest.raises(RuntimeError):
        ga.unregister_extension_types(lazy=False)

    # Same concept with registration
    ga.register_extension_types(lazy=False)
    ga.register_extension_types(lazy=True)
    with pytest.raises(RuntimeError):
        ga.register_extension_types(lazy=False)

    # Reset state
    ga.unregister_extension_types()
    ga.register_extension_types()
    assert ga._extension_types_registered is True


def test_array():
    array = ga.array(["POINT (30 10)"])
    assert array.type == ga.wkt()

    wkb_item = b'\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3e\x40\x00\x00\x00\x00\x00\x00\x24\x40'
    array = ga.array([wkb_item])
    assert array.type == ga.wkb()

    with pytest.raises(TypeError):
        ga.array([1])

    array = ga.array(["POINT (30 10)"], ga.wkt())
    assert array.type == ga.wkt()
    assert array.type.storage_type == pa.utf8()

    # Validation not yet supported for large types
    array = ga.array(["POINT (30 10)"], ga.large_wkt(), validate=False)
    assert array.type == ga.large_wkt()
    assert array.type.storage_type == pa.large_utf8()

    array = ga.array([wkb_item], ga.wkb())
    assert array.type == ga.wkb()
    assert array.type.storage_type == pa.binary()

    # Validation not yet supported for large types
    array = ga.array([wkb_item], ga.large_wkb(), validate=False)
    assert array.type == ga.large_wkb()
    assert array.type.storage_type == pa.large_binary()

def test_kernel_void():
    with pytest.raises(TypeError):
        kernel = ga.Kernel.void(pa.int32())
        kernel.push(5)

    array = ga.array(['POINT (30 10)'])
    kernel = ga.Kernel.void(array.type)
    out = kernel.push(array)
    assert out.type == pa.null()
    assert len(out) == 1

    array = ga.array(['POINT (30 10)', 'POINT (31 11)'])
    kernel = ga.Kernel.void_agg(array.type)
    assert kernel.push(array) is None
    out = kernel.finish()
    assert out.type == pa.null()
    assert len(out) == 1

def test_kernel_as():
    array = ga.array(['POINT (30 10)'], ga.wkt().with_crs('EPSG:1234'))
    kernel = ga.Kernel.as_wkt(array.type)
    out = kernel.push(array)
    assert out.type.extension_name == 'geoarrow.wkt'
    assert out.type.crs == 'EPSG:1234'
    assert isinstance(out, ga.VectorArray)

    array = ga.array(['POINT (30 10)'], ga.wkt().with_crs('EPSG:1234'))
    kernel = ga.Kernel.as_wkb(array.type)
    out = kernel.push(array)
    assert out.type.extension_name == 'geoarrow.wkb'
    assert out.type.crs == 'EPSG:1234'
    assert isinstance(out, ga.VectorArray)

    if sys.byteorder == 'little':
        wkb_item = b'\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3e\x40\x00\x00\x00\x00\x00\x00\x24\x40'
        assert out[0].as_py() == wkb_item

def test_kernel_visit_void():
    array = ga.array(['POINT (30 10)'], ga.wkt())
    kernel = ga.Kernel.visit_void_agg(array.type)
    assert kernel.push(array) is None
    out = kernel.finish()
    assert out.type == pa.null()
    assert len(out) == 1

    array = ga.array(['POINT (30 10)', 'NOT VALID WKT AT ALL'], ga.wkt(), validate=False)
    kernel = ga.Kernel.visit_void_agg(array.type)
    with pytest.raises(ValueError):
        kernel.push(array) is None
    out = kernel.finish()
    assert out.type == pa.null()
    assert len(out) == 1
