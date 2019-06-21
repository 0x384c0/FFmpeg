#include "libavutil/frame.h"

struct Osd;

struct Osd *OSD_new(struct AVFormatContext *ic, int topInset, int rightInset);
void OSD_delete(struct Osd *osd);
void OSD_processFrame(struct Osd *osd, double audio_clock, AVFrame *frame);