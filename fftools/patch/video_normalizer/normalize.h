#include "libavutil/frame.h"

#define NORMALIZE_CHANNEL_ID 0

struct Normalizer;

struct Normalizer *Normalizer_new();
void Normalizer_delete(struct Normalizer *normalizer);
void Normalizer_processFrame(struct Normalizer *normalizer, AVFrame *avFrameP);