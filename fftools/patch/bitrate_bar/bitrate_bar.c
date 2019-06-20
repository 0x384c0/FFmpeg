#include <stdio.h>
#include <string.h>

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#define BITBAR_DATA_SIZE 2560
#define BAR_HEIGH 30
#define BAR_POINTER_HEIGHT BAR_HEIGH * 1.5 //for retina
#define BAR_POINTER_WIDTH 6 //6 for retina, 3 for non retina


int bitrate_bar_data[BITBAR_DATA_SIZE];

uint8_t *bitrate_bar_image=NULL;
int 
aligned_linesize = 0,
frame_width = 0;


static const char* get_str_err(const int err) {
	static char buf[1024];

	if (err == 0){
		strcpy(buf, "NO ERROR");
		return buf;
	}

	if (av_strerror(err, buf, sizeof(buf))) {
		strcpy(buf, "Couldn't get a proper error!");
	}
	return buf;
}
static void print_array_int(int arr[],int size){
	for (int i = 0; i < size; i++){
		printf("%d, ",arr[i]);
	}
}
static void generate_bar_image(Frame *frame){
	if(aligned_linesize != frame->frame->linesize[0]){
		int q,i,w;

		if(bitrate_bar_image){free(bitrate_bar_image);}

		aligned_linesize = frame->frame->linesize[0];
		frame_width = frame->width;
		bitrate_bar_image = malloc(aligned_linesize * BAR_HEIGH);

		int max = 0;

		for(q=0;q<BITBAR_DATA_SIZE-3;q++){
			max=0;
			for(w=0;w<3;w++){
				max+=bitrate_bar_data[q+w];
			}
			bitrate_bar_data[q]=max/3;
		}

		max=0;
		for(q = 0; q < BITBAR_DATA_SIZE; q++){
			if(bitrate_bar_data[q] > max){ max = bitrate_bar_data[q];}
		}

		for(q = 0; q < BITBAR_DATA_SIZE; q++){
			i = q * frame_width / BITBAR_DATA_SIZE;
			for(w=0;w<BAR_HEIGH;w++){
				if (max){
					bitrate_bar_image[i + w * aligned_linesize] = bitrate_bar_data[q] * 255 / max;
				}
			}
		}
	}
}
static void insert_bar(Frame *frame){
	if(bitrate_bar_image){
		memcpy(&frame->frame->data[0][0],bitrate_bar_image,	aligned_linesize * BAR_HEIGH);//copy bitrate data to 1st shannel
		// memset(&frame->frame->data[1][0],255/2,				aligned_linesize*BAR_HEIGH/4);//fill 2nd shannel
		// memset(&frame->frame->data[2][0],255/2,				aligned_linesize*BAR_HEIGH/4);//fill 3rd shannel
	}
}
static void insert_pointer_over_bar(Frame *frame,VideoState *video_state){
	double 
	master_clock = get_master_clock(video_state),
	duration = video_state->ic->duration / (double) 1000000LL;

	if (isnan(master_clock)) { return; }

	int
	position = (double) frame_width * (master_clock / duration) - BAR_POINTER_WIDTH/2;
	// printf("\nmaster_clock: %f \t\t of duration: %f\n", master_clock, duration);
	// printf("position: %d \t\t of width: %d\n", position, aligned_linesize);
	// printf("linesize[0]: %d, width: %d  \n", frame->frame->linesize[0], frame->frame->width); // linesize might be greater, then width, should calculate aligmnent

	for(int height=0;height<BAR_POINTER_HEIGHT;height++){
		for(int width=0;width<BAR_POINTER_WIDTH;width++){
			if ((position + width) >= frame_width){
				return; // bar out of frame
			}

			int color = 255;//white
			if (width >= ((double) BAR_POINTER_WIDTH)/3 && width < ((double)BAR_POINTER_WIDTH) * 2.0 / 3.0 ){
				color = 0;
			}
			frame->frame->data[0][position + width + height * (aligned_linesize) ] = color;//draw vertical line
		}
	}
}


//public
static void BitRateBar_insert(Frame *frame,VideoState *video_state){
	generate_bar_image(frame);
	insert_bar(frame);
	insert_pointer_over_bar(frame,video_state);
}

