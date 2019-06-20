Clone
* git clone https://github.com/0x384c0/FFmpeg --depth 1

Build
* install sdl2
* install ffmpeg <version>
* cd fftools/
* make config 
* make build test

Build win64
* install x86_64-w64-mingw32
* download ffmpeg-<version>-win64-dev ffmpeg-<version>-win64-shared SDL2-<version>
* cd fftools/
* make win_64_check_dependencies
* make win_64_config
* make win_64_build

Usage
* b - toggle bitrate bar
* n - toggle audio compressor
* h - toggle video normalizer

![Alt text](screenshots/screenshot_video_normalizer.jpg?raw=true "video_normalizer")
![Alt text](screenshots/screenshot.jpg?raw=true "bitrate_bar")


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