CC=clang
C_FILES=ffplay.c cmdutils.c patch/*.c patch/AudioCompress/*.c patch/Osd/*.c patch/video_normalizer/*.c patch/bitrate_bar/*.c
OUT_FILE=ffpatched
CONFIG_PARAMS=--enable-gpl --disable-ffmpeg --disable-ffprobe --disable-avdevice --disable-avcodec --disable-avformat --disable-swresample --disable-swscale --disable-postproc --disable-avfilter # --enable-libx264
FFMPEG_LIBS=-lavformat -lavcodec -lavfilter -lavdevice -lavutil -lswresample -lswscale

.ONESHELL:

#unix  compile scripts
all:
	$(MAKE) clean
	$(MAKE) config
	$(MAKE) build
	$(MAKE) test

config:
	cd ../ && ./configure --cc=$(CC) $(CONFIG_PARAMS)

build:
	$(CC) -w -O3 -o $(OUT_FILE) $(C_FILES) -I. -I.. -I/usr/local/include/ -L/usr/local/lib/ $(FFMPEG_LIBS) -lz -lbz2 -lSDL2  -lmp3lame -lx264 `sdl2-config --cflags --libs`

build_and_debug:
	$(CC) -g -w -o $(OUT_FILE) $(C_FILES) -I. -I.. -I/usr/local/include/ -L/usr/local/lib/ $(FFMPEG_LIBS) -lz -lbz2 -lSDL2  -lmp3lame -lx264 `sdl2-config --cflags --libs` && \
	lldb -o run ./$(OUT_FILE) ~/Movies/test.mp4
	lldb ./$(OUT_FILE)

test:
	./$(OUT_FILE) ~/Movies/test.mp4


clean:
	git clean -fdx

# colors
NC=\x1b[0m
RED=\x1b[31;01m

# win compile scripts
MIN_GW=x86_64-w64-mingw32
CROSS_PREFIX=$(MIN_GW)-
WIN_64_CC=$(CROSS_PREFIX)gcc
WIN_64_FFMPEG_DIR=../../ffmpeg-4.1.3-win64-dev
WIN_64_FFMPEG_SHARED_DIR=../../ffmpeg-4.1.3-win64-shared
WIN_64_SDL_DIR=../../SDL2-2.0.9/$(MIN_GW)
WIN_64_OUT_DIR=$(OUT_FILE)_win64

win_64_all:
	$(MAKE) clean
	$(MAKE) win_64_check_dependencies
	$(MAKE) win_64_config
	$(MAKE) win_64_build
	$(MAKE) win_64_test

win_64_check_dependencies:
	printf "\nChecking dependencies...\n" && \
	if [ ! -x "$$(command -v $(WIN_64_CC))" ]; then echo "$(WIN_64_CC) is reqiured, but it's not installed. Aborting. Dowload and install $(MIN_GW) from https://mingw-w64.org/doku.php/download/mingw-builds and try again."; exit 1; fi && \
	if [ ! -d $(WIN_64_FFMPEG_DIR) ]; then echo "$(RED)fmpeg libs is missing in $(WIN_64_FFMPEG_DIR) . Dowload it from https://ffmpeg.zeranoe.com/builds/win64/dev/ffmpeg-4.1.3-win64-dev.zip$(NC)"; exit 1; fi && \
	if [ ! -d $(WIN_64_FFMPEG_SHARED_DIR) ]; then echo "$(RED)ffmpeg dlls is missing in $(WIN_64_FFMPEG_SHARED_DIR) . Dowload it from https://ffmpeg.zeranoe.com/builds/win64/shared/ffmpeg-4.1.3-win64-shared.zip$(NC)"; exit 1; fi && \
	if [ ! -d $(WIN_64_SDL_DIR) ]; then echo "$(RED)SDL libs is missing in $(WIN_64_SDL_DIR) . Dowload it from https://www.libsdl.org/release/SDL2-devel-2.0.9-mingw.tar.gz$(NC)"; exit 1; fi && \
	printf "Dependencies are OK\n"

win_64_config:
	cd ../ && ./configure  --arch=x86 --target-os=mingw64 --cross-prefix=$(CROSS_PREFIX) --pkg-config=pkg-config $(CONFIG_PARAMS)

win_64_build:
	$(CROSS_PREFIX)objdump  -p $(OUT_FILE).exe | grep 'DLL Name:' | sed -e "s/\t*DLL Name: //g"
	set -e
	$(WIN_64_CC) -w -O3 -o $(OUT_FILE) $(C_FILES) -I. -I../ -I$(WIN_64_SDL_DIR)/include/SDL2 -L$(WIN_64_SDL_DIR)/lib  -I$(WIN_64_FFMPEG_DIR)/include -L$(WIN_64_FFMPEG_DIR)/lib -I$(WIN_64_MINGW_DIR)/include -L$(WIN_64_MINGW_DIR)/lib      $(FFMPEG_LIBS) -lmingw32 -lSDL2main -lSDL2
	mkdir -p $(WIN_64_OUT_DIR)
	mv -f $(OUT_FILE).exe $(WIN_64_OUT_DIR)
	find $(WIN_64_FFMPEG_SHARED_DIR) -name "*.dll" -exec cp -n {} $(WIN_64_OUT_DIR) \;
	find $(WIN_64_SDL_DIR) -name "*.dll" -exec cp -n {} $(WIN_64_OUT_DIR) \;


win_64_test:
	$(WIN_64_OUT_DIR)/$(OUT_FILE) -i /d/Videos/test.avi