import pandas as pd
import pyarrow as pa
import geoarrow.pandas
import geoarrow.pyarrow as ga


def test_as_geoarrow():
    series = pd.Series(["POINT (0 1)", "POINT (2 3)"])
    ga_series = series.geoarrow.as_geoarrow()
    assert isinstance(ga_series, pd.Series)

    arr = ga.array(ga_series)
    assert arr == ga.as_geoarrow(["POINT (0 1)", "POINT (2 3)"])
