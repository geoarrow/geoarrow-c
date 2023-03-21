
import pyarrow as pa
import pytest

import geoarrow as ga
import geoarrow.lib as lib

def test_schema_holder():
    holder = lib.SchemaHolder()
    assert holder.is_valid() is False
    with pytest.raises(ValueError):
        holder.release()

    pa.int32()._export_to_c(holder._addr())
    assert holder.is_valid() is True
    holder.release()

def test_array_holder():
    holder = lib.ArrayHolder()
    assert holder.is_valid() is False
    with pytest.raises(ValueError):
        holder.release()

    pa.array([1, 2, 3], pa.int32())._export_to_c(holder._addr())
    assert holder.is_valid() is True
    holder.release()

def test_c_vector_type():
    type_obj = lib.CVectorType.Make(
        lib.GeometryType.POINT,
        ga.Dimensions.XY,
        ga.CoordType.SEPARATE
    )

    assert type_obj.geometry_type == ga.GeometryType.POINT
    assert type_obj.dimensions == ga.Dimensions.XY
    assert type_obj.coord_type == ga.CoordType.SEPARATE

    schema = type_obj.to_schema()
    type_obj2 = lib.CVectorType.FromSchema(schema)
    assert type_obj2 == type_obj

    pa_type = pa.DataType._import_from_c(schema._addr())
    pa_type_expected = pa.struct(
        [pa.field('x', pa.float64()), pa.field('y', pa.float64())]
    )
    assert pa_type == pa_type_expected

    # Schema is now released, so we get an error
    with pytest.raises(ValueError):
        lib.CVectorType.FromSchema(schema)

def test_c_vector_type_with():
    type_obj = lib.CVectorType.Make(
        lib.GeometryType.POINT,
        ga.Dimensions.XY,
        ga.CoordType.SEPARATE
    )

    type_linestring = type_obj.with_geometry_type(ga.GeometryType.LINESTRING)
    assert type_linestring.geometry_type == ga.GeometryType.LINESTRING

    type_xyz = type_obj.with_dimensions(ga.Dimensions.XYZ)
    assert type_xyz.dimensions == ga.Dimensions.XYZ

    type_interleaved = type_obj.with_coord_type(ga.CoordType.INTERLEAVED)
    assert type_interleaved.coord_type == ga.CoordType.INTERLEAVED

    type_spherical = type_obj.with_edge_type(ga.EdgeType.SPHERICAL)
    assert type_spherical.edge_type == ga.EdgeType.SPHERICAL

    type_crs = type_obj.with_crs(b'EPSG:1234', ga.CrsType.UNKNOWN)
    assert type_crs.crs_type == ga.CrsType.UNKNOWN
    assert type_crs.crs == b'EPSG:1234'

def test_kernel_void():
    kernel = lib.Kernel(b'void')

    schema_in = lib.SchemaHolder()
    pa.int32()._export_to_c(schema_in._addr())

    schema_out = kernel.start(schema_in, b'')
    schema_out_pa = pa.DataType._import_from_c(schema_out._addr())
    assert schema_out_pa == pa.null()

    array_in = lib.ArrayHolder()
    pa.array([1, 2, 3], pa.int32())._export_to_c(array_in._addr())
    array_out = kernel.push_batch(array_in)
    array_out_pa = pa.Array._import_from_c(array_out._addr(), schema_out_pa)
    assert array_out_pa == pa.array([None, None, None], pa.null())

def test_kernel_init_error():
    with pytest.raises(ValueError):
        lib.Kernel(b'not_a_kernel')
