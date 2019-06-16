#include <stdio.h>
#include <math.h>
#include "libavutil/frame.h"

#define INTERPOLATE

#define CHANNEL_ID 0
#define REGIONS_W 16
#define REGIONS_H 9

#define MAX_LUMA 255 // int8 value
#define MAX_LUMA_MODIFYER 255 * 0.3 // int8 value
#define AVEARGE_LUMA_MIN 255 * 0.2 // int8 value, luma adjust treshold
#define LUMA_HISTORY 30.0 // frames

struct FrameRegionLumaInfo {
	int
	prevFrameAverageLuma,
	luma_modifyer; //from 0 to MAX_LUMA_MODIFYER
};

struct SurroundingLuma{
	int
	bottomRight,
	bottom,
	rigth,
	center
};

struct FrameRegionLumaInfo frameLumaInfo[REGIONS_W][REGIONS_H] = { MAX_LUMA, 0 };


static int clamp(int x,int minVal,int maxVal){
	if (x < minVal)
		return minVal;
	if (x > maxVal)
		return maxVal;
	return x;
}

//interpolate
static int interpolate_linear(int a,int b,int c,int d,float x,float y){//f(0,0) f(1,0) f(0,1) f(1,1)
	return (a-b-c+d)*x*y+(b-a)*x+(c-a)*y+a;
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
static int get_new_luma(struct SurroundingLuma surroundingLuma, float x, float y, int current_luma){

	#ifdef INTERPOLATE
		int luma_modifyer = interpolate_linear(
				surroundingLuma.center,
				surroundingLuma.rigth,
				surroundingLuma.bottom,
				surroundingLuma.bottomRight,
				x,
				y
			);
	#else
		int luma_modifyer = surroundingLuma.center;
	#endif

	int needModifyLuma = luma_modifyer != 0;
	if (needModifyLuma){
		int new_luma = current_luma - current_luma * luma_modifyer/MAX_LUMA + luma_modifyer;
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
		}
	}

	for (int w = 0; w < REGIONS_W; w++){
		for (int h = 0; h < REGIONS_H; h++){
			int 
			totalLuma = 0,
			offsetW = regionW * w,
			offsetH = regionH * h;

			struct SurroundingLuma surroundingLuma = {
				frameLumaInfo[clamp(w+1,0,REGIONS_W-1)][clamp(h+1,0,REGIONS_H-1)].luma_modifyer,
				frameLumaInfo[w][clamp(h+1,0,REGIONS_H-1)].luma_modifyer,
				frameLumaInfo[clamp(w+1,0,REGIONS_W-1)][h].luma_modifyer,
				frameLumaInfo[w][h].luma_modifyer
			};

			for (int frameW = 0; frameW < regionW; frameW++){
				for (int frameH = 0; frameH < regionH; frameH++){
					int pixel = (offsetW + frameW) + ((offsetH + frameH) * avFrame.width);
					totalLuma += avFrame.data[CHANNEL_ID][pixel];
					avFrame.data[CHANNEL_ID][pixel] = get_new_luma(
						surroundingLuma,
						frameW/(float)regionW,
						frameH/(float)regionH,
						avFrame.data[CHANNEL_ID][pixel]
					);
				}
			}
			frameLumaInfo[w][h].prevFrameAverageLuma = totalLuma/(regionW * regionH);
		}
	}

	return;
}

