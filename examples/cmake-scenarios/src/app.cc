
#include <cstdio>
#include <cstring>
#include <iostream>

#include "geoarrow/geoarrow.hpp"

int main(int argc, char* argv[]) {
    // Use something from the geoarrow c runtime
    std::printf("Schema format for int32 is '%s'\n", GeoArrowVersion());

    return EXIT_SUCCESS;
  }
