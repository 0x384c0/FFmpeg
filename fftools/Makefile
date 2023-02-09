SRC_PATH=../
CC=clang
ARCH=arm64
C_FILES=ffplay.c cmdutils.c patch/*.c patch/AudioCompress/*.c patch/Osd/*.c patch/video_normalizer/*.c patch/bitrate_bar/*.c
OUT_FILE=ffpatched # TODO: use subfolder
CONFIG_PARAMS=--enable-gpl --disable-ffmpeg --disable-ffprobe --disable-avdevice --disable-avcodec --disable-avformat --disable-swresample --disable-swscale --disable-postproc --disable-avfilter --arch=${ARCH} # --enable-libx264
FFMPEG_LIBS=-lavformat -lavcodec -lavfilter -lavdevice -lavutil -lswresample -lswscale
TEST_STREAM="http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8"

.ONESHELL:

#unix  compile scripts
all:
	$(MAKE) libavutil/ffversion.h
	$(MAKE) clean
	$(MAKE) config
	$(MAKE) build
	$(MAKE) test

# create version, copied from ../MakeFile
VERSION_SH  = $(SRC_PATH)/ffbuild/version.sh
GIT_LOG     = $(SRC_PATH)/.git/logs/HEAD

.version: $(wildcard $(GIT_LOG)) $(VERSION_SH) $(SRC_PATH)ffbuild/config.mak
.version: M=@

libavutil/ffversion.h .version:
	$(M)$(VERSION_SH) $(SRC_PATH) $(SRC_PATH)libavutil/ffversion.h $(EXTRA_VERSION)
	$(Q)touch .version

# make
config:
	cd $(SRC_PATH) && ./configure --cc=$(CC) $(CONFIG_PARAMS)

build:
	$(CC) -w -O3 -arch=${ARCH} -o $(OUT_FILE) $(C_FILES) -I. -I.. -I/usr/local/include/ -L/usr/local/lib/ $(FFMPEG_LIBS) -lz -lbz2 -lSDL2  -lmp3lame -lx264 `sdl2-config --cflags --libs`

build_and_debug:
	$(CC) -g -w  -arch=${ARCH} -o $(OUT_FILE) $(C_FILES) -I. -I.. -I/usr/local/include/ -L/usr/local/lib/ $(FFMPEG_LIBS) -lz -lbz2 -lSDL2  -lmp3lame -lx264 `sdl2-config --cflags --libs` && \
	lldb -o run ./$(OUT_FILE) $(TEST_STREAM)
	lldb ./$(OUT_FILE)

test:
	./$(OUT_FILE) $(TEST_STREAM)


clean:
	git clean -fdx

# colors
NC=\x1b[0m
RED=\x1b[31;01m

# make windows
FFMPEG_VERSION=5.1.2
SDL_VERSION=2.26.3

MIN_GW=x86_64-w64-mingw32
CROSS_PREFIX=$(MIN_GW)-
WIN_64_CC=$(CROSS_PREFIX)gcc

# WIN_64_FFMPEG_DIR=$(SRC_PATH)../ffmpeg-4.1.3-win64-dev

WIN_64_LIBS_BASE=win_64_libs
WIN_64_LIBS_TMP=$(WIN_64_LIBS_BASE)/tmp
WIN_64_FFMPEG_SHARED_URL="https://github.com/GyanD/codexffmpeg/releases/download/$(FFMPEG_VERSION)/ffmpeg-$(FFMPEG_VERSION)-full_build-shared.zip"
WIN_64_FFMPEG_SHARED_DIR=$(WIN_64_LIBS_BASE)/ffmpeg-$(FFMPEG_VERSION)-full_build-shared
WIN_64_SDL_SHARED_URL="https://www.libsdl.org/release/SDL2-devel-$(SDL_VERSION)-mingw.zip"
WIN_64_SDL_SHARED_DIR=$(WIN_64_LIBS_BASE)/SDL2-$(SDL_VERSION)


WIN_64_OUT_DIR=$(OUT_FILE)_win64

win_64_all:
	$(MAKE) clean
	$(MAKE) win_64_check_dependencies
	$(MAKE) win_64_config
	$(MAKE) win_64_build
	$(MAKE) win_64_test

win_64_get_dependencies:
	rm -rf $(WIN_64_LIBS_TMP)
	mkdir -p $(WIN_64_LIBS_BASE)
	mkdir -p $(WIN_64_LIBS_TMP)
	printf "\nChecking dependencies...\n"
	if [ ! -x "$$(command -v $(WIN_64_CC))" ];	then echo "$(WIN_64_CC) is reqiured, but it's not installed. Aborting. Dowload and install $(MIN_GW) from https://mingw-w64.org/doku.php/download/mingw-builds and try again."; exit 1; fi && \
	if [ ! -d $(WIN_64_FFMPEG_SHARED_DIR) ];	then wget $(WIN_64_FFMPEG_SHARED_URL) -O $(WIN_64_LIBS_TMP)/ffmpeg.zip;	unzip $(WIN_64_LIBS_TMP)/ffmpeg.zip -d $(WIN_64_LIBS_BASE);	fi && \
	if [ ! -d $(WIN_64_SDL_SHARED_DIR) ];		then wget $(WIN_64_SDL_SHARED_URL)    -O $(WIN_64_LIBS_TMP)/sdl.zip;	unzip $(WIN_64_LIBS_TMP)/sdl.zip -d $(WIN_64_LIBS_BASE);		fi && \
	printf "Dependencies are OK\n"

win_64_config:
	cd ../ && ./configure  --arch=x86 --target-os=mingw64 --cross-prefix=$(CROSS_PREFIX) --pkg-config=pkg-config $(CONFIG_PARAMS)

win_64_build:
	$(CROSS_PREFIX)objdump  -p $(OUT_FILE).exe | grep 'DLL Name:' | sed -e "s/\t*DLL Name: //g"
	set -e
	# $(WIN_64_CC) -w -O3 -o $(OUT_FILE) $(C_FILES) -I. -I$(SRC_PATH) -I $(WIN_64_SDL_SHARED_DIR)/$(MIN_GW)/include/SDL2 -L $(WIN_64_SDL_SHARED_DIR)/$(MIN_GW)/lib  -I $(WIN_64_FFMPEG_DIR)/include -L $(WIN_64_FFMPEG_DIR)/lib -I $(WIN_64_MINGW_DIR)/include -L $(WIN_64_MINGW_DIR)/lib      $(FFMPEG_LIBS) -lmingw32 -lSDL2main -lSDL2
	$(WIN_64_CC) -w -O3 -o $(OUT_FILE) $(C_FILES) -I. -I$(SRC_PATH) -I $(WIN_64_SDL_SHARED_DIR)/$(MIN_GW)/include/SDL2 -L $(WIN_64_SDL_SHARED_DIR)/$(MIN_GW)/lib  -I $(WIN_64_FFMPEG_SHARED_DIR)/include -L $(WIN_64_FFMPEG_SHARED_DIR)/lib -I $(WIN_64_MINGW_DIR)/include -L $(WIN_64_MINGW_DIR)/lib      $(FFMPEG_LIBS) -lmingw32 -lSDL2main -lSDL2
	mkdir -p $(WIN_64_OUT_DIR)
	mv -f $(OUT_FILE).exe $(WIN_64_OUT_DIR)
	find $(WIN_64_FFMPEG_SHARED_DIR) -name "*.dll" -exec cp -n {} $(WIN_64_OUT_DIR) \;
	find $(WIN_64_SDL_SHARED_DIR) -name "*.dll" -exec cp -n {} $(WIN_64_OUT_DIR) \;


win_64_test:
	$(WIN_64_OUT_DIR)/$(OUT_FILE) $(TEST_STREAM)