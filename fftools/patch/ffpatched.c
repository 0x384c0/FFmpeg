#include "libavutil/frame.h"
#include "libavformat/avformat.h"

#include "AudioCompress/compress.h"
#include "osd/osd.h"
#include "video_normalizer/normalize.h"
#include "bitrate_bar/bitrate_bar.c" //TODO: create and use headers

// #define LOG_VIDEO_FRAME_CONTENT
// #define LOG_VIDEO_FRAME
// #define LOG_AUDIO_FRAME

#define BITBAR_HEIGHT 30
#define OSD_INSET BITBAR_HEIGHT / 2 + 1
#define COPIED_FRAMES_BUF_SIZE 5

static int
IS_VIDEO_NORMALIZER_ENABLED = 1,
IS_AUDIO_COMPRESS_ENABLED = 1,
IS_BITRATE_BAR_ENABLED = 1;

//private
static int
DURATION_IS_KNOWN = 0;

static void drawInAvFrameYUV(AVFrame *avFrame,int x, int y, int w, int h){
	// printf("format %s\n",av_get_pix_fmt_name(avFrame->format ));
	int channelNumber = 0;
	int linesize = avFrame->linesize[channelNumber];
	avFrame->data[channelNumber][0] = 0;
	printf("\n\n");
	for (int height = y; height < y + h; ++height){
		for (int width = x; width < x + w; ++width){	
			printf("%3d ", avFrame->data[channelNumber][linesize * height + width]);
			avFrame->data[0][avFrame->linesize[0] * height + width] = 255;
		}
		printf("\n");
	}
	printf("\033[%dA",h+2); fflush(stdout);
}

static void logVideoFrame(Frame *frame){
	AVFrame *avFrame = frame->frame;
	printf("\n");
	printf("format %d: width %d : height %d ",
		frame->format,
		frame->width,
		frame->height );

	for (int channelNumber = 0; channelNumber < 3; ++channelNumber){
		av_log(NULL, AV_LOG_WARNING,"\nlinesize %4d: width %d : height %d : nb_samples %d : format %d : key_frame %d : pts %d :	",
			avFrame->linesize[channelNumber], 
			avFrame->width, 
			avFrame->height, 
			avFrame->nb_samples, 
			avFrame->format, 
			avFrame->key_frame, 
			avFrame->pts );
	}
	fflush(stdout);
}
static void logAudioFrame(Uint8 *stream, int len){
	int 
	NUMBER_OF_ROWS = 40;
	av_log(NULL, AV_LOG_WARNING,"\n ffpatched_processAudioFrame: %d %d\n\n\n", stream, len );
	int
	row = 0,
	len_i = len;
	while (len_i > 0) {
		printf("%3d ",stream[len_i]);
		len_i--;
		row++;
		if (row > NUMBER_OF_ROWS) {
			row = 0;
			printf("\n");
		}
	}
	len_i = len;
}

//frame backup
intptr_t copiedFrames[COPIED_FRAMES_BUF_SIZE] = { 0 };
int currentFrameId = 0;
static void copyFrameData(AVFrame *avFrame){

	int dataSize = avFrame->linesize[NORMALIZE_CHANNEL_ID] * avFrame->height;

	if (dataSize == 0)
		return;

	if (copiedFrames[currentFrameId] == 0)
		copiedFrames[currentFrameId] = malloc(dataSize);


	memcpy(copiedFrames[currentFrameId],avFrame->data[NORMALIZE_CHANNEL_ID],dataSize);
	avFrame->data[NORMALIZE_CHANNEL_ID] = copiedFrames[currentFrameId];

	currentFrameId ++;
	if (currentFrameId >= COPIED_FRAMES_BUF_SIZE)
		currentFrameId = 0;
}

//utils
struct Compressor *compressor;
struct Osd *osd;
struct Normalizer *normalizer;

