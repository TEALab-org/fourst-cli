
#ifndef FOURST_HH
#define FOURST_HH

#ifdef __EMSCRIPTEN__
 #include <emscripten.h>
 #define FOURST_API EMSCRIPTEN_KEEPALIVE extern "C"
#else
 #define FOURST_API extern "C"
#endif

// C/C++ libraries
#include <cstdio>

// Use C++ standard
using namespace std;

namespace fourst {
// CUSTOM TYPES
typedef int _int;

/* External C-API interface */

// Initialize the FOURST library
FOURST_API void fourst_init();

FOURST_API int fourst_gencode(double* in);

}


#endif /* FOURST_HH */
