#include <stdio.h>
#include <math.h>
#include "libavutil/frame.h"

#define INTERPOLATE

#define CHANNEL_ID 0
#define REGIONS_W 16
#define REGIONS_H 9

#define MAX_LUMA 255 // int8 value
#define MAX_LUMA_MODIFYER 255 * 0.6 // int8 value
#define AVEARGE_FRAME_LUMA_MIN 255 * 0.12 // int8 value, region luma adjust treshold
#define AVEARGE_REGION_LUMA_MIN 255 * 0.2 // int8 value, region luma adjust treshold
#define REGION_LUMA_HISTORY 30.0 // frames
#define FRAME_LUMA_HISTORY REGION_LUMA_HISTORY * 0.6

struct FrameRegionLumaInfo {
	int
	prevFrameAverageLuma,
	lumaModifyer; //from 0 to MAX_LUMA_MODIFYER
};

struct SurroundingLuma{
	int
	bottomRight,
	bottom,
	rigth,
	center
};

//instance data
struct FrameRegionLumaInfo totalFrameLumaInfo = { MAX_LUMA, 0 };
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

//range compression
static int compress(int x,int min){
	return x - x * min/MAX_LUMA + min;
}
#define INTERSECT_X 81 //x of intersection between compress and gain functions
static int gain(int x,int min){
	return ((float)(x*min))/120.0 + x;
}

//smooth luma adjusting
static void change_lumaModifyer(struct FrameRegionLumaInfo *frameRegionLumaInfo, int activationTreshold, int maxValue, int historyLen){
	int new_lumaModifyer = 0;
	if (frameRegionLumaInfo->prevFrameAverageLuma < activationTreshold)
		new_lumaModifyer = frameRegionLumaInfo->lumaModifyer + maxValue / historyLen;
	else if (frameRegionLumaInfo->lumaModifyer != 0)
		new_lumaModifyer = frameRegionLumaInfo->lumaModifyer - maxValue / historyLen;
	frameRegionLumaInfo->lumaModifyer = clamp(new_lumaModifyer, 0, maxValue);
}

//adjust luma (0-255) for single pixel
static int get_new_luma(struct SurroundingLuma surroundingLuma, float x, float y, int current_luma){
	//get luma modifyer for pixel
	#ifdef INTERPOLATE
		int lumaModifyer = interpolate_linear(
				surroundingLuma.center,
				surroundingLuma.rigth,
				surroundingLuma.bottom,
				surroundingLuma.bottomRight,
				x,
				y
			);
	#else
		int lumaModifyer = surroundingLuma.center;
	#endif

	//apply total frame modifyer to prevent flickering
	lumaModifyer = lumaModifyer * (totalFrameLumaInfo.lumaModifyer/(float)MAX_LUMA);


	//apply modifyer if needed
	int needModifyLuma = lumaModifyer != 0;
	if (needModifyLuma){
		int new_luma;
		if (current_luma < INTERSECT_X)
			new_luma = gain(current_luma,lumaModifyer);
		else
			new_luma = compress(current_luma,lumaModifyer);
		return new_luma;
	} else {
		return current_luma;
	}
}

//public
static void Normalizer_processFrame(Frame *vp){
	AVFrame avFrame = *vp->frame;

	int
	regionW = round((float) avFrame.width / REGIONS_W),
	regionH = round((float) avFrame.height / REGIONS_H);

	int totalFrameLuma = 0;
	for (int w = 0; w < REGIONS_W; w++){
		for (int h = 0; h < REGIONS_H; h++){
			change_lumaModifyer(&frameLumaInfo[w][h], AVEARGE_REGION_LUMA_MIN, MAX_LUMA_MODIFYER,REGION_LUMA_HISTORY);
			totalFrameLuma += frameLumaInfo[w][h].prevFrameAverageLuma;
		}
	}
	change_lumaModifyer(&totalFrameLumaInfo, AVEARGE_FRAME_LUMA_MIN, MAX_LUMA, FRAME_LUMA_HISTORY);
	int needModifyLuma = totalFrameLumaInfo.lumaModifyer > 0;

	for (int w = 0; w < REGIONS_W; w++){
		for (int h = 0; h < REGIONS_H; h++){
			int 
			totalLuma = 0,
			offsetW = regionW * w,
			offsetH = regionH * h;

			if (needModifyLuma){//modify luma and calculate current total luma
				struct SurroundingLuma surroundingLuma = {
					frameLumaInfo[clamp(w+1,0,REGIONS_W-1)][clamp(h+1,0,REGIONS_H-1)].lumaModifyer,
					frameLumaInfo[w][clamp(h+1,0,REGIONS_H-1)].lumaModifyer,
					frameLumaInfo[clamp(w+1,0,REGIONS_W-1)][h].lumaModifyer,
					frameLumaInfo[w][h].lumaModifyer
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
			} else {//only calculate current total luma
				for (int frameW = 0; frameW < regionW; frameW++){
					for (int frameH = 0; frameH < regionH; frameH++){
						int pixel = (offsetW + frameW) + ((offsetH + frameH) * avFrame.width);
						totalLuma += avFrame.data[CHANNEL_ID][pixel];
					}
				}
			}
			frameLumaInfo[w][h].prevFrameAverageLuma = totalLuma/(regionW * regionH);
		}
	}

	totalFrameLumaInfo.prevFrameAverageLuma = totalFrameLuma/(REGIONS_W*REGIONS_H);

	return;
}

