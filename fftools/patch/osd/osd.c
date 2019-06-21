#include <stdint.h>
#include <math.h>
#include "libavutil/frame.h"
#include "libavformat/avformat.h"

#include "osd.h"

uint8_t vsrat_font[]={
    #include "1font.inc"
};

#define FONT_WIDTH 240
#define FONT_HEIGHT 22
#define FONT_CHARS 13

struct Osd{
    int64_t duration;
    uint8_t *vsrat_font_offset[FONT_CHARS];
    int vsrat_font_len[FONT_CHARS];
    int topInset;
    int rightInset;
};

//private
void font_init(struct Osd* osd){
    int q;
    int n=0;
    int is_char=0;
    int offset;
    for(q=0;q<FONT_WIDTH;q++){
        if(vsrat_font[q]){
            if(!is_char){
                offset=q;
                is_char++;
            }
        } else {
            if(is_char){
                osd->vsrat_font_offset[n]=vsrat_font+offset+FONT_WIDTH;
                osd->vsrat_font_len[n]=q-offset;
                is_char=0;
                n++;
            }
        }
    }
}

char *vsrat_get_time(struct Osd *osd, double audio_clock,AVFrame *vp){
    static char tmp[100];
    int64_t ts;
    int ns, hh, mm, ss;
    int tns, thh, tmm, tss;
    tns  = osd->duration / 1000000LL;
    thh  = tns / 3600;
    tmm  = (tns % 3600) / 60;
    tss  = (tns % 60);

    ns   = (int)audio_clock;
    hh   = ns / 3600;
    mm   = (ns % 3600) / 60;
    ss   = (ns % 60);
    sprintf(tmp,"%02d:%02d:%02d/%02d:%02d:%02d",
                                hh, mm, ss, thh, tmm, tss);

    return(tmp);
}

void podpisat(struct Osd *osd, AVFrame *avFrame, char *text, int posY, int posXRightInset){
    char *chars="0123456789:./";

    int q;
    int e,w;
    int len=strlen(text);
    int posX;
    int p;
    int charID;
    char *charPos;
    uint8_t *charFont;
    int sample;
    uint8_t *canvas;
    int draw_size=0;

    for(e=0;e<len;e++){
    if(text[e]==' '){posX+=10;continue;}
    charPos=strchr(chars,text[e]);
    if(!charPos){continue;}
    charID=charPos-chars;
    if(charID<0 || charID>=FONT_CHARS){continue;}
    draw_size+=osd->vsrat_font_len[charID]+1;
    }


    posX=avFrame->width-draw_size; //draw at right frame side
    posX-=posXRightInset;
    for(e=0;e<len;e++){
    if(text[e]==' '){posX+=10;continue;}
    charPos=strchr(chars,text[e]);
    if(!charPos){continue;}
    charID=charPos-chars;
    if(charID<0 || charID>=FONT_CHARS){continue;}
    charFont=osd->vsrat_font_offset[charID];


    if(posX>=0){
    canvas=&avFrame->data[0][posX+posY*avFrame->linesize[0]];
    for(w=0;w<FONT_HEIGHT;w++){
    if(w+posY>avFrame->height){continue;}
    for(q=0;q<osd->vsrat_font_len[charID];q++){
    *canvas++=*canvas<*charFont?*charFont:*canvas;
    charFont++;
    }
    charFont+=FONT_WIDTH-osd->vsrat_font_len[charID];
    canvas+=avFrame->linesize[0]-osd->vsrat_font_len[charID];
    }
    }
    posX+=1+osd->vsrat_font_len[charID];
    }
}

//public
struct Osd *OSD_new(struct AVFormatContext *ic, int topInset, int rightInset){   
    struct Osd *osd = calloc(1,sizeof(struct Osd));
    osd->duration = ic->duration;
    osd->topInset = topInset;
    osd->rightInset = rightInset;
    font_init(osd);
    return osd;
}

void OSD_delete(struct Osd *osd){
    free(osd);
}

void OSD_processFrame(struct Osd *osd, double audio_clock, AVFrame *frame){
    char *time=vsrat_get_time(osd,audio_clock,frame);
    podpisat(osd ,frame, time, osd->topInset, osd->rightInset);
}
