
for f in nanoarrow.h nanoarrow.c; do
  curl -L \
    https://raw.githubusercontent.com/paleolimbot/arrow-nanoarrow/unknown-flags/dist/$f \
    -o src/geoarrow/$f
done

sed -i.bak \
  -e 's|// #define NANOARROW_NAMESPACE YourNamespaceHere|// When testing we use nanoarrow.h, but geoarrow_config.h will not exist in bundled\
// mode. In the tests we just have to make sure geoarrow.h is always included first.\
#if !defined(GEOARROW_CONFIG_H_INCLUDED)\
#include "geoarrow_config.h"\
#endif|' \
  src/geoarrow/nanoarrow.h
rm src/geoarrow/nanoarrow.h.bak
