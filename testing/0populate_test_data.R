
for (nm in names(wk::wk_example_wkt)) {
  readr::write_lines(
    na.omit(wk::wk_example_wkt[[nm]]), sprintf("testing/%s.wkt", nm)
  )
}
