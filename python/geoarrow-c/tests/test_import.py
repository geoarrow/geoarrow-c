import gc
import subprocess
import sys
import sysconfig
import weakref
from array import array

import pytest
from geoarrow.c.lib import _lib as lib


@pytest.mark.skipif(
    sys.platform == "emscripten", reason="Emscripten does not support subprocesses"
)
def test_free_threaded_import_does_not_enable_gil():
    if not sysconfig.get_config_var("Py_GIL_DISABLED"):
        return

    result = subprocess.run(
        [
            sys.executable,
            "-c",
            "import geoarrow.c, sys; assert not sys._is_gil_enabled()",
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0, result.stderr


@pytest.mark.skipif(
    sys.platform == "emscripten", reason="Emscripten does not support subprocesses"
)
def test_lib_import_does_not_import_geoarrow_types():
    result = subprocess.run(
        [
            sys.executable,
            "-c",
            "import geoarrow.c.lib, sys; assert 'geoarrow.types' not in sys.modules",
        ],
        check=False,
        capture_output=True,
        text=True,
    )

    assert result.returncode == 0, result.stderr


def test_builder_keeps_python_buffers_alive_until_release():
    type_obj = lib.CGeometryDataType.Make(
        lib.GEOARROW_GEOMETRY_TYPE_POINT,
        lib.GEOARROW_DIMENSIONS_XY,
        lib.GEOARROW_COORD_TYPE_SEPARATE,
    )
    builder = lib.CBuilder(type_obj.to_schema())

    invalid = array("d", [0.0])
    invalid_ref = weakref.ref(invalid)
    with pytest.raises(lib.GeoArrowCException):
        builder.set_buffer_double(100, invalid)
    del invalid
    gc.collect()
    assert invalid_ref() is None

    x1 = array("d", [1.0])
    x1_ref = weakref.ref(x1)
    builder.set_buffer_double(1, x1)
    del x1
    gc.collect()
    assert x1_ref() is not None

    x2 = array("d", [2.0])
    x2_ref = weakref.ref(x2)
    builder.set_buffer_double(1, x2)
    del x2
    gc.collect()
    assert x1_ref() is None
    assert x2_ref() is not None

    y = array("d", [3.0])
    builder.set_buffer_double(2, y)
    out = builder.finish()
    del builder
    gc.collect()
    assert x2_ref() is not None

    out.release()
    gc.collect()
    assert x2_ref() is None
