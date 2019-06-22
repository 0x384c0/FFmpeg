#include "bitrate_bar.h"

#include <stdio.h>
#include <string.h>

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

#define BITBAR_DATA_SIZE 2560
#define BAR_POINTER_WIDTH 6 //6 for retina, 3 for non retina

struct BitRateBar{
    uint8_t *bitrate_bar_image;
    int bitrate_bar_data[BITBAR_DATA_SIZE];
    int 
    aligned_linesize,
    frame_width,
    barHeight,
    barPointerHeight;
};


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
static void generate_bar_image(struct BitRateBar *bitRateBar, AVFrame *frame){
    if(bitRateBar->aligned_linesize != frame->linesize[0]){
        int q,i,w;

        bitRateBar->aligned_linesize = frame->linesize[0];
        bitRateBar->frame_width = frame->width;

        if (!bitRateBar->bitrate_bar_image)
            bitRateBar->bitrate_bar_image = malloc(bitRateBar->aligned_linesize * bitRateBar->barHeight);

        int max = 0;

        for(q=0;q<BITBAR_DATA_SIZE-3;q++){
            max=0;
            for(w=0;w<3;w++){
                max+=bitRateBar->bitrate_bar_data[q+w];
            }
            bitRateBar->bitrate_bar_data[q]=max/3;
        }

        max=0;
        for(q = 0; q < BITBAR_DATA_SIZE; q++){
            if(bitRateBar->bitrate_bar_data[q] > max){ max = bitRateBar->bitrate_bar_data[q];}
        }

        for(q = 0; q < BITBAR_DATA_SIZE; q++){
            i = q * bitRateBar->frame_width / BITBAR_DATA_SIZE;
            for(w=0;w<bitRateBar->barHeight;w++){
                if (max){
                    bitRateBar->bitrate_bar_image[i + w * bitRateBar->aligned_linesize] = bitRateBar->bitrate_bar_data[q] * 255 / max;
                }
            }
        }
    }
}
static void insert_bar(struct BitRateBar *bitRateBar, AVFrame *frame){
    if(bitRateBar->bitrate_bar_image){
        memcpy(&frame->data[0][0],bitRateBar->bitrate_bar_image,    bitRateBar->aligned_linesize * bitRateBar->barHeight);//copy bitrate data to 1st shannel
        // memset(&frame->frame->data[1][0],255/2,                bitRateBar->aligned_linesize*bitRateBar->barHeight/4);//fill 2nd shannel
        // memset(&frame->frame->data[2][0],255/2,                bitRateBar->aligned_linesize*bitRateBar->barHeight/4);//fill 3rd shannel
    }
}
static void insert_pointer_over_bar(struct BitRateBar *bitRateBar, AVFrame *frame, double master_clock, int64_t ic_duration){
    double duration = ic_duration / (double) 1000000LL;

    if (isnan(master_clock)) { return; }

    int
    position = (double) bitRateBar->frame_width * ((double) master_clock / duration) - BAR_POINTER_WIDTH/2,
    dataSize = frame->width * frame->height;

    for(int height=0;height<bitRateBar->barPointerHeight;height++){
        for(int width=0;width<BAR_POINTER_WIDTH;width++){
            if ((position + width) >= bitRateBar->frame_width || position + width < 0){
                continue; // bar out of frame
            }

            int color = 255;//white
            if (width >= ((double) BAR_POINTER_WIDTH)/3 && width < ((double)BAR_POINTER_WIDTH) * 2.0 / 3.0 ){
                color = 0;
            }
            int pointerPixel = position + width + height * (bitRateBar->aligned_linesize);
            if (pointerPixel > 0 && pointerPixel < dataSize)
                frame->data[0][pointerPixel] = color;//draw vertical line
        }
    }
}


//public

struct BitRateBar *BitRateBar_new(AVStream *av_stream,AVFormatContext *ic,AVPacket *pkt,int st_index[],int bitbarHeight){
    struct BitRateBar *bitRateBar = calloc(1,sizeof(struct BitRateBar));
    bitRateBar->bitrate_bar_image = NULL;


    bitRateBar->barHeight = bitbarHeight;
    bitRateBar->barPointerHeight = bitRateBar->barHeight * 1.5;

    // vsrat bitrate index builder
    int ret;
    int fuck_id=0;
    memset(bitRateBar->bitrate_bar_data,0,BITBAR_DATA_SIZE * sizeof(int));

    fuck_id=st_index[AVMEDIA_TYPE_VIDEO];

    AVCodecContext *fuck_avctx=av_stream->codec;
    ret = avformat_seek_file(ic, -1, INT64_MIN, 0, INT64_MAX, 0);


    if(av_stream->nb_index_entries>0){
        int q;
        int64_t prev=0;
        for(q=0; q<av_stream->nb_index_entries; q++){

            int bar_id = (av_rescale_q(av_stream->index_entries[q].timestamp, av_stream->time_base, AV_TIME_BASE_Q) - ic->start_time) * BITBAR_DATA_SIZE / ic->duration;

            bitRateBar->bitrate_bar_data[bar_id] += av_stream->index_entries[q].size == 0 ? 
                                            av_stream->index_entries[q].pos-prev :
                                            av_stream->index_entries[q].size;
            prev = av_stream->index_entries[q].pos;
        }

    } else {
        uint64_t seen_last_time=0;
        uint64_t fuck_1=0;

        ret = avformat_seek_file(ic, -1, INT64_MIN, 0, INT64_MAX, 0);

        printf("avformat_seek_file(%d,%d,%d,%d,%d,%d)\n", ic, -1, INT64_MIN, 0, INT64_MAX, 0);
        printf("avformat_seek_file    0x%08x \t\t ERROR: %s\n", ret, get_str_err(ret));
        while(1){

            ret = av_read_frame(ic, pkt);
            printf("av_read_frame        0x%08x \t\t ERROR: %s\n", ret, get_str_err(ret));
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
                bitRateBar->bitrate_bar_data[bar_id]+=pkt->size;
            }
            av_free_packet(pkt);
        }

        ret = avformat_seek_file(ic, -1, INT64_MIN, 0, INT64_MAX, 0);
    }

    return bitRateBar;
}

void BitRateBar_delete(struct BitRateBar *bitRateBar){
    if (bitRateBar->bitrate_bar_image) free(bitRateBar->bitrate_bar_image);
    free(bitRateBar);
}

void BitRateBar_processFrame(struct BitRateBar *bitRateBar, AVFrame *frame, double master_clock, int64_t ic_duration){
    generate_bar_image(bitRateBar,frame);
    insert_bar(bitRateBar,frame);
    insert_pointer_over_bar(bitRateBar,frame,master_clock,ic_duration);
}