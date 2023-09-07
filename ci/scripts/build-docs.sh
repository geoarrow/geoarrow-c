
set -e
set -o pipefail

if [ ${VERBOSE:-0} -gt 0 ]; then
  set -x
fi

SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
GEOARROW_DIR="$(cd "${SOURCE_DIR}/../.." && pwd)"

show_header() {
  if [ -z "$GITHUB_ACTIONS" ]; then
    echo ""
    printf '=%.0s' $(seq ${#1}); printf '\n'
    echo "${1}"
    printf '=%.0s' $(seq ${#1}); printf '\n'
  else
    echo "::group::${1}"; printf '\n'
  fi
}

case $# in
  0) TARGET_GEOARROW_DIR="${GEOARROW_DIR}"
     ;;
  1) TARGET_GEOARROW_DIR="$1"
     ;;
  *) echo "Usage:"
     echo "  Build documentation based on a source checkout elsewhere:"
     echo "    $0 path/to/geoarrow-c"
     echo "  Build documentation for this geoarrow checkout:"
     echo "    $0"
     exit 1
     ;;
esac

main() {
   pushd "${TARGET_GEOARROW_DIR}"

   # Clean the previous build
   rm -rf docs/_build
   mkdir -p docs/_build

   # Run doxygen
   show_header "Run Doxygen for C library"
   pushd src/apidoc
   doxygen
   popd

   # Build + install Python bindings
   # pip install . doesn't quite work with the setuptools available on the
   # ubuntu docker image...python -m build works I think because it sets up
   # a virtualenv
   pushd python/geoarrow-c
   rm -rf dist
   python3 -m build --wheel
   pip3 install dist/geoarrow*.whl
   popd

   pushd docs

   show_header "Build Sphinx project"

   # Use the README as the docs homepage
   pandoc ../README.md --from markdown --to rst -s -o source/README_generated.rst

   # Build sphinx project
   sphinx-build source _build/html

   popd

   popd
}

main
