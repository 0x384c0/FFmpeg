### Download builds
* [Win 64](https://github.com/0x384c0/FFmpeg/releases/download/4.1.3/ffpatched_win64.zip)

### Build manually
Clone
* `git clone https://github.com/0x384c0/FFmpeg --depth 1`

Build for Apple Silicon
* install sdl2 (for osx `brew install sdl2@2.0.9`)
* install ffmpeg (for osx `brew install ffmpeg@4.1.3`)
* `cd fftools/`
* `make config`
* `make build`

Build for win64
* install [x86_64-w64-mingw32](https://mingw-w64.org/doku.php/download/mingw-builds) (or for osx `brew install mingw-w64@6.0.0`)
* extract downloaded libraries in same directory with FFmpeg
* `cd fftools/`
* `make win_64_check_dependencies`
* `make win_64_config`
* `make win_64_build`

### Updating ffmpeg
* pull master from https://github.com/FFmpeg/FFmpeg
* merge with lates stable release
* update versions for Github Actions and MakeFile

### Hotkeys
* b - toggle bitrate bar and OSD
* n - toggle audio compressor
* h - toggle video normalizer

### Screenshots
![bitrate_bar](screenshots/screenshot_bitrate_bar.jpg?raw=true "bitrate_bar")
![video_normalizer](screenshots/screenshot_video_normalizer.jpg?raw=true "video_normalizer")
