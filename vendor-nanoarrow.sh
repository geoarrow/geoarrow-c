
for f in nanoarrow.h nanoarrow.c; do
  curl -L \
    https://raw.githubusercontent.com/paleolimbot/arrow-nanoarrow/unknown-flags/dist/$f \
    -o src/geoarrow/$f
done

sed -i.bak \
  -e 's|// #define NANOARROW_NAMESPACE YourNamespaceHere|#include "geoarrow_config.h"|' \
  src/geoarrow/nanoarrow.h
rm src/geoarrow/nanoarrow.h.bak
