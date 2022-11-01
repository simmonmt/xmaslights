#ifndef _LIB_GEOMETRY_TRANSLATION_H_
#define _LIB_GEOMETRY_TRANSLATION_H_ 1

#include <math.h>

inline double Degrees(double rad) { return (rad / M_PI) * 180.0; }

inline double Radians(double deg) { return (deg / 180.0) * M_PI; }

#endif  // _LIB_GEOMETRY_TRANSLATION_H_
