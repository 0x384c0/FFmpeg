#include <stdio.h>
#include <math.h>

#include "normalize.h"

#define INTERPOLATE

#define REGIONS_W 16
#define REGIONS_H 9

#define MAX_LUMA 255 // int8 value
#define MAX_LUMA_MODIFYER 255 * 0.6 // int8 value
#define AVEARGE_FRAME_LUMA_MIN 255 * 0.15 // int8 value, region luma adjust treshold
#define AVEARGE_REGION_LUMA_MIN 255 * 0.2 // int8 value, region luma adjust treshold
#define REGION_LUMA_HISTORY 30.0 // frames
#define FRAME_LUMA_HISTORY REGION_LUMA_HISTORY * 0.6

struct FrameRegionInfo {
	int
	prevFrameAverageLuma,
	lumaModifyer,
	offsetW,
	offsetH,
	w,
	h; //from 0 to MAX_LUMA_MODIFYER
};

struct SurroundingLuma{
	int
	bottomRight,
	bottom,
	rigth,
	center
};

struct Normalizer{
	struct FrameRegionInfo totalFrameInfo;
	struct FrameRegionInfo frameInfo[REGIONS_W][REGIONS_H];
};


int clamp(int x,int minVal,int maxVal){
	if (x < minVal)
		return minVal;
	if (x > maxVal)
		return maxVal;
	return x;
}

//interpolate
int interpolate_linear(int a,int b,int c,int d,float x,float y){//f(0,0) f(1,0) f(0,1) f(1,1)
	return (a-b-c+d)*x*y+(b-a)*x+(c-a)*y+a;
}

//range compression
int compress(int x,int min){
	return x - x * min/MAX_LUMA + min;
}
#define INTERSECT_X 81 //x of intersection between compress and gain functions
int gain(int x,int min){
	return ((float)(x*min))/120.0 + x;
}

//smooth luma adjusting
void change_lumaModifyer(struct FrameRegionInfo *frameRegionInfo, int activationTreshold, int maxValue, int historyLen){
	int new_lumaModifyer = 0;
	if (frameRegionInfo->prevFrameAverageLuma < activationTreshold)
		new_lumaModifyer = frameRegionInfo->lumaModifyer + maxValue / historyLen;
	else if (frameRegionInfo->lumaModifyer != 0)
		new_lumaModifyer = frameRegionInfo->lumaModifyer - maxValue / historyLen;
	frameRegionInfo->lumaModifyer = clamp(new_lumaModifyer, 0, maxValue);
}

