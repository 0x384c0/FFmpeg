#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

struct BitRateBar;

struct BitRateBar *BitRateBar_new(AVStream *av_stream,AVFormatContext *ic,AVPacket *pkt,int st_index[],int bitbarHeight);
void BitRateBar_delete(struct BitRateBar *bitRateBar);
void BitRateBar_processFrame(struct BitRateBar *bitRateBar, AVFrame *frame, double master_clock, int64_t ic_duration);