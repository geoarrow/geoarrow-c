# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

import pyarrow as pa
import pytest

import geoarrow.lib as lib
import geoarrow._lib as _lib

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
        _lib.GEOARROW_GEOMETRY_TYPE_POINT,
        _lib.GEOARROW_DIMENSIONS_XY,
        _lib.GEOARROW_COORD_TYPE_SEPARATE
    )

    assert type_obj.geometry_type == _lib.GEOARROW_GEOMETRY_TYPE_POINT
    assert type_obj.dimensions == _lib.GEOARROW_DIMENSIONS_XY
    assert type_obj.coord_type == _lib.GEOARROW_COORD_TYPE_SEPARATE

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
        _lib.GEOARROW_GEOMETRY_TYPE_POINT,
        _lib.GEOARROW_DIMENSIONS_XY,
        _lib.GEOARROW_COORD_TYPE_SEPARATE
    )

    type_linestring = type_obj.with_geometry_type(_lib.GEOARROW_GEOMETRY_TYPE_LINESTRING)
    assert type_linestring.geometry_type == _lib.GEOARROW_GEOMETRY_TYPE_LINESTRING

    type_xyz = type_obj.with_dimensions(_lib.GEOARROW_DIMENSIONS_XYZ)
    assert type_xyz.dimensions == _lib.GEOARROW_DIMENSIONS_XYZ

    type_interleaved = type_obj.with_coord_type(_lib.GEOARROW_COORD_TYPE_INTERLEAVED)
    assert type_interleaved.coord_type == _lib.GEOARROW_COORD_TYPE_INTERLEAVED

    type_spherical = type_obj.with_edge_type(_lib.GEOARROW_EDGE_TYPE_SPHERICAL)
    assert type_spherical.edge_type == _lib.GEOARROW_EDGE_TYPE_SPHERICAL

    type_crs = type_obj.with_crs(b'EPSG:1234', _lib.GEOARROW_CRS_TYPE_UNKNOWN)
    assert type_crs.crs_type == _lib.GEOARROW_CRS_TYPE_UNKNOWN
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
