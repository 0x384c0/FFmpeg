#include "ffpatched.h"

#include "osd/osd.h"
#include "AudioCompress/compress.h"
#include "video_normalizer/normalize.h"
#include "bitrate_bar/bitrate_bar.h"

// #define LOG_VIDEO_FRAME_CONTENT
// #define LOG_VIDEO_FRAME
// #define LOG_AUDIO_FRAME

#define BITBAR_HEIGHT 30
#define OSD_INSET BITBAR_HEIGHT / 2 + 1
#define COPIED_FRAMES_BUF_SIZE 5

struct FFpatched
{
    //state
    int
    IS_VIDEO_NORMALIZER_ENABLED,
    IS_AUDIO_COMPRESS_ENABLED,
    IS_BITRATE_BAR_ENABLED,
    DURATION_IS_KNOWN;
    //frame backup
    intptr_t copiedFrames[COPIED_FRAMES_BUF_SIZE];
    int currentFrameId;
    //utils
    struct Compressor *compressor;
    struct Osd *osd;
    struct Normalizer *normalizer;
    struct BitRateBar *bitRateBar;
};

struct FFpatched *ffpatchedInstance;

//private
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

static void logVideoFrame(AVFrame *avFrame){
    printf("\n");
    printf("format %d: width %d : height %d ",
        avFrame->format,
        avFrame->width,
        avFrame->height );

    for (int channelNumber = 0; channelNumber < 3; ++channelNumber){
        av_log(NULL, AV_LOG_WARNING,"\nlinesize %4d: width %d : height %d : nb_samples %d : format %d : key_frame %d : pts %d :    ",
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
    av_log(NULL, AV_LOG_WARNING,"\n FFpatched_processAudioFrame: %d %d\n\n\n", stream, len );
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
static void backupFrameData(AVFrame *avFrame){
    int dataSize = avFrame->linesize[NORMALIZE_CHANNEL_ID] * avFrame->height;

    if (dataSize == 0)
        return;

    if (ffpatchedInstance->copiedFrames[ffpatchedInstance->currentFrameId] == 0)
        ffpatchedInstance->copiedFrames[ffpatchedInstance->currentFrameId] = malloc(dataSize);


    memcpy(ffpatchedInstance->copiedFrames[ffpatchedInstance->currentFrameId],avFrame->data[NORMALIZE_CHANNEL_ID],dataSize);
    avFrame->data[NORMALIZE_CHANNEL_ID] = ffpatchedInstance->copiedFrames[ffpatchedInstance->currentFrameId];

    ffpatchedInstance->currentFrameId ++;
    if (ffpatchedInstance->currentFrameId >= COPIED_FRAMES_BUF_SIZE)
        ffpatchedInstance->currentFrameId = 0;
}
static void freeBackedFrameData(){
    for (int i = 0; i < COPIED_FRAMES_BUF_SIZE; i++){
        intptr_t pointer = ffpatchedInstance->copiedFrames[i];
        if (pointer) free(pointer);
    }
}

//public
void FFpatched_init(){
    ffpatchedInstance = calloc(1,sizeof(struct FFpatched));
    ffpatchedInstance->IS_VIDEO_NORMALIZER_ENABLED = 1;
    ffpatchedInstance->IS_AUDIO_COMPRESS_ENABLED = 1;
    ffpatchedInstance->IS_BITRATE_BAR_ENABLED = 1;
    av_log(NULL, AV_LOG_WARNING,"Usage: n - audio compressor, h - video normalizer, b - bitrate bar\n");
    ffpatchedInstance->compressor = Compressor_new(0);
    ffpatchedInstance->normalizer = Normalizer_new();
}
int FFpatched_handleRead(AVStream *st,AVFormatContext *ic,AVPacket *pkt,int st_index[]){
    ffpatchedInstance->DURATION_IS_KNOWN = st->duration > 0.1 || st->nb_index_entries > 0;
    if (ffpatchedInstance->DURATION_IS_KNOWN){
        ffpatchedInstance->osd = OSD_new(ic,BITBAR_HEIGHT + OSD_INSET,OSD_INSET);
        ffpatchedInstance->bitRateBar = BitRateBar_new(st,ic,pkt,st_index,BITBAR_HEIGHT);
    } else {
        return 0;
    }
}

void FFpatched_processVideoFrame(AVFrame *avFrame, double master_clock, double audio_clock, int64_t ic_duration){
    if (ffpatchedInstance->IS_VIDEO_NORMALIZER_ENABLED || (ffpatchedInstance->IS_BITRATE_BAR_ENABLED && ffpatchedInstance->DURATION_IS_KNOWN))
        backupFrameData(avFrame);
    #ifdef LOG_VIDEO_FRAME_CONTENT
        drawInAvFrameYUV(avFrame,10,10,35,35);
    #endif
    #ifdef LOG_VIDEO_FRAME
        logVideoFrame(frame);
    #endif
    if (ffpatchedInstance->IS_VIDEO_NORMALIZER_ENABLED)
        Normalizer_processFrame(ffpatchedInstance->normalizer,avFrame);
    if (ffpatchedInstance->IS_BITRATE_BAR_ENABLED && ffpatchedInstance->DURATION_IS_KNOWN){
        BitRateBar_processFrame(ffpatchedInstance->bitRateBar,avFrame, master_clock, ic_duration);
        OSD_processFrame(ffpatchedInstance->osd, master_clock, avFrame); //video_state->audio_clock,
    }
}

void FFpatched_processAudioFrame(int paused,int muted, uint8_t audio_buf, int audio_buf_index, int len){
    uint8_t *stream = (uint8_t *)audio_buf + audio_buf_index;

    #ifdef LOG_AUDIO_FRAME
        logAudioFrame(stream,len);
    #endif

    // Compressor_reset();
    if (!paused && !muted && audio_buf && ffpatchedInstance->IS_AUDIO_COMPRESS_ENABLED)
        Compressor_Process_int16(ffpatchedInstance->compressor, stream, len/2);
}
void FFpatched_handleSDLKeyEvent(Uint8 sdlKey){
    switch (sdlKey) {
        case SDLK_n:
            ffpatchedInstance->IS_AUDIO_COMPRESS_ENABLED = !ffpatchedInstance->IS_AUDIO_COMPRESS_ENABLED;
            av_log(NULL, AV_LOG_WARNING,"\nIS_AUDIO_COMPRESS_ENABLED = %d\n\n",ffpatchedInstance->IS_AUDIO_COMPRESS_ENABLED);    
            break;

        case SDLK_b:
            if (ffpatchedInstance->DURATION_IS_KNOWN){
                ffpatchedInstance->IS_BITRATE_BAR_ENABLED = !ffpatchedInstance->IS_BITRATE_BAR_ENABLED;
                av_log(NULL, AV_LOG_WARNING,"\nIS_BITRATE_BAR_ENABLED = %d\n\n",ffpatchedInstance->IS_BITRATE_BAR_ENABLED);
            } else {
                av_log(NULL, AV_LOG_WARNING,"\nBITRATE BAR NOT AVAILABLE\n\n");
            }
            break;

        case SDLK_h:
            ffpatchedInstance->IS_VIDEO_NORMALIZER_ENABLED = !ffpatchedInstance->IS_VIDEO_NORMALIZER_ENABLED;
            av_log(NULL, AV_LOG_WARNING,"\nIS_VIDEO_NORMALIZER_ENABLED = %d\n\n",ffpatchedInstance->IS_VIDEO_NORMALIZER_ENABLED);    
            break;

        default:
            break;
    }
}
void FFpatched_deinit(){
    ffpatchedInstance->IS_VIDEO_NORMALIZER_ENABLED = 0;
    ffpatchedInstance->IS_AUDIO_COMPRESS_ENABLED = 0;
    ffpatchedInstance->IS_BITRATE_BAR_ENABLED = 0;
    Compressor_delete(ffpatchedInstance->compressor);
    Normalizer_delete(ffpatchedInstance->normalizer);
    OSD_delete(ffpatchedInstance->osd);
    BitRateBar_delete(ffpatchedInstance->bitRateBar);
    freeBackedFrameData();
    free(ffpatchedInstance);
}