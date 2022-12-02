
# Test data

Currently generated in R using:

```r
for (nm in names(wk::wk_example_wkt)) {
  readr::write_lines(
    na.omit(wk::wk_example_wkt[[nm]]),
    sprintf("testing/%s.wkt", nm)
  )
}

for (nm in names(wk::wk_example_wkt)) {
  ewkb_val <- wk::as_wkb(readr::read_lines(sprintf("testing/%s.wkt", nm)))
  readr::write_file(unlist(unclass(ewkb_val)), sprintf("testing/%s.ewkb", nm))
  wkb_val <- sf::st_as_binary(sf::st_as_sfc(ewkb_val))
  readr::write_file(unlist(unclass(wkb_val)), sprintf("testing/%s.wkb", nm))
}
```
