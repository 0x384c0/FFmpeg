#include <stdio.h>
#include <math.h>
#include "libavutil/frame.h"

// #define DEBUG_REGIONS

#define CHANNEL_ID 0
#define REGIONS_W 16
#define REGIONS_H 9

#define MAX_LUMA 255 // int8 value
#define MAX_LUMA_MODIFYER 255 * 0.3 // int8 value
#define AVEARGE_LUMA_MIN 255 * 0.3 // int8 value, luma adjust treshold
#define LUMA_HISTORY 30.0 // frames

struct FrameRegionLumaInfo {
	int
	prevFrameAverageLuma,
	luma_modifyer; //from 0 to MAX_LUMA_MODIFYER
};

struct FrameRegionLumaInfo frameLumaInfo[REGIONS_W][REGIONS_H] = { MAX_LUMA, 0 };

static int clamp(int x,int minVal,int maxVal){
	if (x < minVal)
		return minVal;
	if (x > maxVal)
		return maxVal;
	return x;
}

//smooth luma adjusting
static void change_luma_modifyer(struct FrameRegionLumaInfo *frameRegionLumaInfo){
	int new_luma_modifyer = 0;
	if (frameRegionLumaInfo->prevFrameAverageLuma < AVEARGE_LUMA_MIN)
		new_luma_modifyer = frameRegionLumaInfo->luma_modifyer + MAX_LUMA_MODIFYER / LUMA_HISTORY;
	else if (frameRegionLumaInfo->luma_modifyer != 0)
		new_luma_modifyer = frameRegionLumaInfo->luma_modifyer - MAX_LUMA_MODIFYER / LUMA_HISTORY;
	frameRegionLumaInfo->luma_modifyer = clamp(new_luma_modifyer, 0, MAX_LUMA_MODIFYER);
}

//adjust luma (0-255) for single pixel
static int get_new_luma(struct FrameRegionLumaInfo *frameRegionLumaInfo, int current_luma){
	int needModifyLuma = frameRegionLumaInfo->luma_modifyer != 0;

	if (needModifyLuma){
		int new_luma = current_luma - current_luma * frameRegionLumaInfo->luma_modifyer/MAX_LUMA + frameRegionLumaInfo->luma_modifyer;
		return new_luma;
	} else {
		return current_luma;
	}
}

//public
static void Normalizer_processFrame(Frame *vp){ // TODO: scene detector
	AVFrame avFrame = *vp->frame;

	int
	regionW = round((float) avFrame.width / REGIONS_W),
	regionH = round((float) avFrame.height / REGIONS_H);

	for (int w = 0; w < REGIONS_W; w++){
		for (int h = 0; h < REGIONS_H; h++){
			change_luma_modifyer(&frameLumaInfo[w][h]);

			int 
			totalLuma = 0,
			offsetW = regionW * w,
			offsetH = regionH * h;

			for (int frameW = 0; frameW < regionW; frameW++){
				for (int frameH = 0; frameH < regionH; frameH++){
					int pixel = (offsetW + frameW) + ((offsetH + frameH) * avFrame.width) ;
					#ifdef DEBUG_REGIONS
						if (w % 2 || h % 2)
							avFrame.data[CHANNEL_ID][pixel] = 0;
						else
							avFrame.data[CHANNEL_ID][pixel] = 255;
					#else
						totalLuma += avFrame.data[CHANNEL_ID][pixel];
						avFrame.data[CHANNEL_ID][pixel] = get_new_luma(&frameLumaInfo[w][h], avFrame.data[CHANNEL_ID][pixel]);
					#endif
				}
			}
			frameLumaInfo[w][h].prevFrameAverageLuma = totalLuma/(regionW * regionH);
		}
	}
	return;
}

