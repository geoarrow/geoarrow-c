
# Building geoarrow documentation

```bash
# run doxygen for the C API
pushd ../src/apidoc
doxygen
popd

# copy the readme into rst so that we can include it from sphinx
pandoc ../README.md --from markdown --to rst -s -o source/README_generated.rst

# Run sphinx to generate the main site
sphinx-build source _build/html
```
