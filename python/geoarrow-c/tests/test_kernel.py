import pytest
from geoarrow.c import AggregateFunction, ScalarFunction, lib
from geoarrow.c.kernel import _pack_options

pa = pytest.importorskip("pyarrow")


class ArrayProvider:
    def __init__(self, array):
        self.array = array

    def __arrow_c_array__(self, requested_schema=None):
        return self.array.__arrow_c_array__(requested_schema)


class StreamProvider:
    def __init__(self, array):
        self.array = array

    def __arrow_c_stream__(self, requested_schema=None):
        return self.array.__arrow_c_stream__(requested_schema)


def test_scalar_function_call_array():
    function = ScalarFunction("void")

    assert function.name == "void"
    result = function(ArrayProvider(pa.array([1, 2, 3])))
    assert isinstance(result, lib.ArrayHolder)
    assert pa.array(result) == pa.nulls(3)
    assert pa.array(function(pa.array(["a", "b"]))) == pa.nulls(2)


def test_scalar_function_call_chunked_array():
    function = ScalarFunction("void")
    array = pa.chunked_array([[1, 2], [3]], type=pa.int32())

    result_holder = function(StreamProvider(array))
    result = pa.chunked_array(result_holder)

    assert isinstance(result_holder, lib.ArrayStreamHolder)
    assert isinstance(result, pa.ChunkedArray)
    assert result.type == pa.null()
    assert [len(chunk) for chunk in result.chunks] == [2, 1]


def test_aggregate_function_call():
    function = AggregateFunction("void_agg")
    array = pa.chunked_array([[1, 2], [3]], type=pa.int32())

    assert function.name == "void_agg"
    result = function(StreamProvider(array))
    assert isinstance(result, lib.ArrayHolder)
    assert pa.array(result) == pa.nulls(1)
    assert pa.array(function(ArrayProvider(pa.array([4])))) == pa.nulls(1)


def test_function_kind_is_explicit():
    with pytest.raises(ValueError, match="Aggregate kernel"):
        ScalarFunction("void_agg")
    with pytest.raises(ValueError, match="Scalar kernel"):
        AggregateFunction("void")


@pytest.mark.parametrize("function_cls", [ScalarFunction, AggregateFunction])
def test_function_requires_string_name(function_cls):
    with pytest.raises(TypeError, match="name.*str"):
        function_cls(b"void")


@pytest.mark.parametrize(
    "function", [ScalarFunction("void"), AggregateFunction("void_agg")]
)
def test_function_requires_array(function):
    with pytest.raises(TypeError, match="__arrow_c_array__"):
        function([1, 2, 3])


def test_function_options_omit_none():
    function = ScalarFunction("void", unused=None)
    assert pa.array(function(pa.array([1]))) == pa.nulls(1)


@pytest.mark.parametrize("options", [{}, {"precision": None}])
def test_empty_options_include_metadata_count(options):
    assert _pack_options(options) == b"\x00\x00\x00\x00"


def test_function_empty_stream():
    array = pa.chunked_array([], type=pa.int32())
    result = pa.chunked_array(ScalarFunction("void")(array))
    assert result.type == pa.null()
    assert result.num_chunks == 0
    assert pa.array(AggregateFunction("void_agg")(array)) == pa.nulls(1)


@pytest.mark.parametrize("as_stream", [False, True])
def test_scalar_function_infers_schema_per_call(as_stream):
    gt = pytest.importorskip("geoarrow.types")
    function = ScalarFunction("format_wkt", precision=2)
    expected = pa.array(["POINT (1.23 2.35)", None, "POINT (3 4)"])

    for spec in [gt.wkt(), gt.large_wkt()]:
        array = pa.array(
            ["POINT (1.234 2.346)", None, "POINT (3 4)"], type=spec.to_pyarrow()
        )
        if as_stream:
            result = pa.chunked_array(
                function(pa.chunked_array([array[:1], array[1:]]))
            )
            assert [len(chunk) for chunk in result.chunks] == [1, 2]
            assert result.combine_chunks() == expected
        else:
            assert pa.array(function(ArrayProvider(array))) == expected


@pytest.mark.parametrize("as_stream", [False, True])
def test_aggregate_function_has_independent_calls(as_stream):
    gt = pytest.importorskip("geoarrow.types")
    function = AggregateFunction("box_agg")
    results = []

    for spec, offset in [(gt.wkt(), 0), (gt.large_wkt(), 10)]:
        array = pa.array(
            [f"POINT ({offset} 1)", f"POINT ({offset + 2} 3)"],
            type=spec.to_pyarrow(),
        )
        if as_stream:
            array = pa.chunked_array([array[:1], array[1:]])
        results.append(function(array))

    for result, offset in zip(results, [0, 10]):
        assert pa.array(result).to_pylist() == [
            {"xmin": offset, "ymin": 1.0, "xmax": offset + 2, "ymax": 3.0}
        ]


@pytest.mark.parametrize("as_stream", [False, True])
@pytest.mark.parametrize(
    "function", [ScalarFunction("format_wkt"), AggregateFunction("box_agg")]
)
def test_function_can_be_reused_after_error(function, as_stream):
    gt = pytest.importorskip("geoarrow.types")
    type_in = gt.wkt().to_pyarrow()
    invalid = pa.array(["POINT (0 0)", "invalid"], type=type_in)
    valid = pa.array(["POINT (1 2)"], type=type_in)
    if as_stream:
        invalid = pa.chunked_array([invalid[:1], invalid[1:]])
        valid = pa.chunked_array([valid])

    with pytest.raises(lib.GeoArrowCException):
        function(pa.chunked_array([pa.nulls(1)]) if as_stream else pa.nulls(1))

    with pytest.raises(lib.GeoArrowCException):
        function(invalid)

    result = function(valid)
    if isinstance(function, ScalarFunction):
        result = pa.chunked_array(result) if as_stream else pa.array(result)
        assert result.to_pylist() == ["POINT (1 2)"]
    else:
        assert pa.array(result).to_pylist() == [
            {"xmin": 1.0, "ymin": 2.0, "xmax": 1.0, "ymax": 2.0}
        ]
