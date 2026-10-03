### Download builds
* [Releases](https://github.com/0x384c0/FFmpeg/releases) (Win 64, Apple Silicon)

### Build manually
Clone
* `git clone https://github.com/0x384c0/FFmpeg --depth 1`

Build for Apple Silicon
* install sdl2 (for osx `brew install sdl2`)
* install ffmpeg built from master (for osx `brew install --HEAD ffmpeg`), library versions must match this source tree
* `cd fftools/`
* `make config`
* `make build`

Build for win64
* install [x86_64-w64-mingw32](https://www.mingw-w64.org/downloads/) (for ubuntu `apt install mingw-w64 nasm`, for osx `brew install mingw-w64`)
* `cd fftools/`
* `make libavutil/ffversion.h`
* `make win_64_get_dependencies` (downloads FFmpeg master shared build from BtbN/FFmpeg-Builds and SDL2)
* `make win_64_config`
* `make win_64_build`

### Updating ffmpeg
* rebase this branch on master from https://github.com/FFmpeg/FFmpeg
* update versions for Github Actions and MakeFile

### Hotkeys
* b - toggle bitrate bar and OSD
* n - toggle audio compressor
* h - toggle video normalizer

### Screenshots
![bitrate_bar](screenshots/screenshot_bitrate_bar.jpg?raw=true "bitrate_bar")
![video_normalizer](screenshots/screenshot_video_normalizer.jpg?raw=true "video_normalizer")
