#import "detector.c"
//private

typedef struct{
    double lumamin;
    double lumamax;
    int lumamindir;
    int lumamaxdir;
} EBAL;

static EBAL **normalizers=NULL;

int BMP_PITHES = 100;

static void do_norm_calc(int ox, int oy, int width, int height, Frame *vp, EBAL *normalizer,AVFrame *pict){


	if(ox<0){width+=ox;ox=0;}
	if(oy<0){height+=oy;oy=0;}
	if(ox+width>vp->width){width=vp->width-ox;}
	if(oy+height>vp->height){height=vp->height-oy;}

	// determine low and hi level of luminocity
	int q,w;
	int v;
	int p;
	int lumamin=255,lumamax=0;
	for(w=0;w<height;w+=10){
	for(q=0;q<width;q+=10){
	v=pict->data[0][ox+q+(oy+w)*BMP_PITHES];//vp->bmp->pitches[0]];
	if(v<lumamin){lumamin=v;}
	if(v>lumamax){lumamax=v;}
	}
	}

	int diff=FFMAX(abs(normalizer->lumamin-lumamin),abs(normalizer->lumamax-lumamax));

	// try to detect scene change by changing colors
	/*
	int chsize=vp->bmp->pitches[2]*vp->height/4;
	int chstep=chsize/50;
	int chth=50;
	int chchanged=0;
	for(q=0;q<50;q++){
	if(abs((int)vp->prevpic[q]-(int)pict.data[1][q*chstep])>chth || abs((int)vp->prevpic[q+50]-(int)pict.data[2][q*chstep])>chth){
	chchanged++;
	}
	vp->prevpic[q   ]=pict.data[1][q*chstep];
	vp->prevpic[q+50]=pict.data[2][q*chstep];
	}

	fprintf(stderr,"changed pins: %d at %p\n",chchanged,vp);
	*/

	/*if(diff>70){
	normalizer->lumamin=lumamin;normalizer->lumamax=lumamax;fprintf(stderr,"scene changed!!!\n");
	}*/



	if(diff>128){
	normalizer->lumamin=lumamin;
	normalizer->lumamax=lumamax;
	}

	normalizer->lumamin=(normalizer->lumamin*52.0+(double)lumamin)/53.0;
	normalizer->lumamax=(normalizer->lumamax*52.0+(double)lumamax)/53.0;


	if(lumamax>normalizer->lumamax+50){
	normalizer->lumamax+=3;
	}

	if(lumamax>normalizer->lumamax+10){
	normalizer->lumamax+=2;
	normalizer->lumamax=(normalizer->lumamax*22.0+(double)lumamax)/23.0;
	}

	if(normalizer->lumamax<50){normalizer->lumamax=50;}

	/*
	if(normalizer->lumamax-normalizer->lumamin<20){
	double mid=((normalizer->lumamax+normalizer->lumamin)*4+lumamin+lumamax)/10.0;
	if(mid<10.0){mid=10.0;}
	if(mid>245.0){mid=245.0;}
	normalizer->lumamin=mid-10.0;
	normalizer->lumamax=mid+10.0;
	}
	*/
}

static void do_norm_do(int ox, int oy, int width, int height, Frame *vp,AVFrame *pict, EBAL *normalizer1, EBAL *normalizer2, EBAL *normalizer3, EBAL *normalizer4){                  

	if(ox<0){width+=ox;ox=0;}
	if(oy<0){height+=oy;oy=0;}
	if(ox+width>vp->width){width=vp->width-ox;}
	if(oy+height>vp->height){height=vp->height-oy;}

	int q,w;
	int v;
	int p;
	int lumamin=255,lumamax=0;


	//fprintf(stderr,"1oldframe: %d/%d, curframe: %d/%d, dir: %d/%d, mult: %d\n",(int)normalizer->lumamin,(int)normalizer->lumamax,lumamin,lumamax,normalizer->lumamindir,normalizer->lumamaxdir,mult);

	double mult_double;
	mult_double=(normalizer1->lumamax-normalizer1->lumamin*0);
	double mult1=(mult_double>0?255.0/mult_double:1);
	mult_double=(normalizer2->lumamax-normalizer2->lumamin*0);
	double mult2=(mult_double>0?255.0/mult_double:1);
	mult_double=(normalizer3->lumamax-normalizer3->lumamin*0);
	double mult3=(mult_double>0?255.0/mult_double:1);
	mult_double=(normalizer4->lumamax-normalizer4->lumamin*0);
	double mult4=(mult_double>0?255.0/mult_double:1);
	int mult;
	double gr_h,gr_v;

	for(w=0;w<height;w+=1){
		// gr_v=(double)w/height;
		//gr_v=0.5-cos(gr_v*M_PI)/2;

		for(q=0;q<width;q+=1){
			gr_h=(double)q/width;
			//gr_h=0.5-cos(gr_h*M_PI)/2;

			p=q+ox+(w+oy)*BMP_PITHES;//vp->bmp->pitches[0];

			int base=0;
			/*(int)((double)((
			normalizer1->lumamin*(1.0-gr_h)+
			normalizer2->lumamin*gr_h)*
			(1.0-gr_v)+(normalizer3->lumamin*(1.0-gr_h)+
			normalizer4->lumamin*gr_h)*(gr_v)));                        
			*/

			v=(int)(
			(double)(pict->data[0][p]-base)*(
			(double)((
			mult1*(1.0-gr_h)+
			mult2*gr_h)*(1.0-gr_v)+
			(mult3*(1.0-gr_h)+mult4*gr_h)*(gr_v))));


			if(v>255){v=255+(255-v)/3;}
			if(v<0){v=0;}
			if(v>255){v=255;}

			pict->data[0][p]=v;
		}
	}
}



