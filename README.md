### Download builds
* [Win 64](https://github.com/0x384c0/FFmpeg/releases/download/4.1.3/ffpatched_win64.zip)

### Build manually
Clone
* `git clone https://github.com/0x384c0/FFmpeg --depth 1`

Update repo
* git remote add ffmpeg_remote https://github.com/FFmpeg/FFmpeg.git
* git fetch --all
* git rebase 6b6b9e5 # Lates tag n4.3.1 commit from https://github.com/FFmpeg/FFmpeg/tags

Build
* install sdl2 (for osx `brew install sdl2@2.0.9`)
* install ffmpeg (for osx `brew install ffmpeg@4.1.3`)
* `cd fftools/`
* `make config`
* `make build`

Build win64 (deprecated) TODO: use builds from https://github.com/BtbN/FFmpeg-Builds/releases and mingw 8.0.0
* install [x86_64-w64-mingw32](https://mingw-w64.org/doku.php/download/mingw-builds) (or for osx `brew install mingw-w64@6.0.0`)
* download 
    * [ffmpeg-4.1.3-win64-dev](https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-4.1.3-win64-dev.zip)
    * [ffmpeg-4.1.3-win64-shared](https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-4.1.3-win64-shared.zip)
    * [SDL2-devel-2.0.9-mingw](https://www.libsdl.org/release/SDL2-devel-2.0.9-mingw.tar.gz)
* extract downloaded libraries in same directory with FFmpeg
* `cd fftools/`
* `make win_64_check_dependencies`
* `make win_64_config`
* `make win_64_build`

### Hotkeys
* b - toggle bitrate bar and OSD
* n - toggle audio compressor
* h - toggle video normalizer

### Screenshots
![bitrate_bar](screenshots/screenshot_bitrate_bar.jpg?raw=true "bitrate_bar")
![video_normalizer](screenshots/screenshot_video_normalizer.jpg?raw=true "video_normalizer")
