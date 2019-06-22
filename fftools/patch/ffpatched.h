#include <SDL.h>
#include "libavutil/frame.h"
#include "libavformat/avformat.h"



void FFpatched_init();
void FFpatched_processVideoFrame(AVFrame *avFrame, double master_clock, double audio_clock, int64_t ic_duration);
void FFpatched_processAudioFrame(int paused,int muted, uint8_t audio_buf, int audio_buf_index, int len);
void FFpatched_handleSDLKeyEvent(Uint8 sdlKey);
void FFpatched_deinit();