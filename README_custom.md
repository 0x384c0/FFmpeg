Clone
* git clone https://github.com/0x384c0/FFmpeg --depth 1

Build
* install sdl2
* install ffmpeg 3.4
* cd fftools/
* make config 
* make build test

Usage
* b - toggle bitrate bar
* n - toggle audio compressor
* h - toggle video normalizer

![Alt text](screenshots/screenshot_video_normalizer.jpg?raw=true "video_normalizer")
![Alt text](screenshots/screenshot.jpg?raw=true "screenshot")


### REQUIRED FILES:
Makefile
README.md
cmdutils.c
cmdutils.h
cmdutils_common_opts.h
cmdutils_opencl.c
compat
configure
ffplay.c
libavutil
patch
screenshots