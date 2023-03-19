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
