
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
    expected_storage = pa.struct(
        [pa.field('x', pa.float64()), pa.field('y', pa.float64())]
    )
    assert pa_type.storage_type == expected_storage
