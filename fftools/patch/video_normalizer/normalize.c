#include <stdio.h>
#include "libavutil/frame.h"

#define MAX_LUMA 255 // int8 value
#define MAX_LUMA_MODIFYER 255 // int8 value
#define AVEARGE_LUMA_MIN 255 * 0.3 // int8 value, luma adjust treshold
#define LUMA_HISTORY 30.0 // frames

struct FrameRegionLumaInfo {
	int
	prevFrameAverageLuma,
	luma_modifyer; //from 0 to MAX_LUMA_MODIFYER
};

struct FrameRegionLumaInfo frameLumaInfo = { MAX_LUMA, 0 };

static int clamp(int x,int minVal,int maxVal){
	if (x < minVal)
		return minVal;
	if (x > maxVal)
		return maxVal;
	return x;
}

//smooth luma adjusting
static void change_luma_modifyer(struct FrameRegionLumaInfo *frameLumaInfo){
	int new_luma_modifyer = 0;
	if (frameLumaInfo->prevFrameAverageLuma < AVEARGE_LUMA_MIN)
		new_luma_modifyer = frameLumaInfo->luma_modifyer + MAX_LUMA_MODIFYER / LUMA_HISTORY;
	else if (frameLumaInfo->luma_modifyer != 0)
		new_luma_modifyer = frameLumaInfo->luma_modifyer - MAX_LUMA_MODIFYER / LUMA_HISTORY;
	frameLumaInfo->luma_modifyer = clamp(new_luma_modifyer, 0, MAX_LUMA_MODIFYER);
}

//adjust luma (0-255) for single pixel
static int get_new_luma(struct FrameRegionLumaInfo *frameLumaInfo, int current_luma){
	int needModifyLuma = frameLumaInfo->luma_modifyer != 0;

	if (needModifyLuma){
		int new_luma = current_luma - current_luma * frameLumaInfo->luma_modifyer/MAX_LUMA_MODIFYER + frameLumaInfo->luma_modifyer;
		return new_luma;
	} else {
		return current_luma;
	}
}

//public
static void Normalizer_processFrame(Frame *vp){ // TODO: scene detector
	AVFrame avFrame = *vp->frame;
	int currentFrameAverageLuma = 0;
	change_luma_modifyer(&frameLumaInfo);
	//luma_modifyer = MAX_LUMA_MODIFYER;

	int pixels = (avFrame.width * avFrame.height) - 1;
	for (int i = 0; i < pixels; ++i){
		currentFrameAverageLuma += avFrame.data[0][i];
		avFrame.data[0][i] = get_new_luma(&frameLumaInfo, avFrame.data[0][i]);
	}

	frameLumaInfo.prevFrameAverageLuma = currentFrameAverageLuma/pixels;
	return;
}

