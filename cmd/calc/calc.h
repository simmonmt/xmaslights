#ifndef _CMD_CALC_CALC_H_
#define _CMD_CALC_CALC_H_ 1

#include <iostream>

#include "lib/geometry/points.h"

double Degrees(double rad);
double Radians(double deg);

// Find the XY locations of cameras 1 and 2 given distance D to the
// central point (the origin).
XYPos FindC1XYPos(double D);
XYPos FindC2XYPos(double D);

struct Metadata {
  double fov_h;  // radians
  double fov_v;  // radians
  int res_h;
  int res_v;
};

// Find the detection angle, in radians.
double FindAngleRad(int pixel, int res, double fov);

// Find the distance between two points.
double FindDist(const XYPos& p1, const XYPos& p2);

// Find the detection's z coordinate given the location of the camera,
// xy coordinates of the detection, and the detection's z angle.
double FindDetectionZ(double z_angle, const XYPos& camera,
                      const XYPos& detection);

// Find b in y=mx+b for a given slope and known position.
double FindLineB(double slope, const XYPos& pos);

struct Line {
  double slope;
  double b;
};

std::ostream& operator<<(std::ostream& os, const Line& pos);

// Find where two lines intersect.
XYPos FindXYIntersection(const Line& l1, const Line& l2);

struct Result {
  XYZPos detection;
  double pixel_y_error;  // offset of c2.y from c1.y
};

Result FindDetectionLocation(double D, const XYPos& c1_pixel,
                             const XYPos& c2_pixel, const Metadata& metadata);

#endif  // _CMD_CALC_CALC_H_
