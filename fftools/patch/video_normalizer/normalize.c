#include <stdio.h>
#include "libavutil/frame.h"

#define MAX_LUMA 255
#define MAX_LUMA_MODIFYER 100 // in percents
#define MAX_LUMA_THAT_CAN_BE_MODIFYED 128
#define AVEARGE_LUMA_MIN 64
#define LUMA_HISTORY 30.0 // in frames

//private
typedef unsigned int uint;

uint
prevFrameAverageLuma = MAX_LUMA,
luma_modifyer = 0;

static uint clamp(uint x,uint minVal,uint maxVal){
	if (x < minVal)
		return minVal;
	if (x > maxVal)
		return maxVal;
	return x;
}

static void change_luma_modifyer(int increment){
	if (increment)
		luma_modifyer += MAX_LUMA_MODIFYER/LUMA_HISTORY;
	else if (luma_modifyer != 0)
		luma_modifyer -= MAX_LUMA_MODIFYER/LUMA_HISTORY;
	luma_modifyer = clamp(luma_modifyer, 0, MAX_LUMA_MODIFYER);
}

static uint get_new_luma(uint current_luma){
	if (current_luma < MAX_LUMA_THAT_CAN_BE_MODIFYED){
		float m = (float)luma_modifyer / MAX_LUMA_MODIFYER;



		uint r = 20;
		if (current_luma > MAX_LUMA_THAT_CAN_BE_MODIFYED - r){
			m *= (MAX_LUMA_THAT_CAN_BE_MODIFYED - current_luma)/(float)r;
		}

		uint new_luma = current_luma * (1 + m);






		return clamp(new_luma,0,MAX_LUMA);
	} else {
		return current_luma;
	}
}

//public
static void Normalizer_processFrame(Frame *vp){ // TODO: scene detector
	AVFrame avFrame = *vp->frame;
	int currentFrameAverageLuma = 0;
	change_luma_modifyer(prevFrameAverageLuma < AVEARGE_LUMA_MIN);

	int pixels = (avFrame.width * avFrame.height) - 1;
	for (int i = 0; i < pixels; ++i){
		currentFrameAverageLuma += avFrame.data[0][i];
		avFrame.data[0][i] = get_new_luma(avFrame.data[0][i]);
	}

	prevFrameAverageLuma = currentFrameAverageLuma/pixels;
	return;
}

