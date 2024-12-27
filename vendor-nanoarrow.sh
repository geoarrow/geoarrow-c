
main() {
  local -r repo_url="https://github.com/apache/arrow-nanoarrow"
  # Check releases page: https://github.com/apache/arrow-nanoarrow/releases/
  local -r commit_sha=33d2c8b973d8f8f424e02ac92ddeaace2a92f8dd

  echo "Fetching $commit_sha from $repo_url"
  SCRATCH=$(mktemp -d)
  trap 'rm -rf "$SCRATCH"' EXIT

  local -r tarball="$SCRATCH/nanoarrow.tar.gz"
  wget -O "$tarball" "$repo_url/archive/$commit_sha.tar.gz"
  tar --strip-components 1 -C "$SCRATCH" -xf "$tarball"

  # Remove previous bundle
  rm -rf src/vendor/nanoarrow

  # Build the bundle
  python "${SCRATCH}/ci/scripts/bundle.py" \
      --include-output-dir=src/vendor \
      --source-output-dir=src/vendor/nanoarrow
}

main
