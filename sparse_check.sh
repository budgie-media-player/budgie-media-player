#!/bin/bash
# Provided only for myself, so that I can keep an eye on code
sparse -I. `pkg-config --cflags gtk+-3.0` `pkg-config --cflags gstreamer-video-1.0` src/*.c src/db/*.c