//public
void Normalizer_processFrame(Frame *vp){// TODO: optimize
	AVFrame avFrame = *vp->frame;

	// int min = 64;
	// for (int i = 0; i < (avFrame.width * avFrame.height )/2; ++i){
	// 	// avFrame.data[0][i] *= 2;
	// 	if (avFrame.data[0][i] < min)
	// 		avFrame.data[0][i] = min;
	// }
	// return;


	int q,w,step=35,i=0;
	int count=(vp->height/step+1)*(vp->width/step+1);


	int scene = detector_frame(avFrame.data[0],vp->width,vp->height);


	if(normalizers==NULL){
		normalizers=calloc(sizeof(EBAL),count);
			for(q=0;q<count;q++){
			normalizers[q]=calloc(1,sizeof(EBAL));
		}
	}

	if(scene>200){
		do_norm_calc(0,0,vp->width,vp->height,vp,normalizers[0],&avFrame);
		for(q=1;q<count;q++){
			normalizers[q]->lumamax=normalizers[0]->lumamax;
			normalizers[q]->lumamin=normalizers[0]->lumamin;
		}
	} else {
		i=0;
		for(w=0;w<vp->height;w+=step){
			for(q=0;q<vp->width;q+=step){
				do_norm_calc(q-step,w-step,step*3,step*3,vp,normalizers[i++],&avFrame);
			}
		}
	}

	int h_count=(vp->width/step)+((vp->width%step)>0?1:0);
	int v_count=(vp->height/step)+((vp->height%step)>0?1:0);

	int e;

	int mdiff=10;

	for(e=0;e<10;e++){
		i=0;
		for(w=0;w<vp->height;w+=step){
			for(q=0;q<vp->width;q+=step){
				if(q && abs(normalizers[i]->lumamax-normalizers[i-1]->lumamax)>mdiff){
					if(normalizers[i]->lumamax>normalizers[i-1]->lumamax){
						normalizers[i-1]->lumamax=normalizers[i]->lumamax-mdiff;
					} else {
						normalizers[i]->lumamax=normalizers[i-1]->lumamax-mdiff;
					}
				}
				i++;
			}
		}
	}

	for(e=0;e<10;e++){
		i=0;
		for(w=0;w<vp->height;w+=step){
			for(q=0;q<vp->width;q+=step){
				if(w && abs(normalizers[i]->lumamax-normalizers[i-h_count]->lumamax)>mdiff){
					if(normalizers[i]->lumamax>normalizers[i-h_count]->lumamax){
						normalizers[i-h_count]->lumamax=normalizers[i]->lumamax-mdiff;
					} else {
						normalizers[i]->lumamax=normalizers[i-h_count]->lumamax-mdiff;
					}
				}

				i++;
			}
		}
	}




	/*
	// remove "pixels"
	int i2;
	int e,r;
	int ii;
	for(ii=0;ii<5;ii++){
	for(w=0;w<vp->height;w+=step){
	for(q=0;q<vp->width;q+=step){
	i=q+w*ll;
	if(i<0 || i>=count){continue;}
	if(normalizers[i]->lumamax-normalizers[i]->lumamin>30){
	for(e=-1;e<2;e++){
	for(r=-1;r<2;r++){
	i2=q+e+(w+r)*ll;
	if(i2<0 || i2>=count){continue;}
	if(abs(normalizers[i]->lumamax-normalizers[i2]->lumamax)>30){
	normalizers[i2]->lumamax=normalizers[i]->lumamax;
	}
	if(abs(normalizers[i]->lumamin-normalizers[i2]->lumamin)>30){
	normalizers[i2]->lumamin=normalizers[i]->lumamin;
	}}}}}}}*/


	i=0;
	for(w=0;w<v_count;w++){
		for(q=0;q<h_count;q++){
			i=q+w*h_count;
			if(q+1<h_count && w+1<v_count){
				do_norm_do(q*step,w*step,step,step,vp,&avFrame,normalizers[i],normalizers[i+1],normalizers[i+h_count],normalizers[i+h_count+1]);                         
			} else if(q+1<h_count){
				do_norm_do(q*step,w*step,step,step,vp,&avFrame,normalizers[i],normalizers[i+1],normalizers[i],normalizers[i+1]);                         
			} else if(w+1<v_count){
				do_norm_do(q*step,w*step,step,step,vp,&avFrame,normalizers[i],normalizers[i],normalizers[i+h_count],normalizers[i+h_count]);                         
			} else {
				do_norm_do(q*step,w*step,step,step,vp,&avFrame,normalizers[i],normalizers[i],normalizers[i],normalizers[i]);                         
			}
			i++;
		}
	}
	

	int posY=0;

	// if(vsrat_osd){
	// char *time=vsrat_get_time(is,vp);
	// podpisat(&pict,vp,time,posY);
	// posY+=30;
	// }
}

