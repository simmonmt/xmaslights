# xmaslights

This repository contains a suite of programs used to locate LED lights in 3D,
edit those locations, and ultimately export them as models for use in xLights.

The initial idea came from https://github.com/aaknitt/pixel_mapper, who
implemented a set of Python programs to do this sort of calculation with three
cameras.

Starting from that base, I implemented a 2-camera version. Eventually I'll need
three, but for now the bushes I cover with lights are only visible from two
sides. Largely in C++, uses OpenCV for most things, and DDP for talking to the
LED controllers. Tested with an ESPixelStick.

Includes an interactive editor (cmd/showfound) whose UI is implemented with
OpenCV (don't ever do this). The editor is used to correct bad detections, add
missing ones, and ultimately generate xLights models.

The goal is to have one command for saving images and one command
(cmd/showfound) for detection, visualization, and fixup. The current state is
a bit of an evolution towards that goal.