//adjust luma (0-255) for single pixel
int get_new_luma(struct FrameRegionInfo totalFrameInfo, struct SurroundingLuma surroundingLuma, float x, float y, int current_luma){
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
	lumaModifyer = lumaModifyer * (totalFrameInfo.lumaModifyer/(float)MAX_LUMA);


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
struct Normalizer *Normalizer_new(){
	struct Normalizer *normalizer = calloc(1,sizeof(struct Normalizer));
	struct FrameRegionInfo defaultTotalFrameInfo = { MAX_LUMA, 0, 0,0,0,0 };
	struct FrameRegionInfo defaultframeInfo[REGIONS_W][REGIONS_H] = { MAX_LUMA, 0, 0,0,0,0 };
	normalizer->totalFrameInfo = defaultTotalFrameInfo;
	memcpy(&normalizer->frameInfo, &defaultframeInfo, sizeof(defaultframeInfo));
	return normalizer;
}

void Normalizer_delete(struct Normalizer *normalizer){
	free(normalizer);
}

void Normalizer_processFrame(struct Normalizer *normalizer, AVFrame *avFrameP){
	struct AVFrame avFrame = *avFrameP;
	int alignedLinesize = avFrame.linesize[NORMALIZE_CHANNEL_ID];

	//calculate region bounds for every region if nedded
	if (normalizer->frameInfo[0][0].w == 0){
		int
		regionW = ceil((float) avFrame.width / REGIONS_W),
		regionH = ceil((float) avFrame.height / REGIONS_H),
		regionsWFitted = avFrame.width % REGIONS_W == 0,
		regionsHFitted = avFrame.height % REGIONS_H == 0;

		for (int regionWId = 0; regionWId < REGIONS_W; regionWId++){
			for (int regionHId = 0; regionHId < REGIONS_H; regionHId++){
				normalizer->frameInfo[regionWId][regionHId].offsetW = regionW * regionWId;
				if (regionsWFitted || regionWId < REGIONS_W - 1)
					normalizer->frameInfo[regionWId][regionHId].w = regionW;
				else
					normalizer->frameInfo[regionWId][regionHId].w = regionW - (regionW - (avFrame.width - normalizer->frameInfo[regionWId][regionHId].offsetW));

				normalizer->frameInfo[regionWId][regionHId].offsetH = regionH * regionHId;
				if (regionsHFitted || regionHId < REGIONS_H - 1)
					normalizer->frameInfo[regionWId][regionHId].h = regionH;
				else
					normalizer->frameInfo[regionWId][regionHId].h = regionH - (regionH - (avFrame.height - normalizer->frameInfo[regionWId][regionHId].offsetH));
			
				int
				regionW = normalizer->frameInfo[regionWId][regionHId].w,
				offsetW = normalizer->frameInfo[regionWId][regionHId].offsetW,
				regionH = normalizer->frameInfo[regionWId][regionHId].h,
				offsetH = normalizer->frameInfo[regionWId][regionHId].offsetH;
			}
		}
	}


	int totalFrameLuma = 0;
	for (int regionWId = 0; regionWId < REGIONS_W; regionWId++){
		for (int regionHId = 0; regionHId < REGIONS_H; regionHId++){
			change_lumaModifyer(&normalizer->frameInfo[regionWId][regionHId], AVEARGE_REGION_LUMA_MIN, MAX_LUMA_MODIFYER,REGION_LUMA_HISTORY);
			totalFrameLuma += normalizer->frameInfo[regionWId][regionHId].prevFrameAverageLuma;
		}
	}
	change_lumaModifyer(&normalizer->totalFrameInfo, AVEARGE_FRAME_LUMA_MIN, MAX_LUMA, FRAME_LUMA_HISTORY);
	int needModifyLuma = normalizer->totalFrameInfo.lumaModifyer > 0;

	for (int regionWId = 0; regionWId < REGIONS_W; regionWId++){
		for (int regionHId = 0; regionHId < REGIONS_H; regionHId++){
			int 
			totalLuma = 0,
			regionW = normalizer->frameInfo[regionWId][regionHId].w,
			regionH = normalizer->frameInfo[regionWId][regionHId].h,
			offsetW = normalizer->frameInfo[regionWId][regionHId].offsetW,
			offsetH = normalizer->frameInfo[regionWId][regionHId].offsetH;

			if (needModifyLuma){//modify luma and calculate current total luma
				struct SurroundingLuma surroundingLuma = {
					normalizer->frameInfo[clamp(regionWId+1,0,REGIONS_W-1)][clamp(regionHId+1,0,REGIONS_H-1)].lumaModifyer,
					normalizer->frameInfo[regionWId][clamp(regionHId+1,0,REGIONS_H-1)].lumaModifyer,
					normalizer->frameInfo[clamp(regionWId+1,0,REGIONS_W-1)][regionHId].lumaModifyer,
					normalizer->frameInfo[regionWId][regionHId].lumaModifyer
				};

				for (int frameW = 0; frameW < regionW; frameW++){
					for (int frameH = 0; frameH < regionH; frameH++){
						int pixel = (offsetW + frameW) + ((offsetH + frameH) * alignedLinesize);
						totalLuma += avFrame.data[NORMALIZE_CHANNEL_ID][pixel];
						avFrame.data[NORMALIZE_CHANNEL_ID][pixel] = get_new_luma(
							normalizer->totalFrameInfo,
							surroundingLuma,
							frameW/(float)regionW,
							frameH/(float)regionH,
							avFrame.data[NORMALIZE_CHANNEL_ID][pixel]
						);
					}
				}
			} else {//only calculate current total luma
				for (int frameW = 0; frameW < regionW; frameW++){
					for (int frameH = 0; frameH < regionH; frameH++){
						int pixel = (offsetW + frameW) + ((offsetH + frameH) * alignedLinesize);
						totalLuma += avFrame.data[NORMALIZE_CHANNEL_ID][pixel];
					}
				}
			}
			normalizer->frameInfo[regionWId][regionHId].prevFrameAverageLuma = totalLuma/(regionW * regionH);
		}
	}

	normalizer->totalFrameInfo.prevFrameAverageLuma = totalFrameLuma/(REGIONS_W*REGIONS_H);

	return;
}

