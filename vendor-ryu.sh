
curl -L https://github.com/ulfjack/ryu/archive/refs/tags/v2.0.tar.gz |
    tar -xzf -

rm -rf src/geoarrow/ryu
mkdir src/geoarrow/ryu

cp ryu-2.0/ryu/d2fixed.c \
    ryu-2.0/ryu/ryu2.h \
    ryu-2.0/ryu/common.h \
    ryu-2.0/ryu/digit_table.h \
    ryu-2.0/ryu/d2fixed_full_table.h \
    ryu-2.0/ryu/d2s_intrinsics.h \
    ryu-2.0/LICENSE-Apache2 \
    src/geoarrow/ryu


rm -rf ryu-2.0