static int BitRateBar_generate(AVStream *av_stream,AVFormatContext *ic,AVPacket *pkt,int st_index[]){

	// vsrat bitrate index builder
	int ret;
	int fuck_id=0;
	memset(bitrate_bar_data,0,BITBAR_DATA_SIZE * sizeof(int));

	// printf("\n st_index: ");
	// print_array_int(st_index,ARRAY_SIZE(st_index));
	// printf("\n AVMEDIA_TYPE_VIDEO: %d \n",AVMEDIA_TYPE_VIDEO);
	// printf("st_index[AVMEDIA_TYPE_VIDEO]: %d \n",st_index[AVMEDIA_TYPE_VIDEO]);

	fuck_id=st_index[AVMEDIA_TYPE_VIDEO];

	AVCodecContext *fuck_avctx=av_stream->codec;
	// printf("Butrate bar! Width: %d, id=%d\n",fuck_avctx->width,fuck_id);
	ret = avformat_seek_file(ic, -1, INT64_MIN, 0, INT64_MAX, 0);


	if(av_stream->nb_index_entries>0/*10000*/){
	// if(0){
		// fast index reading available!
		// printf("We have index with %d entries\n",av_stream->nb_index_entries);
		int q;
		int64_t prev=0;
		for(q=0; q<av_stream->nb_index_entries; q++){

			// int is_time_in_milliseconds

			int bar_id = (av_rescale_q(av_stream->index_entries[q].timestamp, av_stream->time_base, AV_TIME_BASE_Q) - ic->start_time) * BITBAR_DATA_SIZE / ic->duration;

			//DEBUG
			// printf("av_rescale_q %d\n", av_rescale_q(av_stream->index_entries[q].timestamp,  av_stream->time_base, AV_TIME_BASE_Q));
			// printf("bar_id: %d, timestamp: %d, time_base: %d / %d, start_time:%d, av_rescale_q:%d, duration:%d \n",
			// 	bar_id,
			// 	av_stream->index_entries[q].timestamp,
			// 	av_stream->time_base.num, av_stream->time_base.den,
			// 	ic->start_time,
			// 	av_rescale_q(av_stream->index_entries[q].timestamp,  av_stream->time_base, AV_TIME_BASE_Q), // current time
			// 	ic->duration);
			// printf("\n%d: bar_id:%d, pos:%d, pos-prev:%d, size:%d",
			// 	q,
			// 	(int)bar_id,
			// 	(int)av_stream->index_entries[q].pos,
			// 	(int)av_stream->index_entries[q].pos-prev,
			// 	(int)av_stream->index_entries[q].size
			// );

			bitrate_bar_data[bar_id] += av_stream->index_entries[q].size == 0 ? 
											av_stream->index_entries[q].pos-prev :
											av_stream->index_entries[q].size;
			prev = av_stream->index_entries[q].pos;
		}

	} else {
		uint64_t seen_last_time=0;
		uint64_t fuck_1=0;

		ret = avformat_seek_file(ic, -1, INT64_MIN, 0, INT64_MAX, 0);

		printf("avformat_seek_file(%d,%d,%d,%d,%d,%d)\n", ic, -1, INT64_MIN, 0, INT64_MAX, 0);
		printf("avformat_seek_file	0x%08x \t\t ERROR: %s\n", ret, get_str_err(ret));
		while(1){

			ret = av_read_frame(ic, pkt);
			printf("av_read_frame		0x%08x \t\t ERROR: %s\n", ret, get_str_err(ret));
			if(ret<0){break;}

			if(pkt->stream_index == fuck_id){

				fuck_1 = pkt->pts != AV_NOPTS_VALUE ?
											pkt->pts :
											pkt->dts;
				if(fuck_1 == AV_NOPTS_VALUE){fuck_1 = seen_last_time;}

				//DEBUG
				printf("\ngot video packet %d! stream:%d, size:%d, pos:%d, time:%d\n",ret,
				pkt->stream_index,(int)pkt->size,(int)pkt->pos,
				(int)(av_rescale_q(fuck_1, av_stream->time_base, AV_TIME_BASE_Q)/1000000LL));
				
				int bar_id = (av_rescale_q(fuck_1, av_stream->time_base, AV_TIME_BASE_Q)-ic->start_time)*BITBAR_DATA_SIZE/ic->duration;
				bitrate_bar_data[bar_id]+=pkt->size;
			}
			av_free_packet(pkt);
		}

		ret = avformat_seek_file(ic, -1, INT64_MIN, 0, INT64_MAX, 0);

	}

	// printf("bitrate_bar_data\n" );
	// print_array_int(bitrate_bar_data,ARRAY_SIZE(bitrate_bar_data));
	// printf("\n\n\n" );
	return ret;
}