//public
static void ffpatched_init(){
	av_log(NULL, AV_LOG_WARNING,"Usage: n - audio compressor, h - video normalizer, b - bitrate bar\n");
	compressor = Compressor_new(0);
	normalizer = Normalizer_new();
}
static int ffpatched_handleRead(AVStream *st,AVFormatContext *ic,AVPacket *pkt,int st_index[]){
	DURATION_IS_KNOWN = st->duration > 0.1 || st->nb_index_entries > 0;
	if (DURATION_IS_KNOWN){
		osd = OSD_new(ic,BITBAR_HEIGHT + OSD_INSET,OSD_INSET);
		int ret = BitRateBar_generate(st,ic,pkt,st_index,BITBAR_HEIGHT);
		return ret;
	} else {
		return 0;
	}
}

static void ffpatched_processVideoFrame(Frame *frame,VideoState *video_state){
	if (IS_VIDEO_NORMALIZER_ENABLED || (IS_BITRATE_BAR_ENABLED && DURATION_IS_KNOWN))
		copyFrameData(frame->frame);
	#ifdef LOG_VIDEO_FRAME_CONTENT
		drawInAvFrameYUV(frame->frame,10,10,35,35);
	#endif
	#ifdef LOG_VIDEO_FRAME
		logVideoFrame(frame);
	#endif
	if (IS_VIDEO_NORMALIZER_ENABLED)
		Normalizer_processFrame(normalizer,frame->frame);
	if (IS_BITRATE_BAR_ENABLED && DURATION_IS_KNOWN){
		BitRateBar_insert(frame,video_state);
		OSD_processFrame(osd, video_state->audio_clock, frame->frame);
	}
}
static void ffpatched_restoreLastVideoFrame(AVFrame *frame){
	restoreFrame(frame);
}


static void ffpatched_processAudioFrame(VideoState *is, int len){
	uint8_t *stream = (uint8_t *)is->audio_buf + is->audio_buf_index;

	#ifdef LOG_AUDIO_FRAME
		logAudioFrame(stream,len);
	#endif

	// Compressor_reset();
	if (!is->paused && !is->muted && is->audio_buf && IS_AUDIO_COMPRESS_ENABLED)
		Compressor_Process_int16(compressor, stream, len/2);
}
static void ffpatched_handleSDLKeyEvent(Uint8 sdlKey){
	switch (sdlKey) {
		case SDLK_n:
			IS_AUDIO_COMPRESS_ENABLED = !IS_AUDIO_COMPRESS_ENABLED;
			av_log(NULL, AV_LOG_WARNING,"\nIS_AUDIO_COMPRESS_ENABLED = %d\n\n",IS_AUDIO_COMPRESS_ENABLED);	
			break;

		case SDLK_b:
			if (DURATION_IS_KNOWN){
				IS_BITRATE_BAR_ENABLED = !IS_BITRATE_BAR_ENABLED;
				av_log(NULL, AV_LOG_WARNING,"\nIS_BITRATE_BAR_ENABLED = %d\n\n",IS_BITRATE_BAR_ENABLED);
			} else {
				av_log(NULL, AV_LOG_WARNING,"\nBITRATE BAR NOT AVAILABLE\n\n");
			}
			break;

		case SDLK_h:
			IS_VIDEO_NORMALIZER_ENABLED = !IS_VIDEO_NORMALIZER_ENABLED;
			av_log(NULL, AV_LOG_WARNING,"\nIS_VIDEO_NORMALIZER_ENABLED = %d\n\n",IS_VIDEO_NORMALIZER_ENABLED);	
			break;

		default:
			break;
	}
}
static void ffpatched_handleExit(){
	IS_VIDEO_NORMALIZER_ENABLED = 0;
	IS_AUDIO_COMPRESS_ENABLED = 0;
	IS_BITRATE_BAR_ENABLED = 0;
	Compressor_delete(compressor);
	Normalizer_delete(normalizer);
	OSD_delete(osd);
}