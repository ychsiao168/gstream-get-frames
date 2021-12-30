## Description
A library using gstreamer to get deocded frames.
see main.c for example.

## How to use this library
* put libgetframe.c and libgetframe.h into your project.
* in Makefile, append these:
    * cflags: $(shell pkg-config --cflags gstreamer-1.0)
    * libs: $(shell pkg-config --libs gstreamer-1.0 gstreamer-app-1.0)

## Notes
* this library can be run on jetson or PC:
    * set HAS_NV_DECODER == 1 in libggetframe.c for jetson board. (Default)
    * set HAS_NV_DECODER == 0 in libggetframe.c for general platform.