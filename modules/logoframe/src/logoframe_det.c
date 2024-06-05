// logo detection of logoframe by Yobi
//
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.


typedef unsigned char BYTE;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include <stdint.h>
#include "logo.h"
#include "logoset.h"

//--- 透過ロゴ変換式 ---
#define ConvLogo(d,dp_y,y)    (((d) * (LOGO_MAX_DP - (dp_y)) + (y) * (dp_y) + LOGO_MAX_DP/2) / LOGO_MAX_DP)
#define ConvInvLogo(d,dp_y,y) (((d) * LOGO_MAX_DP - (y) * (dp_y) + (LOGO_MAX_DP - (dp_y))/2) / (LOGO_MAX_DP - (dp_y)))


//=====================================================================
// Public関数：起動時に１回だけ呼び出す
// ロゴデータの初期設定
//=====================================================================
void LogoInit(LOGO_DATASET *pl){
	pl->readdat.ptr = NULL;
	pl->framedat.num_frames = 0;

	pl->paramdat.dp_y = NULL;
	pl->paramdat.y = NULL;
	pl->paramdat.dif_y_col = NULL;
	pl->paramdat.dif_y_row = NULL;
	pl->paramdat.area_y    = NULL;
	pl->framedat.rate_logoon = NULL;
	pl->framedat.rate_fade = NULL;
	pl->framedat.flag_nosample = NULL;
	pl->framedat.flag_nofadesm = NULL;
	pl->framedat.rate_logooni1 = NULL;
	pl->framedat.rate_logooni2 = NULL;
	pl->framedat.rate_fadei1   = NULL;
	pl->framedat.rate_fadei2   = NULL;
	pl->framedat.rank_unclear  = NULL;
	pl->framedat.flag_logoon   = NULL;

	// initial threshold
	pl->thresdat.rate_th_logo    = DEF_LOGO_RATE_TH_LOGO;
	pl->thresdat.num_fadein      = DEF_LOGO_NUM_FADEIN;
	pl->thresdat.num_fadeout     = DEF_LOGO_NUM_FADEOUT;
	pl->thresdat.num_cutleft     = DEF_LOGO_NUM_CUTLEFT;
	pl->thresdat.num_cutright    = DEF_LOGO_NUM_CUTRIGHT;
	pl->thresdat.num_onwidth     = DEF_LOGO_NUM_ONWIDTH;
	pl->thresdat.num_onlevel     = DEF_LOGO_NUM_ONLEVEL;
	pl->thresdat.num_offwidth    = DEF_LOGO_NUM_OFFWIDTH;
	pl->thresdat.thres_ymax      = DEF_LOGO_THRES_YMAX;
	pl->thresdat.thres_ymin      = DEF_LOGO_THRES_YMIN;
	pl->thresdat.thres_yedge     = DEF_LOGO_THRES_YEDGE;
	pl->thresdat.thres_ydif      = DEF_LOGO_THRES_YDIF;
	pl->thresdat.thres_ysetdif   = DEF_LOGO_THRES_YSETDIF;
	pl->thresdat.thres_yoffedg   = DEF_LOGO_THRES_YOFFEDG;
	pl->thresdat.num_areaset     = DEF_LOGO_NUM_AREASET;
	pl->thresdat.num_clrrate     = DEF_LOGO_NUM_CLRRATE;
	pl->thresdat.auto_fade       = DEF_LOGO_AUTO_FADE;
	pl->thresdat.auto_bs11       = 1;
}


//---------------------------------------------------------------------
// メモリ解放処理（ロゴ自体のデータ）
//---------------------------------------------------------------------
void LogoFree_read(LOGO_DATASET *pl){
	if (pl->readdat.ptr)
		free(pl->readdat.ptr);
	if (pl->paramdat.dp_y)
		free(pl->paramdat.dp_y);
	if (pl->paramdat.y)
		free(pl->paramdat.y);
	if (pl->paramdat.dif_y_col)
		free(pl->paramdat.dif_y_col);
	if (pl->paramdat.dif_y_row)
		free(pl->paramdat.dif_y_row);
	if (pl->paramdat.area_y)
		free(pl->paramdat.area_y);

	pl->readdat.ptr = NULL;
}

//---------------------------------------------------------------------
// メモリ解放処理（ロゴフレーム処理データ）
//---------------------------------------------------------------------
void LogoFree_frame(LOGO_DATASET *pl){
	if (pl->framedat.rate_logoon)
		free(pl->framedat.rate_logoon);
	if (pl->framedat.rate_fade)
		free(pl->framedat.rate_fade);
	if (pl->framedat.flag_nosample)
		free(pl->framedat.flag_nosample);
	if (pl->framedat.flag_nofadesm)
		free(pl->framedat.flag_nofadesm);
	if (pl->framedat.rate_logooni1)
		free(pl->framedat.rate_logooni1);
	if (pl->framedat.rate_logooni2)
		free(pl->framedat.rate_logooni2);
	if (pl->framedat.rate_fadei1)
		free(pl->framedat.rate_fadei1);
	if (pl->framedat.rate_fadei2)
		free(pl->framedat.rate_fadei2);
	if (pl->framedat.rank_unclear)
		free(pl->framedat.rank_unclear);
	if (pl->framedat.flag_logoon)
		free(pl->framedat.flag_logoon);

	pl->framedat.num_frames = 0;
}

//=====================================================================
// Public関数：終了時に１回だけ呼び出す
// 確保したメモリの解放
//=====================================================================
void LogoFree(LOGO_DATASET *pl){
	LogoFree_read(pl);
	LogoFree_frame(pl);
}


//---------------------------------------------------------------------
// ロゴ設定−ロゴデータ読み出し
// ロゴデータをファイルから読み込み
//   logofname : ロゴデータ(.lgd)のファイル名
//---------------------------------------------------------------------
int LogoRead_file(const char *logofname, LOGO_READREC *plogor)
{
	FILE* fprd;
	LOGO_FILE_HEADER lfh;
	LOGO_HEADER lgh;
	LOGO_PIXEL  *ptr;
	long readed;
	LOGO_HEADER_V02 lgh_v02;

	fprd = fopen(logofname, "rb");
	if (!fprd){
		fprintf(stderr, "error: failed to create/open '%s'\n", logofname);
		return 2;
	}

	// get logo file
	readed = fread(&lfh, sizeof(LOGO_FILE_HEADER), 1, fprd);
	if (readed < 1){
		fprintf(stderr, "error: failed in reading logofile \n");
		fclose(fprd);
		return 2;
	}
	// check version
	int ver;
	if (strncmp(lfh.str, "<logo data file ver0.1", 22) == 0){
		ver = 1;
	}
	else if (strncmp(lfh.str, "<logo data file ver0.2", 22) == 0){
		ver = 2;
	}
	else{
		fprintf(stderr, "error: failed logo header check '%s'\n", logofname);
		fclose(fprd);
		return 2;
	}

	// get logo header
	readed = 0;
	if (ver == 1){
		readed = fread(&lgh, sizeof(LOGO_HEADER), 1, fprd);
	}
	else if (ver == 2){			// add for ver 0.2
		readed = fread(&lgh_v02, sizeof(LOGO_HEADER_V02), 1, fprd);
		// convert
		memset(&lgh,0,sizeof(LOGO_HEADER));
		memcpy(&(lgh.x),&(lgh_v02.x),sizeof(short)*8);
	}
	if (readed < 1){
		fprintf(stderr, "error: failed in reading logoheader \n");
		fclose(fprd);
		return 2;
	}

	// allocate memory
	ptr = (LOGO_PIXEL *)malloc(LOGO_PIXELSIZE(&lgh));
	if (ptr == NULL){
		fprintf(stderr, "error: failed in memory allocation.\n");
		fclose(fprd);
		return 2;
	}

	// read data
	memset(ptr,0,LOGO_PIXELSIZE(&lgh));
	readed = fread(ptr, 1, LOGO_PIXELSIZE(&lgh), fprd);

	// set data
	memcpy(&(plogor->lfh), &lfh, sizeof(LOGO_FILE_HEADER));
	memcpy(&(plogor->lgh), &lgh, sizeof(LOGO_HEADER));
	plogor->ptr = ptr;

	fclose(fprd);
	return 0;
}


//---------------------------------------------------------------------
// ロゴ設定−ロゴデータ確認−BS11判別
// ロゴデータがBS11であるか確認（11部分の大きさ・形で判断）
// 返り値：0=BS11以外  1=BS11
//---------------------------------------------------------------------
int LogoRead_Chk_BS11(const LOGO_READREC *plogor){
	int retval;
	int x, y;
	int dp_y, flag_dp_y;
	int state_row, state_col;
	int row_st, row_ed, col_ank;
	int ytop, flag_row;
	int loc_dp;
	int ncnt;
	int logo_width, logo_height;


	// set fix parameter
	logo_width  = plogor->lgh.w;
	logo_height = plogor->lgh.h;

	state_col = 0;
	col_ank = 0;
	ytop = -1;
	for(x=0; x<logo_width; x++){
		row_st = 0;
		row_ed = 0;
		state_row = 0;
		ncnt = 0;
		loc_dp = x;
		for(y=0; y<logo_height; y++){
			dp_y   = plogor->ptr[loc_dp].dp_y;
			loc_dp += logo_width;
			flag_dp_y = (dp_y >= LOGO_MAX_DP * 10 / 100)? 1 : 0;
			//--- check row state ---
			if (state_row == 0){			// search start point
				if (flag_dp_y > 0){
					ncnt ++;
					if (ncnt >= 3){
						state_row = 1;
						row_st = y - ncnt + 1;
						ncnt = 0;
					}
				}
				else{
					ncnt = 0;
				}
			}
			else if (state_row == 1){		// search end point
				if (flag_dp_y > 0){
					ncnt = 0;
				}
				else{
					ncnt ++;
					if (ncnt >= 3){
						state_row = 2;
						row_ed = y - ncnt;
						ncnt = 0;
					}
				}
			}
			else if (state_row == 2){		// after end point
				if (flag_dp_y > 0){
					ncnt ++;
					if (ncnt >= 3){
						state_row = 9;
					}
				}
				else{
					ncnt = 0;
				}
			}
		}
		//--- check line length ---
		if (state_row == 1){				// not found end point
			state_row = 2;
			row_ed = y - ncnt - 1;
		}
		if (state_row == 0){
			flag_row = 1;					// blank line
		}
		else if (state_row == 2){			// get line
			if (ytop < 0){
				ytop = row_st;
			}
			if (abs(row_ed - row_st - 11) < 5 && abs(row_st - ytop) < 5){
				flag_row = 2;				// short line
			}
			else if (abs(row_ed - row_st - 50) < 5 && abs(row_st - ytop) < 5){
				flag_row = 3;				// long line
			}
			else{
				flag_row = 0;				// other line
			}
		}
		else{
			flag_row = 0;					// other line
		}
		//--- check column state ---
		if (state_col == 0){				// before detection
			ytop = -1;
			if (flag_row == 2){
				state_col = 1;
			}
		}
		else if (state_col == 1){			// 1st 1 short
			if (flag_row == 3 || flag_row == 0){
				state_col = 2;
				col_ank = x;
			}
			else if (flag_row != 2){
				state_col = 0;
			}
		}
		else if (state_col == 2){			// 1st1 long
			if (flag_row == 1 || flag_row == 0){
				if (abs(x - col_ank - 14) < 3){
					state_col = 3;
				}
				else{
					state_col = 0;
				}
			}
			else if (flag_row != 3){
				state_col = 0;
			}
		}
		else if (state_col == 3){			// blank between 11
			if (flag_row == 2 || flag_row == 0){
				state_col = 4;
			}
			else if (flag_row != 1){
				state_col = 9;
			}
		}
		else if (state_col == 4){			// 2nd1 short
			if (flag_row == 3 || flag_row == 0){
				if (abs(x - col_ank - 28) < 3){
					state_col = 5;
					col_ank = x;
				}
				else{
					state_col = 9;
				}
			}
			else if (flag_row != 2){
				state_col = 9;
			}
		}
		else if (state_col == 5){			// 2nd1 long
			if (flag_row == 1 || flag_row == 0){
				if (abs(x - col_ank - 13) < 3){
					state_col = 6;
				}
				else{
					state_col = 9;
				}
			}
			else if (flag_row != 3){
				state_col = 9;
			}
		}
		else if (state_col == 6){			// after 11
			if (flag_row != 1){
				state_col = 9;
			}
		}
//		printf("%d:%d ", x, state_col);
	}

	retval = (state_col == 6)? 1 : 0;
	return retval;
}


//---------------------------------------------------------------------
// ロゴ設定−ロゴデータ確認
// ロゴデータ内容を確認し、自動検出によるパラメータを設定
//---------------------------------------------------------------------
void LogoRead_Chk(LOGO_THRESREC *plogot, const LOGO_READREC *plogor){
	int flag_bs11;

	if (plogot->auto_bs11 > 0){
		flag_bs11 = LogoRead_Chk_BS11(plogor);
		if (flag_bs11 > 0){
			plogot->auto_bs11 = 2;
			plogot->thres_yedge = DEF_LOGO_AUTO_YEDGE;
		}
		else{
			plogot->auto_bs11 = -1;
		}
	}

}


//---------------------------------------------------------------------
// ロゴ設定−ロゴデータメモリ確保
// ロゴデータを内部計算用に変換し、変数領域を確保して格納する
//---------------------------------------------------------------------
int LogoSet_mem(LOGO_PARAMREC *plogop, const LOGO_READREC *plogor, const LOGO_THRESREC *plogot){
	long total_y;
	int i;
	int ymax, ymin;
	int y, val, val_ymax, loc_ymax;
	int ncount, loc_dp;
	long num_total, num_total_tmp, num;
	long *histdp;
	long hist[256];

	// set y param
	plogop->yx = plogor->lgh.x;
	plogop->yy = plogor->lgh.y;
	plogop->yh = plogor->lgh.h;
	plogop->yw = plogor->lgh.w;

	// allocate memory
	total_y = plogop->yh * plogop->yw;
	plogop->dp_y  = (short *)malloc( sizeof(short) * total_y );
	plogop->y     = (short *)malloc( sizeof(short) * total_y );
	plogop->dif_y_col  = (char *)malloc( sizeof(char)  * total_y );
	plogop->dif_y_row  = (char *)malloc( sizeof(char)  * total_y );
	plogop->area_y     = (short *)malloc( sizeof(short)  * total_y );
	if ((plogop->dp_y  == NULL) || (plogop->y  == NULL) ||
		(plogop->dif_y_col  == NULL) || (plogop->dif_y_row  == NULL) ||
		(plogop->area_y == NULL)){
		fprintf(stderr, "error: failed in memory allocation.\n");
		return 2;
	}

	// set Y data
	// convert Y from (0,255) to (ymin,ymax)
	ymax = plogot->thres_ymax;
	ymin = plogot->thres_ymin;
	for(i=0; i<total_y; i++){
		plogop->dp_y[i] = plogor->ptr[i].dp_y;
		plogop->y[i]    = (plogor->ptr[i].y * (ymax-ymin) + (256/2)) / 256 + (ymin << 4);;
//if (plogop->y[i] < 200*16 && plogop->dp_y[i] > 50){printf("(%d,%d)",plogop->y[i]/16,plogop->dp_y[i]);}
	}


	// temporary allocate
	histdp = (long *)malloc( sizeof(long) * LOGO_MAX_DP );
	if (histdp == NULL){
		fprintf(stderr, "error: failed in memory allocation.\n");
		return 2;
	}

	// get DP_Y threshold level
	num_total = 0;
	num_total_tmp = 0;
	for(i=0; i<LOGO_MAX_DP; i++){
		histdp[i] = 0;
	}
	for(i=0; i<total_y; i++){
		if (plogop->dp_y[i] > (LOGO_MAX_DP * 7 / 100) &&
			plogop->dp_y[i] < (LOGO_MAX_DP * 85 / 100)){
			histdp[plogop->dp_y[i]] ++;
			num_total_tmp ++;
			if (plogop->dp_y[i] > (LOGO_MAX_DP * 15 / 100)){	// 従来設定
				num_total ++;
			}
		}
	}
//	printf("[%ld %ld]\n", num_total, num_total_tmp);
	if (num_total < 500){						// ロゴ割合が低い場合は差し替え
		num_total = num_total_tmp;
	}
	num = 0;
	loc_dp = (LOGO_MAX_DP * 10 / 100);				// default 10%
	ncount = LOGO_MAX_DP * 3 / 100;					// extra 3% from 1/2
	i = LOGO_MAX_DP * 85 / 100;						// start from 85%
	while(ncount > 0 && i > LOGO_MAX_DP * 7 / 100){
		num += histdp[i];
		if (num >= num_total/2 && num_total >= 200){	// update until 1/2+3%
			ncount --;
			loc_dp = i;
		}
		i --;
	}
	plogop->thres_dp_y = loc_dp;
//	printf("(%d %ld)\n", loc_dp, num);

	// get most point of logo Y-level
	for(i=0; i<=255; i++){
		hist[i] = 0;
	}
	for(i=0; i<total_y; i++){
		if (plogop->dp_y[i] >= loc_dp){
			y = (plogop->y[i] + (1 << 3)) >> 4;
			if (y < 0){
				y = 0;
			}
			else if (y > 255){
				y = 255;
			}
			hist[y] ++;
		}
	}
	loc_ymax  = 1;
	val_ymax  = 0;
	for(i=1; i<=254; i++){
		val = hist[i-1] + hist[i] + hist[i+1];
		if (val_ymax < val){
			val_ymax = val;
			loc_ymax = i;
		}
//		if (val > 0){
//			printf("%d:%d ", i, val);
//		}
	}
	plogop->most_logo_y = loc_ymax;

	free(histdp);

	return 0;
}


//---------------------------------------------------------------------
// ロゴ設定−エッジ検出用内部設定
// ロゴデータのエッジ検出に必要なパラメータ抽出
// 隣接点でロゴデータに差がある位置のみ検出対象として記憶する
//---------------------------------------------------------------------
int LogoSet_edge(LOGO_PARAMREC *plogop){
	int i, j;
	int interval;
	int total_dif;
	int total_dif_c1, total_dif_c2;
	int interlace;
	long logo_width, logo_height;
	long logo_numline;
	short dif_thres;
	short data0, data1;
	short *pt_dp0, *pt_dp1;
	short *pt_data0, *pt_data1;
	char *pt_dif;
	long d1, d2, d1a, d2a;
	char fdif;

	// set fix parameter
	logo_width  = plogop->yw;
	logo_height = plogop->yh;
	interval = 2;
	dif_thres = 30;
	if (plogop->thres_dp_y < LOGO_MAX_DP * 13 / 100){	// 輝度差が小さければ低くする
		dif_thres = 20;
	}

	dif_thres += 2;
	do{
		dif_thres -= 2;
		// detect logo column edge
		pt_dif = plogop->dif_y_col;
		total_dif = 0;
		total_dif_c1 = 0;
		total_dif_c2 = 0;
		logo_numline = 0;
		interlace = plogop->yy % 2;
		for(i=0; i<logo_height; i++){
			pt_dp0   = &(plogop->dp_y[logo_numline]);
			pt_dp1   = &(plogop->dp_y[logo_numline + interval]);
			pt_data0 = &(plogop->y[logo_numline]);
			pt_data1 = &(plogop->y[logo_numline + interval]);
			for(j=0; j<logo_width; j++){
				if (j >= logo_width - interval){
					fdif = 0;
				}
				else{
					data0 = (*pt_data0);
					data1 = (*pt_data1);
					d1 = ConvLogo(       0, *pt_dp0, data0) - ConvLogo(       0, *pt_dp1, data1);
					d2 = ConvLogo(256 << 4, *pt_dp0, data0) - ConvLogo(256 << 4, *pt_dp1, data1);
					d1a = abs(d1);
					d2a = abs(d2);
					if ((d1a > (dif_thres << 4)) ||
						(d2a > (dif_thres << 4))){
						fdif = 1;
						total_dif ++;
						if (interlace == 0){
							total_dif_c1 ++;
						}
						else{
							total_dif_c2 ++;
						}
					}
					else{
						fdif = 0;
					}
				}
				*pt_dif = fdif;
				pt_dif ++;
				pt_dp0 ++;
				pt_dp1 ++;
				pt_data0 ++;
				pt_data1 ++;
			}
			logo_numline += logo_width;
			interlace = 1 - interlace;
		}

		// detect logo row edge
		pt_dif = plogop->dif_y_row;
		logo_numline = 0;
		interlace = plogop->yy % 2;
		for(i=0; i<logo_height; i++){
			if (i >= logo_height - interval){
				memset(pt_dif, 0, sizeof(char) * logo_width);
				pt_dif += logo_width;
			}
			else{
				pt_dp0   = &(plogop->dp_y[logo_numline]);
				pt_dp1   = &(plogop->dp_y[logo_numline + (logo_width * interval)]);
				pt_data0 = &(plogop->y[logo_numline]);
				pt_data1 = &(plogop->y[logo_numline + (logo_width * interval)]);
				for(j=0; j<logo_width; j++){
					data0 = (*pt_data0);
					data1 = (*pt_data1);
					d1 = ConvLogo(       0, *pt_dp0, data0) - ConvLogo(       0, *pt_dp1, data1);
					d2 = ConvLogo(256 << 4, *pt_dp0, data0) - ConvLogo(256 << 4, *pt_dp1, data1);
					d1a = abs(d1);
					d2a = abs(d2);
					if ((d1a > (dif_thres << 4)) ||
						(d2a > (dif_thres << 4))){
						fdif = 1;
						total_dif ++;
						if (interlace == 0){
							total_dif_c1 ++;
						}
						else{
							total_dif_c2 ++;
						}
					}
					else{
						fdif = 0;
					}
					*pt_dif = fdif;
					pt_dif ++;
					pt_dp0 ++;
					pt_dp1 ++;
					pt_data0 ++;
					pt_data1 ++;
				}
			}
			logo_numline += logo_width;
			interlace = 1 - interlace;
		}
	}while((total_dif_c1 < 100 || total_dif_c2 < 100) && dif_thres > 16);  // ver1.21
	plogop->total_dif = total_dif;
	plogop->total_dif_c1 = total_dif_c1;
	plogop->total_dif_c2 = total_dif_c2;

	return 0;
}


//---------------------------------------------------------------------
// ロゴ設定−エリア検出用内部設定−16x16エリア設定
// ロゴデータのエリア検出に必要なパラメータ抽出
// 16x16検出用の複数エリアを設定
//---------------------------------------------------------------------
int LogoSet_sumparam_16x16(LOGO_PARAMREC *plogop){
	int i;
	int interval;
	int num_area, cnt_area;
	int cnt_d, cnt_d1, cnt_d2;
	int btx, bty;
	int st_x, ed_x, st_y, ed_y;
	int x, y;
	int pos;
	int tmpval, tmpcnt;
	long logo_width, logo_height;
	short *pt_area;
	LOGO_PARAMAREAREC wk_area[24];


	// set fix parameter
	logo_width  = plogop->yw;
	logo_height = plogop->yh;
	interval = 2;

	// initialize
	num_area = 0;

	// get several 16x16 area
	for(bty = interval; bty < logo_height-interval-4; bty += 8){
		st_y = bty;
		ed_y = st_y + 15;
		if (ed_y >= logo_height-interval){
			tmpval = ed_y - (logo_height-interval) + 1;
			st_y -= tmpval;
			ed_y -= tmpval;
			if (st_y < interval){
				st_y = interval;
			}
		}
		for(btx = interval; btx < logo_width-interval-4; btx += 8){
			st_x = btx;
			ed_x = st_x + 15;
			if (ed_x >= logo_width-interval){
				tmpval = ed_x - (logo_width-interval) + 1;
				st_x -= tmpval;
				ed_x -= tmpval;
				if (st_x < interval){
					st_x = interval;
				}
			}
			// count
			cnt_d1 = 0;
			cnt_d2 = 0;
			for(y=st_y; y<=ed_y; y++){
				pt_area = &(plogop->area_y[y * logo_width]);
				pt_area += st_x;
				for(x=st_x; x<=ed_x; x++){
					if (*pt_area == 1){
						cnt_d1 ++;
					}
					else if (*pt_area == 2){
						cnt_d2 ++;
					}
					pt_area ++;
				}
			}
			if (cnt_d1 <= cnt_d2){
				cnt_d = cnt_d1;
			}
			else{
				cnt_d = cnt_d2;
			}

			// add data
			if (cnt_d > 4){
				pos = 0;
				tmpcnt = wk_area[pos].total_area;
				while( tmpcnt >= cnt_d && pos < num_area ){
					pos ++;
					if (pos < num_area){
						tmpcnt = wk_area[pos].total_area;
					}
				}
				if (pos < num_area){		// shift data
					for(i = num_area; i > pos; i--){
						if (i < 24){
							wk_area[i].total_area  = wk_area[i-1].total_area;
							wk_area[i].total_area1 = wk_area[i-1].total_area1;
							wk_area[i].total_area2 = wk_area[i-1].total_area2;
							wk_area[i].xmin        = wk_area[i-1].xmin;
							wk_area[i].xmax        = wk_area[i-1].xmax;
							wk_area[i].ymin        = wk_area[i-1].ymin;
							wk_area[i].ymax        = wk_area[i-1].ymax;
						}
					}
				}
//printf("(%d,%d,%d,%d)", num_area, pos, cnt_d1, cnt_d2);
				if (pos < 24){
					wk_area[pos].total_area  = cnt_d;
					wk_area[pos].total_area1 = cnt_d1;
					wk_area[pos].total_area2 = cnt_d2;
					wk_area[pos].xmin        = st_x;
					wk_area[pos].xmax        = ed_x;
					wk_area[pos].ymin        = st_y;
					wk_area[pos].ymax        = ed_y;
					if (num_area < 24){
						num_area ++;
					}
				}
			}
		}
	}

	// delete overlapped area
	cnt_area = num_area;
	pos = num_area-1;
	while(pos > 0 && cnt_area >= LOGO_AREANUM){
		st_x = wk_area[pos].xmin;
		st_y = wk_area[pos].ymin;
		i = pos-1;
		while(i >= 0){
			if (abs(wk_area[i].xmin - st_x) <= 8 && abs(wk_area[i].ymin - st_y) <= 8){
				wk_area[pos].total_area = 0;		// delete data
				cnt_area --;
				i = 0;			// loop end
			}
			i --;
		}
		pos --;
	}

//for(i=0;i<num_area;i++){
//	printf("(%d:%d,%d,%d)", i, (int)wk_area[i].xmin, (int)wk_area[i].ymin, (int)wk_area[i].total_area);
//}

	// set area data
	cnt_area = 0;
	pos = 0;
	for(i=1; i<LOGO_AREANUM; i++){
		if (pos < num_area){
			tmpcnt = wk_area[pos].total_area;
			while(tmpcnt == 0 && pos  < num_area){
				pos ++;
				if ( pos < num_area){
					tmpcnt = wk_area[pos].total_area;
				}
			}
		}
		if (pos < num_area){
			cnt_area = i;
			plogop->area[i].total_area  = wk_area[pos].total_area;
			plogop->area[i].total_area1 = wk_area[pos].total_area1;
			plogop->area[i].total_area2 = wk_area[pos].total_area2;
			plogop->area[i].xmin        = wk_area[pos].xmin;
			plogop->area[i].xmax        = wk_area[pos].xmax;
			plogop->area[i].ymin        = wk_area[pos].ymin;
			plogop->area[i].ymax        = wk_area[pos].ymax;
		}
		else{
			plogop->area[i].total_area  = 0;
			plogop->area[i].total_area1 = 0;
			plogop->area[i].total_area2 = 0;
			plogop->area[i].xmin        = 0;
			plogop->area[i].xmax        = 0;
			plogop->area[i].ymin        = 0;
			plogop->area[i].ymax        = 0;
		}
		pos ++;
	}

	return cnt_area;
}


//---------------------------------------------------------------------
// ロゴ設定−エリア検出用内部設定
// ロゴデータのエリア検出に必要なパラメータ抽出
// ロゴのない部分とロゴのある部分をそれぞれ認識する。エッジ部分は省く。
// ロゴのある部分は同じ輝度andロゴのない部分と差が大きいand数も多い部分だけ選択し、
// 48x48画素だけを検出対象として絞る。
//---------------------------------------------------------------------
int LogoSet_sumparam(LOGO_PARAMREC *plogop){
	int i, j, k;
	int interval;
	long logo_width, logo_height;
	long logo_numline;
	short *pt_dp0, *pt_data0;
	short *pt_val0, *pt_val1, *pt_val2, *pt_val3, *pt_val4, *pt_val5, *pt_val6;
	short *pt_area;
	short *newval_area;
	short hista[256];
	short val_invalid;
	int val0ch;
	int ylogo, ybase, ystep;
	int area_xmin, area_xmax, area_ymin, area_ymax;
	int area_xwid, area_ywid;
	int cnt_d, cnt_d1, cnt_d2, cnt_dmax;
	int st_x, ed_x, st_y, ed_y;
	int x, y;
	int total_area1, total_area2;
	int val_max, val_tmp, loc_max;
	int num_local;
	short farea;

	// set fix parameter
	logo_width  = plogop->yw;
	logo_height = plogop->yh;
	interval = 2;

	// alloc work area
	newval_area = (short *)malloc( sizeof(short) * logo_width * logo_height );
	if (newval_area == NULL){
		fprintf(stderr, "error: failed in memory allocation.\n");
		return 2;
	}

	// decide Y level to calculate
	ylogo = plogop->most_logo_y;
	if (ylogo > 180){
		ybase = ylogo - 40;
		ystep = 1;
	}
	else{
		ybase = ylogo + 40;
		ystep = -1;
	}

	// calc y data
	pt_dp0   = plogop->dp_y;
	pt_data0 = plogop->y;
	pt_val0  = newval_area;
	for(i=0; i<logo_height; i++){
		for(j=0; j<logo_width; j++){
			*pt_val0 = ConvLogo(ybase << 4, *pt_dp0, *pt_data0);
			pt_val0 ++;
			pt_dp0 ++;
			pt_data0 ++;
		}
	}
	for(i=0; i<256; i++){
		hista[i] = 0;
	}
	area_xmin = logo_width;
	area_xmax = 0;
	area_ymin = logo_height;
	area_ymax = 0;
	total_area1 = 0;
	total_area2 = 0;

	// detect logo area
	pt_dp0   = plogop->dp_y;
	pt_data0 = plogop->y;
	pt_area = plogop->area_y;
	logo_numline = 0;
	val_invalid = -100;
	for(i=0; i<logo_height; i++){
		if (i < interval){
			pt_val1 = &(newval_area[logo_numline]);
			pt_val2 = &(newval_area[logo_numline + (logo_width * interval)]);
		}
		else if (i >= logo_height - interval){
			pt_val1 = &(newval_area[logo_numline - (logo_width * interval)]);
			pt_val2 = &(newval_area[logo_numline]);
		}
		else{
			pt_val1 = &(newval_area[logo_numline - (logo_width * interval)]);
			pt_val2 = &(newval_area[logo_numline + (logo_width * interval)]);
		}
		pt_val0 = &(newval_area[logo_numline]);
		for(j=0; j<logo_width; j++){
			if (j < interval){
				pt_val3 = &(val_invalid);
				pt_val4 = pt_val0 + interval;
			}
			else if (j >= logo_width - interval){
				pt_val3 = pt_val0 - interval;
				pt_val4 = &(val_invalid);
			}
			else{
				pt_val3 = pt_val0 - interval;
				pt_val4 = pt_val0 + interval;
			}
			if (j < 1){
				pt_val5 = &(val_invalid);
				pt_val6 = pt_val0 + 1;
			}
			else if (j >= logo_width){
				pt_val5 = pt_val0 - 1;
				pt_val6 = &(val_invalid);
			}
			else{
				pt_val5 = pt_val0 - 1;
				pt_val6 = pt_val0 + 1;
			}
			if ((abs(*pt_val0 - *pt_val1) < (2 << 4)) ||
				(abs(*pt_val0 - *pt_val2) < (2 << 4)) ||
				(abs(*pt_val0 - *pt_val3) < (2 << 4)) ||
				(abs(*pt_val0 - *pt_val4) < (2 << 4)) ||
				((abs(*pt_val0 - *pt_val5) < (2 << 4)) &&
				 (abs(*pt_val0 - *pt_val6) < (2 << 4))) ){
				if (abs(*pt_val0 - (ybase << 4)) < (1 << 3) && *pt_dp0 <= LOGO_MAX_DP/200){
					farea = 1;
					total_area1 ++;
				}
				else if (abs(*pt_data0 - (ylogo << 4)) <= (5 << 4) ||	// within +-5
						 (*pt_data0 >= (255 << 4) && ylogo >= 254)){
					farea = 2;
					total_area2 ++;
					val0ch = (*pt_val0 + (1 << 3)) / (1 << 4);
					if (val0ch < 0){
						val0ch = 0;
					}
					else if (val0ch > 255){
						val0ch = 255;
					}
					hista[val0ch] ++;
				}
				else{
					farea = 0;
				}
			}
			else{
				farea = 0;
			}
			*pt_area = farea;
			pt_area ++;
			pt_val0 ++;
			pt_val1 ++;
			pt_val2 ++;
			pt_dp0  ++;
			pt_data0 ++;
			if (farea == 0 || farea == 2){
				if (area_xmin > j){
					area_xmin = j;
				}
				if (area_xmax < j){
					area_xmax = j;
				}
				if (area_ymin > i){
					area_ymin = i;
				}
				if (area_ymax < i){
					area_ymax = i;
				}
			}
		}
		logo_numline += logo_width;
	}

	// limit area2 in same level point
	val_max = 0;
	loc_max = 0;
	for( i = ybase + ystep*3; i != ylogo + ystep; i += ystep){
		val_tmp = hista[i-1] + hista[i] + hista[i+1];
		if ((val_tmp >= 80 && val_max <= val_tmp * 2) ||
			(val_tmp <= 80 && val_max * 9 <= val_tmp * 10)){
			loc_max = i;
			if (val_max <= val_tmp){
				if (abs(i - ybase) < 5){
					val_max = val_tmp / (6 - abs(i - ybase));
				}
				else{
					val_max = val_tmp;
				}
			}
		}
//		if (hista[i] > 0){
//			printf("@%d:%d",i,hista[i]);
//		}
	}
	pt_val0 = newval_area;
	pt_area = plogop->area_y;
	for(i=0; i<logo_height; i++){
		for(j=0; j<logo_width; j++){
			if (*pt_area == 2){
				if (*pt_val0 < ((loc_max - 1) << 4) - (1 << 3) ||
					(*pt_val0 >= ((loc_max + 1) << 4) + (1 << 3) && loc_max < 254)){
					*pt_area = 0;
					total_area2 --;
				}
			}
			pt_val0 ++;
			pt_area ++;
		}
	}

//printf("{%d,%d}", total_area1, total_area2);
	// limit area to 48x48
	if (area_xmin > 0){
		area_xmin --;
	}
	if (area_xmax < logo_width - 1){
		area_xmax ++;
	}
	if (area_ymin > 0){
		area_ymin --;
	}
	if (area_ymax < logo_height - 1){
		area_ymax ++;
	}
	area_ywid = area_ymax - area_ymin + 1;
	area_xwid = area_xmax - area_xmin + 1;
	cnt_dmax = -1;
	for(i=0; i<5; i++){		// search best area from 5x5 point(by i,j)
		if (i == 2 || area_ywid >= 48+8){
			if (area_ywid > 48){
				st_y = (area_ywid - 48) * i / 4 + area_ymin;
				ed_y = st_y + 48 - 1;
			}
			else{
				st_y = area_ymin;
				ed_y = area_ymax;
			}
			for(j=0; j<5; j++){
				if (j == 2 || area_xwid >= 48+8){
					if (area_xwid > 48){
						st_x = (area_xwid - 48) * j / 4 + area_xmin;
						ed_x = st_x + 48 - 1;
					}
					else{
						st_x = area_xmin;
						ed_x = area_xmax;
					}
					cnt_d1 = 0;
					cnt_d2 = 0;
					for(y=st_y; y<=ed_y; y++){
						pt_area = &(plogop->area_y[y * logo_width]);
						pt_area += st_x;
						for(x=st_x; x<=ed_x; x++){
							if (*pt_area == 1){
								cnt_d1 ++;
							}
							else if (*pt_area == 2){
								cnt_d2 ++;
							}
							pt_area ++;
						}
					}
					if (cnt_d1 <= cnt_d2){
						cnt_d = cnt_d1;
					}
					else{
						cnt_d = cnt_d2;
					}
//printf("{%d,%d}", cnt_d1, cnt_d2);
					if (cnt_d > cnt_dmax){		// select as use area
						cnt_dmax = cnt_d;
						plogop->area[0].total_area  = cnt_d;
						plogop->area[0].total_area1 = cnt_d1;
						plogop->area[0].total_area2 = cnt_d2;
						plogop->area[0].xmin        = st_x;
						plogop->area[0].xmax        = ed_x;
						plogop->area[0].ymin        = st_y;
						plogop->area[0].ymax        = ed_y;
					}
				}
			}
		}
	}
//printf("{%d,%d}", (int)plogop->area[0].total_area1, (int)plogop->area[0].total_area2);
	

	// set 16x16 area
	num_local = LogoSet_sumparam_16x16(plogop);

	// set select 
	for(i=0; i<logo_height; i++){
		pt_area = &(plogop->area_y[i * logo_width]);
		for(j=0; j<logo_width; j++){
			farea = *pt_area;
			if ((farea & 0x3) != 0){
				for(k=0; k<=num_local; k++){
					if (j >= plogop->area[k].xmin && j <= plogop->area[k].xmax &&
						i >= plogop->area[k].ymin && i <= plogop->area[k].ymax){
						*pt_area |= (1 << (k+8));
//if ((farea & 0x3)==2 && (farea & 0x100)==0) printf("(%d %x)", k,*pt_area);
					}
				}
				if (((*pt_area) & 0xFF00) == 0){		// not select any area
					*pt_area = 0;
				}
			}
			pt_area ++;
		}
	}

//for(i=0;i<=6;i++){
//	printf("[%d](%d,%d)-(%d,%d)%d/%d", i,
//			(int)plogop->area[i].xmin, (int)plogop->area[i].ymin,
//			(int)plogop->area[i].xmax, (int)plogop->area[i].ymax, 
//			(int)plogop->area[i].total_area1, (int)plogop->area[i].total_area2);
//}

	// finish
	free(newval_area);

	plogop->num_local   = num_local;
	plogop->scale_area2 = loc_max;

	return 0;
}


//---------------------------------------------------------------------
// ロゴ設定−パラメータ補正
// ロゴデータ読み込み後に実行するパラメータ補正
//---------------------------------------------------------------------
int LogoSet_revparam(LOGO_THRESREC *plogot, const LOGO_PARAMREC *plogop){
	int data_src, data_dst, data_dif;
	int data_base;

	// revise ydif by ysetdif
	data_src  = plogot->thres_ysetdif * 16;
	data_base = plogop->most_logo_y * 16;
	if (data_src > 0){
		data_dst = ConvLogo(data_src, plogop->thres_dp_y, data_base);
		data_dif = abs(data_src - data_dst);
//printf("{%d %d %d}", data_src, data_dst, data_dif);
		if (data_dif < DEF_LOGO_AUTO_YDIFMIN){
			data_dif = DEF_LOGO_AUTO_YDIFMIN;
		}
		if (plogot->thres_ydif > data_dif){
			plogot->thres_ydif = data_dif;
		}
	}
	return 0;
}


//=====================================================================
// Public関数：ロゴデータ内容を読み込むため呼び出す
// ロゴデータを読み込む
//=====================================================================
int LogoRead(const char *logofname, LOGO_DATASET *pl){
	LOGO_PARAMREC *plogop = &(pl->paramdat);
	LOGO_READREC  *plogor = &(pl->readdat);
	LOGO_THRESREC *plogot = &(pl->thresdat);
	int ret;

	if (plogor->ptr != NULL){
		LogoFree_read(pl);
	}

	ret = LogoRead_file(logofname, plogor);
	if (ret == 0){
		LogoRead_Chk(plogot, plogor);
		ret = LogoSet_mem(plogop, plogor, plogot);
	}
	if (ret == 0){
		ret = LogoSet_edge(plogop);
	}
	if (ret == 0){
		ret = LogoSet_sumparam(plogop);
	}
	if (ret == 0){
		ret = LogoSet_revparam(plogot, plogop);
	}

	return ret;
}



//=====================================================================
// Public関数：全体のフレーム数が確定した時に１度だけ呼び出す
// フレーム毎ロゴデータ格納用メモリ初期化
//=====================================================================
int LogoFrameInit(LOGO_DATASET *pl, long num_frames){
	LOGO_FRAMEREC *plogof = &(pl->framedat);

	if (plogof->num_frames != 0){
		LogoFree_frame(pl);
	}

	plogof->num_frames    = num_frames;
	plogof->rate_logoon   = (char *)malloc( sizeof(char) * num_frames );
	plogof->rate_fade     = (char *)malloc( sizeof(char) * num_frames );
	plogof->flag_nosample = (char *)malloc( sizeof(char)  * num_frames );
	plogof->flag_nofadesm = (char *)malloc( sizeof(char)  * num_frames );
	plogof->rate_logooni1 = (char *)malloc( sizeof(char) * num_frames );
	plogof->rate_logooni2 = (char *)malloc( sizeof(char) * num_frames );
	plogof->rate_fadei1   = (char *)malloc( sizeof(char) * num_frames );
	plogof->rate_fadei2   = (char *)malloc( sizeof(char) * num_frames );
	plogof->rank_unclear  = (char *)malloc( sizeof(char)  * num_frames );
	plogof->flag_logoon   = (char *)malloc( sizeof(char)  * num_frames );
	if ((plogof->rate_logoon   == NULL) || (plogof->rate_fade     == NULL) ||
		(plogof->flag_nosample == NULL) || (plogof->flag_nofadesm == NULL) ||
		(plogof->rate_logooni1 == NULL) || (plogof->rate_logooni2 == NULL) ||
		(plogof->rate_fadei1   == NULL) || (plogof->rate_fadei2   == NULL) ||
		(plogof->rank_unclear  == NULL) || (plogof->flag_logoon   == NULL)){
		fprintf(stderr, "error: failed in memory allocation.\n");
		return 2;
	}
	memset( plogof->rate_logoon,   0, sizeof(char) * num_frames );
	memset( plogof->rate_fade,     0, sizeof(char) * num_frames );
	memset( plogof->flag_nosample, 0, sizeof(char) * num_frames );
	memset( plogof->flag_nofadesm, 0, sizeof(char) * num_frames );
	memset( plogof->rate_logooni1, 0, sizeof(char) * num_frames );
	memset( plogof->rate_logooni2, 0, sizeof(char) * num_frames );
	memset( plogof->rate_fadei1,   0, sizeof(char) * num_frames );
	memset( plogof->rate_fadei2,   0, sizeof(char) * num_frames );
	memset( plogof->rank_unclear,  0, sizeof(char) * num_frames );
	memset( plogof->flag_logoon,   0, sizeof(char) * num_frames );

	memset( plogof->workres,  0, sizeof(LOGO_OUTSUBREC) * LOGO_FIND_MAX );
	memset( plogof->res,      0, sizeof(LOGO_OUTSUBREC) * LOGO_FIND_MAX );

	return 0;
}

//---------------------------------------------------------------------
// １画像−変数初期化
// ロゴ検出計算領域の初期化
//---------------------------------------------------------------------
void LogoCalc_clear(LOGO_CALCREC *plogoc, short num_fadein, short num_fadeout){
//	int i;
	int num_fade;

	//--- initialize data ---
//	for(i=0; i<LOGO_FADE_MAXLEVEL; i++){
//		plogoc->hist_y[i] = 0;
//	}
//	plogoc->cnt_logooff = 0;
//	plogoc->cnt_logoon  = 0;
//	plogoc->cnt_logomv  = 0;
//	plogoc->cnt_logovc  = 0;
//
//	plogoc->cntf_logooff = 0;
//	plogoc->cntf_logoon  = 0;
//	plogoc->cntf_logost  = 0;
//	plogoc->cntf_logovc  = 0;
//
//	plogoc->sum_areaoff = 0;
//	plogoc->sum_areaon  = 0;
//	plogoc->sum_areadif = 0;
//	plogoc->sum_areanum = 0;
//
//  plogoc->cnt_offedg = 0;
//  plogoc->cntf_offedg = 0;
//	plogoc->cnts_logooff = 0;
//	plogoc->cnts_logoon  = 0;
//
//	for(i=0; i<256; i++){
//		plogoc->hista_areaoff[i] = 0;
//		plogoc->hista_areaon[i]  = 0;
//		plogoc->hista_areacal[i] = 0;
//	}
//	plogoc->num_hista_off = 0;
//	plogoc->num_hista_on  = 0;
	memset(plogoc, 0 , sizeof(LOGO_CALCREC));

	//--- set for fade histogram ---
	if (num_fadein >= num_fadeout){
		num_fade = num_fadein;
	}
	else{
		num_fade = num_fadeout;
	}
	if (num_fade == 0 || num_fade >= LOGO_FADE_STEP){
		num_fade = LOGO_FADE_STEP - 1;
	}
	else if (num_fade < 5){
		num_fade = num_fade * 2;
	}
	plogoc->fade_calcstep = num_fade + 1;

}


//---------------------------------------------------------------------
// １画像−演算−エッジ検出
// エッジ両側の２画素データからロゴ有無計算値を取得
//   data0, data1 : 画像データ
//   y0, y1       : ロゴデータの輝度値
//   dp_y0, dp_y1 : ロゴデータの割合（LOGO_MAX_DPの時100%）
//   thres_ymax   : この値を超える輝度時は予測演算を行わない
//   thres_ydege  : この値を超える輝度時はエッジ演算を行わない
//   thres_ydif   : この値/16以上の輝度差がロゴデータにあれば演算を行う。
//   thres_yoffedg: この値を超える輝度時はエッジ検出でロゴなし判定をしない
//   most_logo_y  : 最も多いロゴの輝度値（小振幅では離れた輝度では検出を行わない）
//   exe_type     : 実行タイプ（0:通常 1:画素値入れ替え）
//---------------------------------------------------------------------
int LogoCalc_getdif_pix(LOGO_CALCREC *plogoc, BYTE data0, BYTE data1,
						short dp_y0, short y0, short dp_y1, short y1,
						short thres_ymax, short thres_yedge, short thres_ydif,
						short thres_yoffedg,
						short most_logo_y, short exe_type){
	short d0_s;
	short d1, d1s, d1r, d1dif;
	long d0_src, d0_rev, d1_rev;
	long d1s_fine, d1r_fine, d1dif_fine;
	int val;
	int flag_chkopp, oppside;
	int val_offedg;
	int d0calc_adif, d1calc_adif;
	int d0calc_much, d1calc_much;
	int most_logo_adif_y0, most_logo_adif_y1;
	const int thres_dpysmin = 100;		// dp_y threshold for shadow of the logo

	// get data for calculate
	//   d0_src : scaled data0 when not use dp_y
	//   d0_rev : scaled data0 when use dp_y
	//   d1_rev : assumed scaled data1 when use dp_y and same Y as d0
	//   d1   : current data1
	//   d1s  : assumed data1 when not use dp_y (logo=off)
	//   d1r  : assumed data1 when use dp_y (logo=on)
	d0_s = ((short) data0) << 4;
	d0_src = (long) d0_s;
	d0_rev = ConvInvLogo(d0_s, dp_y0, y0);
	d1_rev = ConvLogo(d0_rev, dp_y1, y1);
	if (d0_rev > (255 << 4)){
		d0_rev = 255 << 4;
	}
	else if (d0_rev < (0 << 4)){
		d0_rev = 0 << 4;
	}
	if (d1_rev > (255 << 4)){
		d1_rev = 255 << 4;
	}
	else if (d1_rev < (0 << 4)){
		d1_rev = 0 << 4;
	}
	d1  = (short) data1;
	d1s = (short) data0;
	d1r = (short)((d1_rev + (1 << 3)) >> 4);

	if (d1 > thres_ymax){
		d1 = thres_ymax;
	}
	if (d1s > thres_ymax){
		d1s = thres_ymax;
	}
	if (d1r > thres_ymax){
		d1r = thres_ymax;
	}

	// decide if check opposite side
	flag_chkopp = 0;
	if ((d1 < d1s && d1s < d1r) || (d1 > d1s && d1s > d1r)){
		oppside = 1;
	}
	else{
		oppside = 0;
	}
	// add value for off edge
	if (data0 > thres_yoffedg || data1 > thres_yoffedg){
		val_offedg = 1;
	}
	else{
		val_offedg = 0;
	}
	// for ext-fine
	d1s_fine = (long) (d1s << 4);
	d1r_fine = d1_rev;
	if (d1r_fine > (thres_ymax << 4)){
		d1r_fine = (thres_ymax << 4);
	}
	d1dif_fine = d1r_fine - d1s_fine;

	// calculate
	d0calc_adif = abs(d0_src - d0_rev);
	d1calc_adif = abs(d0_rev - d1_rev);
	d0calc_much = (d0calc_adif * 2 >= d1calc_adif && d0calc_adif >= (1 << 4))? 1 : 0;
	d1calc_much = (d1calc_adif * 2 >= d0calc_adif && d1calc_adif >= (1 << 4))? 1 : 0;
	most_logo_adif_y0 = abs(most_logo_y - ((y0+8) >> 4));
	most_logo_adif_y1 = abs(most_logo_y - ((y1+8) >> 4));

	d1dif = d1r - d1s;
	if (data0 > thres_ymax && data1 > thres_ymax && d1r >= thres_ymax-1){
	}
	else if (data0 > thres_yedge && data1 > thres_yedge && d1r > thres_yedge-10){
	}
	else if (( abs(d1dif) < 10 ) &&
			 ((d0calc_adif >= (1 << 4) && most_logo_adif_y0 > 2) ||
			  (d1calc_adif >= (1 << 4) && most_logo_adif_y1 > 2))){
	}
	else if (( abs(d1dif) < 14 ) &&
			 ((dp_y0 < thres_dpysmin && d0calc_much > 0 && most_logo_adif_y0 > 2) ||
			  (dp_y1 < thres_dpysmin && d1calc_much > 0 && most_logo_adif_y1 > 2))){
	}
	else if (((abs(d1dif) < 14 || dp_y0 < thres_dpysmin) && d0calc_much > 0 && most_logo_adif_y0 > 32) ||
			 ((abs(d1dif) < 14 || dp_y1 < thres_dpysmin) && d1calc_much > 0 && most_logo_adif_y1 > 32)){
		if (exe_type == 0){
			if (thres_yoffedg >= 255){
				if (abs(d1 - d1s) < 4){
					plogoc->cnts_logooff += 1;
				}
				else if (abs(d1 - d1r) < 5){
					plogoc->cnts_logoon += 1;
				}
			}
		}
	}
	else if ((abs(d1 - d1s) < 4) && thres_yoffedg < 255 &&		// nologo
			 ((dp_y0 < thres_dpysmin && d0calc_adif >= (1 << 4) && most_logo_adif_y0 > 32) ||
			  (dp_y1 < thres_dpysmin && d0calc_adif >= (1 << 4) && most_logo_adif_y1 > 32))){
	}
//	else if (( abs(d1dif) < 12 ) &&
//			 ((abs(d0_src - d0_rev) >= (1 << 4) && abs(most_logo_y - ((y0+8) >> 4)) > 2) ||
//			  (abs(d0_rev - d1_rev) >= (1 << 4) && abs(most_logo_y - ((y1+8) >> 4)) > 2))){
//	}
//	else if (( abs(d1dif) < 14 ) &&
//			 ((abs(d0_src - d0_rev) >= (5 << 4) && abs(most_logo_y - ((y0+8) >> 4)) > 72) ||
//			  (abs(d0_rev - d1_rev) >= (5 << 4) && abs(most_logo_y - ((y1+8) >> 4)) > 72))){
//	}
//	else if ((abs(d1 - d1s) < 4) &&		// nologo
//			 ((dp_y0 < thres_dpysmin &&
//			   abs(d0_src - d0_rev) >= (1 << 4) && abs(most_logo_y - ((y0+8) >> 4)) > 36) ||
//			  (dp_y1 < thres_dpysmin &&
//			   abs(d0_rev - d1_rev) >= (1 << 4) && abs(most_logo_y - ((y1+8) >> 4)) > 36))){
//	}
	else if ( abs(d1dif) >= 10 ){
		if (exe_type == 0){
			val = ((d1 - d1s) * plogoc->fade_calcstep + d1dif/2) / d1dif + LOGO_FADE_OFST;
			if (val >= 0 && val < LOGO_FADE_MAXLEVEL){
				plogoc->hist_y[val] += 1;
			}

			if (abs(d1 - d1s) < 4){
				plogoc->cnt_logooff += 1;
				plogoc->cnt_offedg += val_offedg;
//if (dp_y0 < dp_y1)
//printf("[%d %d %d]", d1, d1s, d1r);
			}
			else if (abs(d1 - d1r) < 5){
				plogoc->cnt_logoon += 1;
			}
			else{
				plogoc->cnt_logomv += 1;
				if (oppside == 1){	// for opp
					flag_chkopp = 1;
				}
			}
		}
	}
	else{
		if ( abs(d1dif) >= 6 ){
			if (exe_type == 0){
				if (abs(d1 - d1s) < 3){
					plogoc->cntf_logooff += 1;
					plogoc->cntf_offedg += val_offedg;
//if (dp_y0 < dp_y1)
//printf("[%d %d %d, %ld %d %d %d %d]", d1, d1s, d1r, d0_rev/16, y0/16, dp_y0, y1/16, dp_y1);
				}
				else if (abs(d1 - d1r) < 3){
					plogoc->cntf_logoon += 1;
				}
				else if (oppside == 1){	// for opp
					flag_chkopp = 1;
				}
			}
			else if (exe_type == 1){
				if (abs(d1 - d1r) < 3){
					plogoc->cntopp_logoon += 1;
				}
			}
		}
		else if ( abs(d1dif_fine) >= thres_ydif && abs(d1dif_fine) >= 56 ){
			if (exe_type == 0){
				if (abs(d1 - d1s) <= 0 && abs(d1 - d1r) >= 3){
					plogoc->cntf_logooff += 1;
					plogoc->cntf_offedg += val_offedg;
//if (dp_y0 < dp_y1)
//printf("[a%d %d %d, %ld %d %d %d %d]", d1, d1s, d1r, d0_rev/16, y0/16, dp_y0, y1/16, dp_y1);
//printf("%d", most_logo_y);
				}
				else if (abs(d1 - d1s) >= 2 && abs(d1 - d1r) <= 3){
					plogoc->cntf_logoon += 1;
				}
				else if (oppside == 1){	// for opp
					flag_chkopp = 1;
				}
			}
			else if (exe_type == 1){
				if (abs(d1 - d1s) <= 0 && abs(d1 - d1r) >= 3){
				}
				else if (abs(d1 - d1s) >= 2 && abs(d1 - d1r) <= 3){
					plogoc->cntopp_logoon += 1;
				}
			}
		}
		else if ( abs(d1dif_fine) >= thres_ydif && thres_ydif > 0 ){
			if (exe_type == 0){
				if (abs(d1 - d1s) <= 0){
					plogoc->cntf_logooff += 1;
					plogoc->cntf_offedg += val_offedg;
				}
				else if ( abs(d1 - d1r) <= 1 ){
					plogoc->cntf_logoon += 1;
				}
				else if (oppside == 1){	// for opp
					flag_chkopp = 1;
				}
			}
			else if (exe_type == 1){
				if (abs(d1 - d1s) <= 0){
				}
				else if ( abs(d1 - d1r) <= 1 ){
					plogoc->cntopp_logoon += 1;
				}
			}
		}
		else{
			if (exe_type == 0){
				plogoc->cntf_logost += 1;
				if (oppside == 1){	// for opp
					flag_chkopp = 1;
				}
			}
		}

		// for fade
		if (exe_type == 0){
			if (dp_y0 < dp_y1){
				if ((d1_rev - d0_src) >= (3 << 4)){
					if (d1 > d1s - 4 && d1 < d1r + 4){
						plogoc->sum_areaoff += data0;
						plogoc->sum_areaon  += data1;
						plogoc->sum_areadif += (d1r - d1s);
						plogoc->sum_areanum ++;
					}
				}
			}
			else if (dp_y0 > dp_y1){
				if ((d0_src - d1_rev) >= (3 << 4)){
					if (d1 > d1r - 4 && d1 < d1s + 4){
						plogoc->sum_areaoff += data1;
						plogoc->sum_areaon  += data0;
						plogoc->sum_areadif += (d1s - d1r);
						plogoc->sum_areanum ++;
					}
				}
			}
		}
	}

	// check opposite side
	if (exe_type == 0 && flag_chkopp > 0){
		// exchange data0 and data1 for checking opposite side
		LogoCalc_getdif_pix(plogoc, data1, data0,
						dp_y0, y0, dp_y1, y1,
						thres_ymax, thres_yedge, thres_ydif, thres_dpysmin,
						most_logo_y, 1);		// exe_type = 1
	}

	return 0;
}


//---------------------------------------------------------------------
// １画像−演算−エリア検出
// エリア検出アルゴリズム用：画素データを領域別に記憶
//   data0   : 画像データ
//   y0      : ロゴデータの輝度値
//   dp_y0   : ロゴデータの割合（LOGO_MAX_DPの時100%）
//   area_y0 : 0:無効エリア 1:ロゴなし計算エリア 2:ロゴあり計算エリア
//   thres_ymax   : この値と最多ロゴ輝度値を超える輝度時は予測演算を行わない
//   most_logo_y  : 最も多いロゴの輝度値
//---------------------------------------------------------------------
int LogoCalc_getdif_areasum(LOGO_CALCREC *plogoc, BYTE data0,
							short dp_y0, short y0, short area_y0,
							short thres_ymax, short most_logo_y){
	int i;
	short d0_s;
	short d0r;
	long d0_rev;

	if ((area_y0 & 0x3) == 1){
		for(i=0; i<LOGO_AREANUM; i++){
			if ((area_y0 & (1 << (i+8))) != 0){
				plogoc->area[i].hista_areaoff[data0] ++;
				plogoc->area[i].num_hista_off ++;
			}
		}
	}
	else if ((area_y0 & 0x3) == 2){
		d0_s = ((short) data0) << 4;
		d0_rev = ConvInvLogo(d0_s, dp_y0, y0);
		if (d0_rev > (254 << 4)){
			d0_rev = 254 << 4;
		}
		else if (d0_rev < (1 << 4)){
			d0_rev = 1 << 4;
		}
		d0r = (d0_rev + (1 << 3)) / (1 << 4);
		// invalidate assumed level when Y-data > thres_ymax
		if (data0 > thres_ymax && data0 > most_logo_y){
			d0r = data0;
		}
		for(i=0; i<LOGO_AREANUM; i++){
			if ((area_y0 & (1 << (i+8))) != 0){
				plogoc->area[i].hista_areaon[data0] ++;
				plogoc->area[i].num_hista_on ++;
				plogoc->area[i].hista_areacal[d0r] ++;
			}
		}
//		printf("(%d:%x)", (int)plogoc->area[0].num_hista_on, (int)area_y0);
//		printf("(%d:%d,%d)", (int)plogoc->area[0].num_hista_on, (int)data0,(int)d0r);
	}
	return 0;
}


//---------------------------------------------------------------------
// １画像−演算−有効データ数測定
// １枚の画像データ分、各ラインの有効データ数測定
//   type         : 実行内容（0:初期化 1:実行前状態取得 2:実行後状態取得-垂直方向 3:実行後状態取得-水平方向）
//---------------------------------------------------------------------
int LogoCalc_getdif_vccalc(LOGO_VCCALCREC *pvc, const LOGO_CALCREC *plogoc, int type){
	long cntd_cur_on, cntd_dif_on;
	long cntd_cur_off, cntd_dif_off;
	long cntf_cur_on, cntf_dif_on;
	long cntf_cur_off, cntf_dif_off;

	if (type == 0){
		pvc->vc_cntd_logooff = 0;
		pvc->vc_cntd_logoon  = 0;
		pvc->vc_cntf_logooff = 0;
		pvc->vc_cntf_logoon  = 0;
		pvc->vc_cntd_voff    = 0;
		pvc->vc_cntd_von     = 0;
		pvc->vc_cntf_voff    = 0;
		pvc->vc_cntf_von     = 0;
	}
	if (type == 1){
		pvc->cntd_last_on  = plogoc->cnt_logoon;
		pvc->cntd_last_off = plogoc->cnt_logooff;
		pvc->cntf_last_on  = plogoc->cntf_logoon;
		pvc->cntf_last_off = plogoc->cntf_logooff;
	}
	if (type == 2 || type == 3){
		// set current data
		cntd_cur_on  = plogoc->cnt_logoon;
		cntd_cur_off = plogoc->cnt_logooff;
		cntf_cur_on  = plogoc->cntf_logoon;
		cntf_cur_off = plogoc->cntf_logooff;

		// for cnt_logovc
		cntd_dif_on  = cntd_cur_on - pvc->cntd_last_on;
		cntd_dif_off = cntd_cur_off - pvc->cntd_last_off;
		if (cntd_dif_on < 0){
			cntd_dif_on = 0;
		}
		else if (cntd_dif_on > 15 && type == 3){		// clip data
			cntd_dif_on = 15;
		}
		if (cntd_dif_off < 0){
			cntd_dif_off = 0;
		}
		else if (cntd_dif_off > 15 && type == 3){	// clip data
			cntd_dif_off = 15;
		}
		if (cntd_dif_on > 2){
			pvc->vc_cntd_von ++;
			if ((cntd_dif_on >= 8  && type == 2) ||
				(cntd_dif_on >= 15 && type == 3)){
				pvc->vc_cntd_von ++;
			}
		}
		if (cntd_dif_off > 2){
			pvc->vc_cntd_voff ++;
			if ((cntd_dif_off >= 8  && type == 2) ||
				(cntd_dif_off >= 15 && type == 3)){
				pvc->vc_cntd_voff ++;
			}
		}
		pvc->vc_cntd_logoon  += cntd_dif_on;
		pvc->vc_cntd_logooff += cntd_dif_off;

		// for cntf_logovc
		cntf_dif_on  = cntf_cur_on - pvc->cntf_last_on;
		cntf_dif_off = cntf_cur_off - pvc->cntf_last_off;
		if (cntf_dif_on < 0){
			cntf_dif_on = 0;
		}
		else if (cntf_dif_on > 15 && type == 3){		// clip data
			cntf_dif_on = 15;
		}
		if (cntf_dif_off < 0){
			cntf_dif_off = 0;
		}
		else if (cntf_dif_off > 15 && type == 3){	// clip data
			cntf_dif_off = 15;
		}
		if (cntf_dif_on > 2){
			pvc->vc_cntf_von ++;
			if ((cntf_dif_on >= 8  && type == 2) ||
				(cntf_dif_on >= 15 && type == 3)){
				pvc->vc_cntf_von ++;
			}
		}
		if (cntf_dif_off > 2){
			pvc->vc_cntf_voff ++;
			if ((cntf_dif_off >= 8  && type == 2) ||
				(cntf_dif_off >= 15 && type == 3)){
				pvc->vc_cntf_voff ++;
			}
		}
		pvc->vc_cntf_logoon  += cntf_dif_on;
		pvc->vc_cntf_logooff += cntf_dif_off;
//printf("(%d,%d,%d,%d)", (int)cntd_dif_on, (int)cntd_dif_off, (int)cntf_dif_on, (int)cntf_dif_off);
	}
	return 0;
}

//---------------------------------------------------------------------
// １画像−演算−有効データ数結果から補正
// 有効データ数測定結果から補正処理
//---------------------------------------------------------------------
int LogoCalc_getdif_vcresult(LOGO_CALCREC *plogoc, LOGO_VCCALCREC *pvc){

	// set data
	plogoc->cnt_logovc  = pvc->vc_cntd_logoon + pvc->vc_cntd_logooff;
	plogoc->cntf_logovc = pvc->vc_cntf_logoon + pvc->vc_cntf_logooff;

	// case for cnt_logovc
	if (pvc->vc_cntd_von <= 3 && pvc->vc_cntd_voff <= 3){
		if (plogoc->cnt_logovc > 20){
			if (plogoc->cnt_logoon > 10){
				plogoc->cnt_logoon = 10;
			}
			if (plogoc->cnt_logooff > 10){
				plogoc->cnt_logooff = 10;
			}
			plogoc->cnt_logovc = plogoc->cnt_logoon + plogoc->cnt_logooff;
		}
	}
	else if (pvc->vc_cntd_von <= 3){
		if (plogoc->cnt_logoon > plogoc->cnt_logooff && plogoc->cnt_logooff > 10){
			plogoc->cnt_logoon = plogoc->cnt_logooff - 1;
		}
	}
	else if (pvc->vc_cntd_voff <= 3){
		if (plogoc->cnt_logooff > plogoc->cnt_logoon && plogoc->cnt_logoon > 10){
			plogoc->cnt_logooff = plogoc->cnt_logoon - 1;
		}
	}
	else{
		if (pvc->vc_cntd_logoon > pvc->vc_cntd_logooff &&
			plogoc->cnt_logoon  < plogoc->cnt_logooff){
			if (plogoc->cnt_logoon > 0){
				plogoc->cnt_logooff = plogoc->cnt_logoon - 1;
			}
		}
		if (pvc->vc_cntd_logoon < pvc->vc_cntd_logooff &&
			plogoc->cnt_logoon  > plogoc->cnt_logooff){
			if (plogoc->cnt_logooff > 0){
				plogoc->cnt_logoon = plogoc->cnt_logooff - 1;
			}
		}
	}
	if (plogoc->cnt_logovc > 20){
		if (pvc->vc_cntd_logoon > plogoc->cnt_logoon){
			pvc->vc_cntd_logoon = plogoc->cnt_logoon;
			plogoc->cnt_logovc  = pvc->vc_cntd_logoon + pvc->vc_cntd_logooff;
		}
		if (pvc->vc_cntd_logooff > plogoc->cnt_logooff){
			pvc->vc_cntd_logooff = plogoc->cnt_logooff;
			plogoc->cnt_logovc  = pvc->vc_cntd_logoon + pvc->vc_cntd_logooff;
		}
	}

	// case for cntf_logovc
	if (pvc->vc_cntf_von <= 3 && pvc->vc_cntf_voff <= 3){
		if (plogoc->cntf_logovc > 30){
			plogoc->cntf_logovc = 30;
		}
	}
	else if (pvc->vc_cntf_von <= 3){
		if (plogoc->cntf_logoon > plogoc->cntf_logooff && plogoc->cntf_logooff > 10){
			plogoc->cntf_logoon = plogoc->cntf_logooff - 1;
		}
	}
	else if (pvc->vc_cntf_voff <= 3){
		if (plogoc->cntf_logooff > plogoc->cntf_logoon && plogoc->cntf_logoon > 10){
			plogoc->cntf_logooff = plogoc->cntf_logoon - 1;
		}
	}
	else{
		if (pvc->vc_cntf_logoon > pvc->vc_cntf_logooff &&
			plogoc->cntf_logoon < plogoc->cntf_logooff){
			if (plogoc->cntf_logoon > 0){
				plogoc->cntf_logooff = plogoc->cntf_logoon - 1;
			}
		}
		if (pvc->vc_cntf_logoon < pvc->vc_cntf_logooff &&
			plogoc->cntf_logoon > plogoc->cntf_logooff){
			if (plogoc->cntf_logooff > 0){
				plogoc->cntf_logoon = plogoc->cntf_logooff - 1;
			}
		}
	}
	if (plogoc->cntf_logovc > 30){
		if (pvc->vc_cntf_logoon > plogoc->cntf_logoon){
			pvc->vc_cntf_logoon = plogoc->cntf_logoon;
			plogoc->cntf_logovc  = pvc->vc_cntf_logoon + pvc->vc_cntf_logooff;
		}
		if (pvc->vc_cntf_logooff > plogoc->cntf_logooff){
			pvc->vc_cntf_logooff = plogoc->cntf_logooff;
			plogoc->cntf_logovc  = pvc->vc_cntf_logoon + pvc->vc_cntf_logooff;
		}
	}

	return 0;
}

//---------------------------------------------------------------------
// １画像−演算
// １枚の画像データからロゴ有無計算値を取得
//   data         : 画像データ輝度値
//   pitch        : 画像データの１行データ数
//   thres_ymax   : この値を超える輝度時は演算を行わない
//   thres_ydege  : この値を超える輝度時はエッジ演算を行わない
//   thres_ydif   : この値/16以上の輝度差がロゴデータにあれば演算を行う。
//   thres_yoffedg: この値を超える輝度時はエッジ検出でロゴなし判定をしない
//---------------------------------------------------------------------
int LogoCalc_getdif(LOGO_CALCREC *plogoc1, LOGO_CALCREC *plogoc2, LOGO_PARAMREC *plogop,
					const BYTE *data, int pitch, short thres_ymax, short thres_yedge,
					short thres_ydif, short thres_yoffedg){
	LOGO_CALCREC  *plogoc;
	LOGO_VCCALCREC *pvc;
	LOGO_VCCALCREC vc1_rec, vc2_rec;
	int i,j;
	int interlace;
	long data_num_offset;
	long logo_width, logo_height;
	long logo_numline;
	long logo_num0, logo_num1;
	const BYTE *pt_data_line;
	const BYTE *pt_data0, *pt_data1;
	const int interval = 2;

	// initialize
	LogoCalc_getdif_vccalc(&vc1_rec, plogoc1, 0);
	LogoCalc_getdif_vccalc(&vc2_rec, plogoc2, 0);

	// detect at column edge
	interlace = plogop->yy % 2;
	data_num_offset = plogop->yy * pitch + plogop->yx;
	pt_data_line = &(data[data_num_offset]);
	logo_width  = plogop->yw;
	logo_height = plogop->yh;
	logo_numline = 0;
	for(i=0; i<logo_height; i++){
		if (interlace == 0){
			plogoc = plogoc1;
			pvc = &vc1_rec;
		}
		else{
			plogoc = plogoc2;
			pvc = &vc2_rec;
		}
		// before calc
		LogoCalc_getdif_vccalc(pvc, plogoc, 1);
		// main check
		pt_data0 = pt_data_line;
		pt_data1 = &(pt_data_line[interval]);
		logo_num0 = logo_numline;
		logo_num1 = logo_numline + interval;
		for(j=0; j<logo_width; j++){
			if (plogop->dif_y_col[logo_num0] > 0){
				LogoCalc_getdif_pix(
						plogoc, *pt_data0, *pt_data1,
						plogop->dp_y[logo_num0], plogop->y[logo_num0],
						plogop->dp_y[logo_num1], plogop->y[logo_num1],
						thres_ymax, thres_yedge, thres_ydif, thres_yoffedg,
						plogop->most_logo_y, 0 );
			}
			if (plogop->area_y[logo_num0] > 0){ // area detect algorithm
				LogoCalc_getdif_areasum(
						plogoc, *pt_data0,
						plogop->dp_y[logo_num0], plogop->y[logo_num0],
						plogop->area_y[logo_num0], thres_ymax, plogop->most_logo_y);
			}
			pt_data0 ++;
			pt_data1 ++;
			logo_num0 ++;
			logo_num1 ++;
		}
		// after calc
		LogoCalc_getdif_vccalc(pvc, plogoc, 2);
		// for next data
		pt_data_line += pitch;
		logo_numline += logo_width;
		interlace = 1 - interlace;
	}

	// detect at row edge
	interlace = plogop->yy % 2;
	data_num_offset = plogop->yy * pitch + plogop->yx;
	pt_data_line = &(data[data_num_offset]);
//	logo_width  = plogop->yw;
//	logo_height = plogop->yh;
	logo_numline = 0;
	for(i=0; i<logo_height; i++){
		if (interlace == 0){
			plogoc = plogoc1;
			pvc = &vc1_rec;
		}
		else{
			plogoc = plogoc2;
			pvc = &vc2_rec;
		}
		// before calc
		LogoCalc_getdif_vccalc(pvc, plogoc, 1);
		// main check
		pt_data0 = pt_data_line;
		pt_data1 = &(pt_data_line[pitch * interval]);
		logo_num0 = logo_numline;
		logo_num1 = logo_numline + (logo_width * interval);
		for(j=0; j<logo_width; j++){
			if (plogop->dif_y_row[logo_num0] > 0){
				LogoCalc_getdif_pix(
						plogoc, *pt_data0, *pt_data1,
						plogop->dp_y[logo_num0], plogop->y[logo_num0],
						plogop->dp_y[logo_num1], plogop->y[logo_num1],
						thres_ymax, thres_yedge, thres_ydif, thres_yoffedg,
						plogop->most_logo_y, 0 );
			}
			pt_data0 ++;
			pt_data1 ++;
			logo_num0 ++;
			logo_num1 ++;
		}
		// after calc
		LogoCalc_getdif_vccalc(pvc, plogoc, 3);

		// for next data
		pt_data_line += pitch;
		logo_numline += logo_width;
		interlace = 1 - interlace;
	}

	// revise data by vccalc
	for(i=0; i<2; i++){
		if (i == 0){
			plogoc = plogoc1;
			pvc = &vc1_rec;
		}
		else{
			plogoc = plogoc2;
			pvc = &vc2_rec;
		}
		LogoCalc_getdif_vcresult(plogoc, pvc);
		
	}

	// set total_dif
	plogoc1->total_dif = plogop->total_dif_c1;
	plogoc2->total_dif = plogop->total_dif_c2;

	return 0;
}

//---------------------------------------------------------------------
// １画像−集計
// １枚の画像データ集計結果（エッジ検出）からロゴ有無を検出
//---------------------------------------------------------------------
int LogoCalc_summary(LOGO_CALCREC *plogoc, LOGO_PARAMREC *plogop){
	long val_hist, val_hist_max, val_hist_sum;
	long  cnt_logo_both;
	long  cnt_logo_lim;
	long  cntf_logo_both;
	long  cntf_logoon, cntf_logooff, cntopp;
	long  cntf_valid, cntf_offlimit;
	long  num_d1, num_d2, num_sum;
	long  sumt_areanum, sumt_areafact, sumt_areadif;
	short weight_logocnt, weight_fadecnt;
	short rate_fade_hist;
	short rate_area_avg;
	short rate_logoon_cnt;
	short rate_logoon_cntf;
	short rate_logoon;
	short rate_fade;
	int n_hist_max;
	int i;
	int invalid_logooff, cnttmp1, cnttmp2, cnttmpsum;

	// get fade rate from hist_y
	val_hist_max = 0;
	val_hist_sum = plogoc->hist_y[LOGO_FADE_OFST-1] + 
				   plogoc->hist_y[LOGO_FADE_OFST+plogoc->fade_calcstep+1];
	for(i=LOGO_FADE_OFST; i <= LOGO_FADE_OFST+plogoc->fade_calcstep; i++){
		val_hist_sum += plogoc->hist_y[i];
		val_hist = plogoc->hist_y[i-1] +
				   plogoc->hist_y[i]   +
				   plogoc->hist_y[i+1] ;
		if (val_hist > val_hist_max){
			val_hist_max = val_hist;
			n_hist_max = i;
		}
	}
	if (val_hist_max > 0){
		rate_fade_hist =
			((plogoc->hist_y[n_hist_max-1] * (n_hist_max -1 - LOGO_FADE_OFST) +
			  plogoc->hist_y[n_hist_max]   * (n_hist_max    - LOGO_FADE_OFST) +
			  plogoc->hist_y[n_hist_max+1] * (n_hist_max +1 - LOGO_FADE_OFST)) * 1000 +
			  (val_hist_max * plogoc->fade_calcstep/2)) /
			  (val_hist_max * plogoc->fade_calcstep);
	}
	else{
		rate_fade_hist = 0;
	}
	if (rate_fade_hist < 0){
		rate_fade_hist = 0;
	}
	else if (rate_fade_hist > 1000){
		rate_fade_hist = 1000;
	}

	// get area avg rate from sum_area*
	sumt_areanum  = plogoc->sum_areanum;
	sumt_areafact = (plogoc->sum_areaon   - plogoc->sum_areaoff);
	sumt_areadif  = plogoc->sum_areadif;
	if (sumt_areadif > 0){
		rate_area_avg = (sumt_areafact * 1000 + sumt_areadif/2) / sumt_areadif;
	}
	else{
		rate_area_avg = 0;
	}
	if (rate_area_avg < 0){
		rate_area_avg = 0;
	}
	else if (rate_area_avg > 1000){
		rate_area_avg = 1000;
	}

	// check yoffedg
	invalid_logooff = 0;
	cnttmp1 = plogoc->cnt_logooff * 2 + plogoc->cntf_logooff;
	cnttmp2 = plogoc->cnt_offedg  * 2 + plogoc->cntf_offedg;
	if (cnttmp1 - cnttmp2 <= plogoc->cnt_logoon * 2 + plogoc->cntf_logoon){
		if (cnttmp1 > plogoc->cnt_logoon * 2 + plogoc->cntf_logoon){
			invalid_logooff = 1;
		}
	}
	else{
		plogoc->cnt_logooff  -= plogoc->cnt_offedg;
		plogoc->cntf_logooff -= plogoc->cntf_offedg;
	}
	// check cnts_logo
	cnttmpsum = plogoc->cnts_logoon + plogoc->cnts_logooff;
	if (cnttmpsum > 0){
		cnttmp1 = plogoc->cnts_logoon;
		cnttmp2 = plogoc->cnts_logooff;
		if (cnttmpsum > 30){
			cnttmp1 = plogoc->cnts_logoon  * 30 / cnttmpsum;
			cnttmp2 = plogoc->cnts_logooff * 30 / cnttmpsum;
		}
		plogoc->cntf_logoon  += cnttmp1;
		plogoc->cntf_logooff += cnttmp2;
		plogoc->cntf_logovc  += (cnttmp1 + cnttmp2);
	}

	// get logo on/off rate from cnt_logo*
	cnt_logo_both = plogoc->cnt_logoon + plogoc->cnt_logooff;
	if (cnt_logo_both > 0){
		rate_logoon_cnt = (plogoc->cnt_logoon * 1000 + cnt_logo_both/2 ) / cnt_logo_both;
	}
	else{
		rate_logoon_cnt = 0;
	}
	cnt_logo_lim = plogoc->cnt_logovc;

	// revise cntf
	cntf_logoon  = plogoc->cntf_logoon;
	cntf_logooff = plogoc->cntf_logooff;
	cntopp       = plogoc->cntopp_logoon;
//	cntf_valid   = cntf_logoon + cntf_logooff;
	cntf_valid   = plogoc->cntf_logovc;
	cntf_offlimit = cntf_logooff * 49/51 - 1;
	if (cntf_logooff < cntf_logoon && cntf_logooff < cntopp * 3){
		// revise logo-on
		cntf_valid   = cntf_logoon + (cntf_logooff/4) - cntopp;
		if (cntf_valid > plogoc->cntf_logovc){
			cntf_valid   = plogoc->cntf_logovc;
		}
		cntf_logooff += cntopp*2;
		if (cntf_logooff > cntf_logoon){		// max 50%
			cntf_logooff = cntf_logoon;
		}
	}
	else if (cntf_offlimit > cntf_logoon && cntf_logoon > cntopp * 3){
		// revise logo-off
		cntf_logoon += (cntf_logoon - cntopp);
		if (cntf_logoon > cntf_offlimit){
			cntf_logoon = cntf_offlimit;
		}
	}

	// get logo on/off rate from cntf_logo*
	cntf_logo_both = cntf_logoon + cntf_logooff;
	if (cntf_logo_both > 0){
		rate_logoon_cntf = (cntf_logoon * 1000 + cntf_logo_both/2 ) / cntf_logo_both;
	}
	else{
		rate_logoon_cntf = 0;
	}

	// balanced rate from cnt_logo and sum_area
	num_d1  = cnt_logo_both * 2;
	num_d2  = cntf_logo_both;
	num_sum = num_d1 + num_d2;
	if (num_sum == 0){
		num_sum = 1;
	}
	weight_logocnt = (num_d1 * 1000 + num_sum/2) / num_sum;
	rate_logoon = (rate_logoon_cnt * weight_logocnt +
				   rate_logoon_cntf * (1000 - weight_logocnt) +
				   10000/2) / 10000;
//printf("{%d %d %d %d}", (int)rate_logoon_cntf, (int)cntf_logooff, (int)cntf_logoon, (int)cntopp);
//printf("(A:%d %d %d)", (int)plogoc->cntf_logoon, (int)plogoc->cntf_logooff, (int)plogoc->cntopp_logoon);

	// balanced rate from hist_y and sum_area
	num_d1  = val_hist_max * 2;
	num_d2  = sumt_areanum;
	num_sum = num_d1 + num_d2;
	if (num_sum == 0){
		num_sum = 1;
	}
	weight_fadecnt = (num_d1 * 1000 + num_sum/2) / num_sum;
	rate_fade = (rate_fade_hist * weight_fadecnt +
				 rate_area_avg * (1000 - weight_fadecnt) +
				 10000/2) / 10000;

//printf("hist:%d %d %d\n", n_hist_max, val_hist_max, plogoc->hist_y[n_hist_max-1]);
//printf("rate:%d %d %d\n", rate_logoon_cnt, rate_fade_hist, rate_area_avg);
//printf("weight:%d %d\n", weight_logocnt, weight_fadecnt);

	// result
	plogoc->rate_areaavg_t  = (rate_area_avg  + 5) / 10;  // for debug
	plogoc->rate_logoon = rate_logoon;
	plogoc->rate_fade   = rate_fade;

	if ((cnt_logo_both < 4) && (cntf_valid < 8)){
//		((cntf_logo_both < 10) || (cntf_logo_both * 8 < plogoc->cntf_logost))){
		plogoc->flag_nosample = 1;
	}
	else{
		plogoc->flag_nosample = 0;
	}
	if (cnt_logo_both + plogoc->cnt_logomv < 4){
		plogoc->flag_nofadesm = 1;
	}
	else{
		plogoc->flag_nofadesm = 0;
	}

	// revise when check points are few
	if (plogoc->total_dif < 200 && plogoc->total_dif >= 10){
		if (cnt_logo_both * 2 > plogoc->total_dif){
			if (cnt_logo_both <= 100){
				cnt_logo_both = 101;
			}
			if (cnt_logo_lim * 2 > plogoc->total_dif){
				if (cnt_logo_lim <= 60){
					if (cnt_logo_lim * 4 > plogoc->total_dif * 3){
						cnt_logo_lim = 61;
					}
					else{
						cnt_logo_lim = 60;
					}
				}
			}
		}
		else if (cntf_valid * 2 > plogoc->total_dif){
			if (cntf_valid <= 100){
				if (cntf_valid * 4 > plogoc->total_dif * 3){
					cntf_valid = 101;
				}
				else{
					cntf_valid = 100;
				}
			}
		}
		else if ((cnt_logo_lim + cntf_valid/2) * 2 > plogoc->total_dif){
			if (cntf_valid < (100 - cnt_logo_lim)*2){
				cntf_valid = (100 - cnt_logo_lim)*2;
			}
		}
		else if (cnt_logo_lim * 6 > plogoc->total_dif && cnt_logo_lim > 4){
			if (cnt_logo_both <= 20){
				cnt_logo_both = 21;
			}
			if (cnt_logo_lim <= 20){
				cnt_logo_lim = 21;
			}
		}
		else if (cntf_valid > 4){
			if ((cntf_valid * 5 > plogoc->total_dif) ||
				(cntf_valid * 7 > plogoc->total_dif && cntf_valid > 25)){
				if (plogop->thres_dp_y < LOGO_MAX_DP * 13 / 100){
					if (cntf_valid <= 35){
						cntf_valid = 35;
					}
				}
				else{
					if (cntf_valid <= 34){
						cntf_valid = 34;
					}
				}
			}
		}
	}

	// decide rank
	if (plogoc->flag_nosample != 0){
		plogoc->rank_unclear = 9;
	}
	else if ((invalid_logooff > 0) && (rate_logoon < 50)){
		plogoc->rank_unclear = 9;
	}
	else if (((cnt_logo_both > 100) && (cnt_logo_lim > 60) && (rate_logoon < 5 || rate_logoon > 90)) ||
		((cntf_valid > 100) && (rate_logoon < 4 || rate_logoon > 90))){
		plogoc->rank_unclear = 0;
	}
	else if (((cnt_logo_both >= 100) && (cnt_logo_lim >= 60) &&  (rate_logoon < 15 || rate_logoon > 80)) ||
		((cntf_valid >= 100) && (rate_logoon < 10 || rate_logoon > 80)) ||
		((cnt_logo_lim + cntf_valid/2 > 100) && (rate_logoon < 10 || rate_logoon > 80))){
		plogoc->rank_unclear = 1;
	}
	else if (((cnt_logo_both > 20) && (cnt_logo_lim > 20) && (rate_logoon < 35 || rate_logoon > 65)) ||
		((cntf_valid > 35) && (rate_logoon < 35 || rate_logoon > 65)) ||
		((cntf_valid == 35) && (rate_logoon < 15 || rate_logoon > 75))){
		plogoc->rank_unclear = 2;
	}
	else{
		plogoc->rank_unclear = 3;
	}

	return 0;
}


//---------------------------------------------------------------------
// １画像−エリア検出集計−位置検出
// 輝度ヒストグラムから最大となる輝度を１６倍した整数で求める
//  入力
//   hist      : 256配列データの輝度ヒストグラム
//   st        : 検出開始する輝度
//   ed        : 検出終了する輝度
//  出力
//   返り値    : 最大となる輝度値（前後２輝度含めた平均）を１６倍した整数
//   val_select: 最大となる輝度値前後を含めた輝度数合計
//   vari_select: 最大となる輝度値前後の分散
// 
//---------------------------------------------------------------------
int LogoCalc_areasummary_getloc(long *val_select, short *vari_select, long *hist, int st, int ed){
	int i;
	int loc_max, loc;
	int mloc_avg5, loc_avg5_div, loc_avg5_mod, ind_avg5;
	int mul;
	long num, num_max;
	long val, val_calc;
	long num_avg5, val_num, val_data;
	long hist_clip[11];

	// get max 3-point location
	num_max = 0;
	loc_max = 1;
	for(i=st; i<=ed; i++){
		num = hist[i-1] + hist[i] + hist[i+1];
		if (num_max < num){
			num_max = num;
			loc_max = i;
		}
	}
	// get max 5-point data and peripheral data
	num_avg5 = 0;
	val_calc = 0;
	for(i=0; i<=10; i++){
		loc = loc_max + i - 5;
		if (loc>=0 && loc<=255){
			hist_clip[i] = hist[loc];
		}
		else{
			hist_clip[i] = 0;
		}
		if (i>=3 && i<=7){
			num_avg5 += hist_clip[i];
			val_calc += hist_clip[i] * loc;
		}
	}
	// get average
	if (num_avg5 > 0){
		mloc_avg5 = (val_calc * 16 + num_avg5/2) / num_avg5;
	}
	else{
		loc_max  = (st+ed)/2;
		mloc_avg5 = loc_max * 16;
	}
	loc_avg5_div = mloc_avg5 / 16;
	loc_avg5_mod = mloc_avg5 % 16;
	ind_avg5     = loc_avg5_div + 5 - loc_max;
	if (ind_avg5 < 3){
		ind_avg5 = 3;
	}
	else if (ind_avg5 > 7){
		ind_avg5 = 7;
	}

	// variance with weighting
	val_num  = 0;
	val_data = 0;
	for(i=0; i<7; i++){
		val = (hist_clip[ind_avg5 + i - 3] * (16 - loc_avg5_mod) +
			   hist_clip[ind_avg5 + i - 2] * loc_avg5_mod
			   + 8) / 16;
		if (i==0 || i==6){
			mul = 256;
		}
		else if (i==1 || i==5){
			mul = 64;
		}
		else if (i==2 || i==4){
			mul = 16;
		}
		else{
			mul = 0;
		}
		val_num  += val;
		val_data += val * mul;
	}
	if (val_num > 0){
		val_data = (val_data + val_num/2) / val_num;
	}
	else{
		val_data = 99;
	}


//for(i=0; i<256;i++){
//	if (hist[i] > 0){
//		printf("%d:%ld ", i,hist[i]);
//	}
//}
//printf("\n");

	*val_select = num_avg5;
	*vari_select  = val_data;
	return mloc_avg5;
}


//---------------------------------------------------------------------
// １画像−エリア検出集計
// １枚の画像データ集計結果（エリア検出）からロゴ有無を検出
// エッジ検出の集計を行った後、補足検出として使用
//   thres_ydege  : この値を超える輝度時は微妙な比較は行わない
//   area_type    : 0:大エリア  1:小エリア
//---------------------------------------------------------------------
int LogoCalc_areasummary_one(LOGO_CALCAREAREC *plogoa, short thres_yedge, short area_type){
	long val_histon_max, val_histcal_max, val_histoff_max;
	short vari_histon_max, vari_histcal_max, vari_histoff_max;
	int loc_histon_max, loc_histcal_max, loc_histoff_max;
	int sloc_histon_max, sloc_histcal_max;
	int loc_st, loc_ed;
	int diff1, diff2;
	int rate_histoff;

	// get max point in logo area
	loc_histon_max = LogoCalc_areasummary_getloc(&val_histon_max, &vari_histon_max, plogoa->hista_areaon, 1, 254);
	sloc_histon_max = (loc_histon_max + 8) / 16;
//printf("on:%d %d %d\n", loc_histon_max, (int)val_histon_max, (int)plogoa->num_hista_on);

	// get max point of calculated logo-off level in logo area
	loc_histcal_max = LogoCalc_areasummary_getloc(&val_histcal_max, &vari_histcal_max, plogoa->hista_areacal, 1, 254);
	sloc_histcal_max = (loc_histcal_max + 8) / 16;
//printf("cal:%d %d\n", loc_histcal_max, (int)val_histcal_max);

	// get max point in blank area within assumed level
	loc_st = sloc_histcal_max-4;
	loc_ed = sloc_histcal_max+4;
	if (loc_st > sloc_histon_max-2){
		loc_st = sloc_histon_max-2;
	}
	if (loc_ed < sloc_histon_max+2){
		loc_ed = sloc_histon_max+2;
	}
	if (loc_st < 1){
		loc_st = 1;
	}
	if (loc_ed > 254){
		loc_ed = 254;
	}
	loc_histoff_max = LogoCalc_areasummary_getloc(&val_histoff_max, &vari_histoff_max, plogoa->hista_areaoff, loc_st, loc_ed);
//printf("off:%d %d %d\n", loc_histoff_max, (int)val_histoff_max, (int)plogoa->num_hista_off);


	// get valid rate from logo area
	if (plogoa->num_hista_on > 0){
		plogoa->rate_areavalid = (val_histon_max * 100 + plogoa->num_hista_on/2) / plogoa->num_hista_on;
	}
	else{
		plogoa->rate_areavalid = 0;
	}
	// get valid rate from blank area
	if (plogoa->num_hista_off > 0){
		rate_histoff = (val_histoff_max * 100 + plogoa->num_hista_off/2) / plogoa->num_hista_off;
	}
	else{
		rate_histoff = 0;
	}

	plogoa->diff_arealogo = loc_histon_max - loc_histcal_max;
	plogoa->diff1_arealogo = loc_histon_max - loc_histoff_max;
	plogoa->diff2_arealogo = loc_histoff_max - loc_histcal_max;
	plogoa->vari_arealogo_on  = vari_histon_max;
	plogoa->vari_arealogo_cal = vari_histcal_max;
	plogoa->vari_arealogo_off = vari_histoff_max;

	diff1 = abs(plogoa->diff1_arealogo);
	diff2 = abs(plogoa->diff2_arealogo);

	// set result
	if (plogoa->rate_areavalid <= 70){
		plogoa->rate_arealogo = -1;
	}
	else if (rate_histoff < 60){
		plogoa->rate_arealogo = -1;
	}
	else if (area_type == 1 && plogoa->vari_arealogo_off >= 56){
		plogoa->rate_arealogo = -1;
	}
	else if (diff1 >= 56 && diff2 >= 56){
		plogoa->rate_arealogo = -1;
	}
	else if (diff1 - diff2 >= 48){
		plogoa->rate_arealogo = 65;
	}
	else if (diff2 - diff1 >= 48){
		plogoa->rate_arealogo = 35;
	}
	else if (plogoa->rate_areavalid <= 80){
		plogoa->rate_arealogo = -1;
	}
	else if (area_type == 1 && plogoa->vari_arealogo_off > 40){
		plogoa->rate_arealogo = -1;
	}
	else if (diff1 - diff2 >= 32){
		plogoa->rate_arealogo = 65;
	}
	else if (diff2 - diff1 >= 32){
		plogoa->rate_arealogo = 35;
	}
	else if (diff1 >= 10 && diff2 >= 30 && plogoa->vari_arealogo_off <= 30){
		plogoa->rate_arealogo = -1;
	}
	else if (diff1 >= 7 && plogoa->vari_arealogo_off <= diff1 * 2 && diff1 + 16 < diff2){
		plogoa->rate_arealogo = -1;
	}
	else if (diff1 >= 7 && plogoa->vari_arealogo_off <= diff1 * 2){
		plogoa->rate_arealogo = 65;
	}
	else if (plogoa->rate_areavalid <= 90){
		plogoa->rate_arealogo = -1;
	}
	else if (diff1 - diff2 >= 24){
		plogoa->rate_arealogo = 65;
	}
	else if (diff2 - diff1 >= 24 && plogoa->vari_arealogo_off <= 48){
		plogoa->rate_arealogo = 35;
	}
	else if (diff1 <= 6 && diff2 >= 20 && plogoa->vari_arealogo_off <= 24){
		plogoa->rate_arealogo = 35;
	}
	else if (diff1 <= 5 && diff2 >= 16 && plogoa->vari_arealogo_off <= 16){
		plogoa->rate_arealogo = 35;
	}
	else if (thres_yedge < sloc_histon_max || area_type > 0){
		plogoa->rate_arealogo = -1;
	}
	else if (diff1 <= 2 && diff2 - diff1 >= 12 && plogoa->vari_arealogo_off <= 9){
		plogoa->rate_arealogo = 35;
	}
	else if (diff1 == 0 && diff2 >= 10 && plogoa->vari_arealogo_off == 0){
		plogoa->rate_arealogo = 35;
	}
	else{
		plogoa->rate_arealogo = -1;
	}

	return 0;
}


//---------------------------------------------------------------------
// １画像−エリア検出集計
// １枚の画像データ集計結果（エリア検出）からロゴ有無を検出
// エッジ検出の集計を行った後、補足検出として使用
//   thres_ydege  : この値を超える輝度時は微妙な比較は行わない
//   num_areaset  : 0の時個別エリア検出は行わない
//---------------------------------------------------------------------
int LogoCalc_areasummary(LOGO_CALCREC *plogoc, short thres_yedge, short num_areaset){
	LOGO_CALCAREAREC *plogoa;
	int i;
	int flag_on, flag_off, flag_both;
	int weight_local;
	int rate_local, rate_main, rate_area;
	int cnt_logo_both;
	int area_type;

	// use only when no sample found or unclear by another algorithm
	if (plogoc->rank_unclear >= 3){
		flag_on = 0;
		flag_off = 0;
		rate_local = 0;
		for(i=0; i<LOGO_AREANUM; i++){
			plogoa = &(plogoc->area[i]);
			if (plogoa->num_hista_off >= 3 && plogoa->num_hista_on >= 3){
				if (i==0){
					area_type = 0;
				}
				else{
					area_type = 1;
				}
				LogoCalc_areasummary_one(plogoa, thres_yedge, area_type);	// get rate
				if (plogoa->rate_arealogo >= 0 && i > 0){		// for local area
					rate_local += plogoa->rate_arealogo;
					if (plogoa->rate_arealogo >= 50){
						flag_on ++;
					}
					else{
						flag_off ++;
					}
				}
//printf("[%d:%d(%d %d)%d(%d %d)(%d %d)]",
//	i, plogoa->rate_arealogo, (int)plogoa->num_hista_off, (int)plogoa->num_hista_on,
//	plogoa->rate_areavalid, plogoa->diff1_arealogo, plogoa->diff2_arealogo,
//	plogoa->vari_arealogo_on, plogoa->vari_arealogo_off);
			}
			else{
				plogoa->rate_arealogo = -1;
			}
		}
		// delete local area
		if (num_areaset == 0){
			flag_on = 0;
			flag_off = 0;
		}
		// local area
		flag_both = flag_on + flag_off;
		if (flag_both > 0){
			rate_local = (rate_local + flag_both/2) / flag_both;
		}
		// main area
		rate_main = plogoc->area[0].rate_arealogo;
		// total area
		if (flag_both == 0){
			rate_area = rate_main;
		}
		else if (rate_main < 0){
			if (flag_both < 3){
				rate_area = (rate_local + 50 + 1)/2;
			}
			else{
				rate_area = rate_local;
			}
		}
		else{
			weight_local = flag_both;
			rate_area = (rate_main * (10 - weight_local) + rate_local * weight_local + 5) / 10;
		}

		// set rate
		if (rate_area >= 0){
			if (plogoc->rank_unclear == 9){
				plogoc->rank_unclear = 3;
				plogoc->rate_logoon  = rate_area;
			}
			else if (plogoc->rank_unclear == 3){
				cnt_logo_both = plogoc->cnt_logoon + plogoc->cnt_logooff +
						(plogoc->cntf_logoon + plogoc->cntf_logooff)/2;
				if (cnt_logo_both < 10){
					plogoc->rate_logoon  = (plogoc->rate_logoon + rate_area * 2) / 3;
				}
				else{
					plogoc->rate_logoon  = (plogoc->rate_logoon + rate_area) / 2;
				}
			}
		}
		plogoc->rate_arealogo = rate_area;

//printf("(%d, %d, %d %d)",rate_local, rate_main, rate_area, plogoc->rate_arealogo);
	}
	else{
		plogoc->rate_arealogo = -2;
	}
	return 0;
}


//---------------------------------------------------------------------
// １画像−結果格納
// １枚の画像データの取得データを保存
//---------------------------------------------------------------------
void LogoCalc_savedata(LOGO_FRAMEREC *plogof, LOGO_CALCREC *plogoc1, LOGO_CALCREC *plogoc2, int nframe){
	short tmpdata;

	plogof->rate_logooni1[nframe] = (char) plogoc1->rate_logoon;
	plogof->rate_logooni2[nframe] = (char) plogoc2->rate_logoon;
	plogof->rate_fadei1[nframe] = (char) plogoc1->rate_fade;
	plogof->rate_fadei2[nframe] = (char) plogoc2->rate_fade;
	plogof->rate_logoon[nframe] = (char) ((plogoc1->rate_logoon + plogoc2->rate_logoon) / 2);
	plogof->rate_fade[nframe]   = (char) ((plogoc1->rate_fade + plogoc2->rate_fade) / 2);
	plogof->flag_nosample[nframe] = (char) ((plogoc2->flag_nosample << 1) + plogoc1->flag_nosample);
	plogof->flag_nofadesm[nframe] = (char) ((plogoc2->flag_nofadesm << 1) + plogoc1->flag_nofadesm);

	if (plogoc1->rank_unclear >= plogoc2->rank_unclear){
		tmpdata = plogoc1->rank_unclear;
	}
	else{
		tmpdata = plogoc2->rank_unclear;
	}
	plogof->rank_unclear[nframe] = (char) tmpdata;

	// revise
	if (plogoc1->rank_unclear == 9 && plogoc2->rank_unclear != 9){
		plogof->rank_unclear[nframe] = (char) plogoc2->rank_unclear;
		plogof->rate_logoon[nframe]  = (char) plogoc2->rate_logoon;
		plogof->rate_fade[nframe]    = (char) plogoc2->rate_fade;
	}
	else if (plogoc1->rank_unclear != 9 && plogoc2->rank_unclear == 9){
		plogof->rank_unclear[nframe] = (char) plogoc1->rank_unclear;
		plogof->rate_logoon[nframe]  = (char) plogoc1->rate_logoon;
		plogof->rate_fade[nframe]    = (char) plogoc1->rate_fade;
	}
}

//=====================================================================
// Public関数：画像１枚に対するロゴ検出を行う
// １枚の画像データのロゴ有無を取得
//   data      : 画像データ輝度値へのポインタ
//   pitch     : 画像データの１行データ数
//   nframe    : フレーム番号
//=====================================================================
void LogoCalc(LOGO_DATASET *pl, const BYTE *data, int pitch, int nframe){
	LOGO_PARAMREC *plogop  = &(pl->paramdat);
	LOGO_CALCREC  *plogoc1 = &(pl->calcdat1);	// for Bottom field of interlace
	LOGO_CALCREC  *plogoc2 = &(pl->calcdat2);	// for Top field of interlace
	LOGO_FRAMEREC *plogof  = &(pl->framedat);
	LOGO_THRESREC *plogot  = &(pl->thresdat);

	LogoCalc_clear( plogoc1, plogot->num_fadein, plogot->num_fadeout );
	LogoCalc_clear( plogoc2, plogot->num_fadein, plogot->num_fadeout );
	LogoCalc_getdif( plogoc1, plogoc2, plogop, data, pitch,
					 plogot->thres_ymax, plogot->thres_yedge,
					 plogot->thres_ydif, plogot->thres_yoffedg);
	LogoCalc_summary( plogoc1, plogop );
	LogoCalc_summary( plogoc2, plogop );
	LogoCalc_areasummary( plogoc1, plogot->thres_yedge, plogot->num_areaset );
	LogoCalc_areasummary( plogoc2, plogot->thres_yedge, plogot->num_areaset );
	LogoCalc_savedata( plogof, plogoc1, plogoc2, nframe );
}


//=====================================================================
// Public関数：デバッグ用に画像１枚のロゴ計算結果を出力
// １フレーム分ロゴ計算結果を出力（デバッグ用）
//   nframe   : フレーム番号
//   fpo_ana2 : 出力ファイルハンドル
//=====================================================================
void LogoWriteFrameParam(LOGO_DATASET *pl, int nframe, FILE *fpo_ana2){
	LOGO_CALCREC  *plogoc1 = &(pl->calcdat1);
	LOGO_CALCREC  *plogoc2 = &(pl->calcdat2);
	LOGO_CALCREC  *plogoc  = &(pl->calcdat1);
	LOGO_FRAMEREC *plogof  = &(pl->framedat);
	int i;
	long sumavg1, sumavg2;

	if (plogoc1->sum_areanum > 0){
		sumavg1 = plogoc1->sum_areaoff / plogoc1->sum_areanum;
	}
	else{
		sumavg1 = 0;
	}
	if (plogoc2->sum_areanum > 0){
		sumavg2 = plogoc2->sum_areaoff / plogoc2->sum_areanum;
	}
	else{
		sumavg2 = 0;
	}

	fprintf(fpo_ana2, "%6d %3d %1d %3d %3d", nframe,
		plogof->rate_logoon[nframe],
		plogof->rank_unclear[nframe],
		plogof->rate_fadei2[nframe],
		plogof->rate_fadei1[nframe]);
	fprintf(fpo_ana2, " B: %d %3d / %3ld %3ld (%3ld %3ld %3ld) %3d %3ld %3ld",
		plogoc2->flag_nosample, plogoc2->rate_logoon,
		plogoc2->cnt_logooff, plogoc2->cnt_logoon,
		plogoc2->cntf_logooff, plogoc2->cntf_logoon, plogoc2->cntopp_logoon,
		plogoc2->rate_areaavg_t,  plogoc2->sum_areanum, sumavg1 );
	fprintf(fpo_ana2, " T: %d %3d / %3ld %3ld (%3ld %3ld %3ld) %3d %3ld %3ld",
		plogoc1->flag_nosample, plogoc1->rate_logoon,
		plogoc1->cnt_logooff, plogoc1->cnt_logoon,
		plogoc1->cntf_logooff, plogoc1->cntf_logoon, plogoc1->cntopp_logoon,
		plogoc1->rate_areaavg_t,  plogoc1->sum_areanum, sumavg2 );
	fprintf(fpo_ana2, " AB");
	if (plogoc2->cnt_logooff + plogoc2->cnt_logoon <= 3 &&
		plogoc2->cntf_logooff + plogoc2->cntf_logoon <= 10){
		fprintf(fpo_ana2, "#");
	}
	else{
		fprintf(fpo_ana2, ":");
	}
	fprintf(fpo_ana2, " %3d %3d %3d %3d(%3d)",
		plogoc2->rate_arealogo, plogoc2->area[0].rate_areavalid,
		plogoc2->area[0].diff1_arealogo, plogoc2->area[0].diff2_arealogo,
		plogoc2->area[0].vari_arealogo_off );
	fprintf(fpo_ana2, "AT: %3d %3d %3d %3d(%3d)",
		plogoc1->rate_arealogo, plogoc1->area[0].rate_areavalid,
		plogoc1->area[0].diff1_arealogo, plogoc1->area[0].diff2_arealogo,
		plogoc1->area[0].vari_arealogo_off );
	if (0){
		for(i=0; i<LOGO_FADE_MAXLEVEL; i++){
			fprintf(fpo_ana2, " %ld", plogoc->hist_y[i]);
		}
	}
	fprintf(fpo_ana2, "\n");
}


//---------------------------------------------------------------------
// 全フレーム−ロゴ表示区間初期検出
// ロゴ期間を検出（第１段階）
// 確実なロゴ表示On/Off位置からロゴ表示On/Off回数を確定する
//---------------------------------------------------------------------
void LogoFind_rough(LOGO_FRAMEREC *plogof, LOGO_THRESREC *plogot){
	int i,j;
	int stat_logoon, flag_logoon, tmp_logoon;
	int ncount;
	int invalid_left, invalid_right;
	int rank_onlevel, n_next_onlevel_sel, n_cnt_onlevel;
	int num_onwidth_sel, num_onwidth_on, num_onwidth_off;
	int n_next_onlevel_on, n_next_onlevel_off;
	long frm_leftstart, frm_leftrise, frm_rightend;
	long num_find;

	// set onwidth
	num_onwidth_on  = plogot->num_onwidth;
	num_onwidth_off = plogot->num_offwidth;
	if (num_onwidth_off == 0){
		num_onwidth_off = plogot->num_onwidth;
	}
	// decide onlevel
	if (plogot->num_onlevel >= 10){
		rank_onlevel = plogot->num_onlevel / 10 + 1;
		n_next_onlevel_on  = (plogot->num_onlevel % 10) * num_onwidth_on / 10;
		n_next_onlevel_off = (plogot->num_onlevel % 10) * num_onwidth_off / 10;
	}
	else{
		rank_onlevel = plogot->num_onlevel + 1;
		n_next_onlevel_on  = 0;
		n_next_onlevel_off = 0;
	}

	// get logo on/off each frame
	for(i=0; i<plogof->num_frames; i++){
		if (plogof->rate_logoon[i] >= plogot->rate_th_logo){
			plogof->flag_logoon[i] = 1;
		}
		else{
			plogof->flag_logoon[i] = 0;
		}
	}

	// detect edge by using num_onwidth
	invalid_left  = 1;
	invalid_right = 0;
	frm_leftrise  = 0;
	frm_leftstart = 0;
	frm_rightend  = plogof->num_frames - 1;
	num_find = 0;
	stat_logoon = -1;
	for(i=0; i<plogof->num_frames; i++){
		flag_logoon = plogof->flag_logoon[i];
		// when end left margin
		if ((invalid_left == 1) && (i >= plogot->num_cutleft)){
			invalid_left = 0;
			if (stat_logoon == 1){
				plogof->res[num_find].frm_rise = frm_leftrise;
			}
			else if (stat_logoon != 0){
				stat_logoon = 0;
			}
		}
		// edge detection
		if ((stat_logoon != flag_logoon) && (invalid_right == 0) &&
			(plogof->rank_unclear[i] <= rank_onlevel) &&
			(num_find < LOGO_FIND_MAX)){
			n_cnt_onlevel = 0;
			ncount = 1;
			tmp_logoon = flag_logoon;
			j = i + 1;
			if (flag_logoon > 0){
				num_onwidth_sel    = num_onwidth_on;
				n_next_onlevel_sel = n_next_onlevel_on;
			}
			else{
				num_onwidth_sel    = num_onwidth_off;
				n_next_onlevel_sel = n_next_onlevel_off;
			}
			while((j < plogof->num_frames) && (tmp_logoon == flag_logoon) &&
				  (ncount < num_onwidth_sel)){
				tmp_logoon = plogof->flag_logoon[j];
				if (plogof->rank_unclear[j] == rank_onlevel){
					n_cnt_onlevel ++;
				}
				if (plogof->rank_unclear[j] > rank_onlevel ||
					n_cnt_onlevel > n_next_onlevel_sel){
					tmp_logoon = -1;
				}
				ncount ++;
				j ++;
			}
			if (tmp_logoon == flag_logoon){		// detect edge
				stat_logoon = flag_logoon;
				if (flag_logoon == 1){			// rise edge
					if (invalid_left == 1){
						frm_leftrise = i;
					}
					else if (i >= plogof->num_frames - plogot->num_cutright){
						frm_rightend = i - 1;
						invalid_right = 1;
					}
					else{
						plogof->res[num_find].frm_rise = i;
					}
				}
				else{							// fall edge
					if (invalid_left == 1){
						frm_leftstart = i;
					}
					else{
						plogof->res[num_find].frm_fall = i-1;
						num_find ++;
					}
				}
			}
		}
	}
	if ((stat_logoon == 1) && (invalid_right == 0)){
		plogof->res[num_find].frm_fall = plogof->num_frames - 1;
		num_find ++;
	}
	plogof->num_find = num_find;
	plogof->frm_leftstart = frm_leftstart;
	plogof->frm_rightend  = frm_rightend;
}


//---------------------------------------------------------------------
// 全フレーム−変化位置検出−位置検出
// ロゴがONからOFFに変化するフレームを検出（開始終了位置を調整）
//  入力
//   loc_start : 確実にロゴONのフレーム番号
//   loc_end   : 確実にロゴOFFのフレーム番号
//  出力
//   返り値    : ロゴOFFに変わる直前のフレーム番号
//---------------------------------------------------------------------
long LogoFind_fine_getloc(long *ploc_short, long *ploc_long,
	LOGO_FRAMEREC *plogof, long loc_start, long loc_end){
	long frm;
	int loc_step;
	int val, val_max;
	int ncnt0[3], ncnt1[3];
	int nhold1[2];
	int ncnt_nosmp;
	int ncand, nend, nfinish;
	int nunclear;
	long loc_short, loc_long, loc_cand;

	if (loc_start <= loc_end){
		loc_step = 1;
	}
	else{
		loc_step = -1;
	}

	val = 0;
	val_max = 0;
	ncnt_nosmp = 0;
	ncnt0[0] = 0;
	ncnt0[1] = 0;
	ncnt0[2] = 0;
	ncnt1[0] = 30;
	ncnt1[1] = 30;
	ncnt1[2] = 30;
	nhold1[0] = 0;
	nhold1[1] = 0;
	ncand   = 0;
	nend    = 0;
	nfinish = 0;
	loc_short = loc_start;
	loc_long  = loc_start;
	loc_cand  = loc_start;
	frm = loc_start;
	while((frm != loc_end) && (nfinish == 0)){
		nunclear = plogof->rank_unclear[frm];
		if (nunclear == 9){
			if (ncand == 0){
				ncnt_nosmp ++;
				loc_long  = frm;
				if (val + ncnt_nosmp >= val_max){
					loc_cand = frm;
					val = val_max;
				}
			}
			else{
				val --;
			}
		}
		else if (plogof->flag_logoon[frm] == 0){
			val --;
			ncnt_nosmp = 0;
			ncnt1[0] = 0;
			ncnt1[1] = 0;
			ncnt1[2] = 0;
			if (nunclear == 0){
				ncnt0[0] ++;
			}
			else{
				ncnt0[0] = 0;
			}
			if (nunclear <= 1){
				ncnt0[1] ++;
			}
			else{
				ncnt0[1] = 0;
			}
			if (nunclear <= 2){
				ncnt0[2] ++;
			}
			else{
				ncnt0[2] = 0;
			}
//			if (ncnt0[0] >= 5){
//				nfinish = 1;
//			}
			if ((ncnt0[0] >= 2) || (ncnt0[1] >= 5)){
				nend = 1;
			}
			if ((ncnt0[1] >= 2) || (ncnt0[2] >= 8)){
				ncand = 1;
			}
			if ((ncnt0[2] == 0) && (nend == 0) && (ncand == 0)){
				loc_long  = frm;
			}
		}
		else{
			val ++;
			ncnt_nosmp = 0;
			ncnt0[0] = 0;
			ncnt0[1] = 0;
			ncnt0[2] = 0;
			if (nunclear == 0){
				val ++;
				if (nhold1[0] > 0){
					nhold1[0] --;
				}
				if ((ncnt1[0] == 0) || (nhold1[0] == 0)){
					ncnt1[0] ++;
					nhold1[0] = 0;
				}
			}
			else{
				if (ncnt1[0] != 0){
					if ((nhold1[0] == 0) && (ncnt1[0] >= 2)){
						nhold1[0] = 2;
					}
					else if ((nhold1[0] <= 2) && (ncnt1[0] >= 3)){
						nhold1[0] += 2;
					}
					else{
						ncnt1[0] = 0;
					}
				}
			}
			if (nunclear <= 1){
				if (nhold1[1] > 0){
					nhold1[1] --;
				}
				if ((ncnt1[1] == 0) || (nhold1[1] == 0)){
					ncnt1[1] ++;
					nhold1[1] = 0;
				}
			}
			else{
				if (ncnt1[1] != 0){
					if ((nhold1[1] == 0) && (ncnt1[1] >= 2)){
						nhold1[1] = 2;
					}
					else if ((nhold1[1] <= 2) && (ncnt1[1] >= 3)){
						nhold1[1] += 2;
					}
					else{
						ncnt1[1] = 0;
					}
				}
			}
			if (nunclear <= 2){
				ncnt1[2] ++;
			}
			else{
				ncnt1[2] = 0;
			}

			if ((ncnt1[0] >= 2 && nhold1[0] == 0) || (ncnt1[1] >= 5 && nhold1[1] == 0) ||
				(ncnt1[2] >= 12)){
				ncand = 0;
			}
			if ((ncnt1[0] >= 2 && nhold1[0] == 0) || (ncnt1[1] >= 5 && nhold1[1] == 0)){
				nend = 0;
			}
			if ((ncnt1[0] >= 5 && nhold1[0] == 0) || (ncnt1[1] >= 15 && nhold1[1] == 0) ||
				(ncnt1[2] >= 30)){
				loc_short = frm;
				loc_long  = frm;
				loc_cand  = frm;
				nend = 0;
				ncand = 0;
				val = 0;
				val_max =0;
			}
			else if ((val >= val_max) && (nend == 0)){
				loc_long  = frm;
				loc_cand  = frm;
				val_max = val;
			}
			else if (nend == 0){
				loc_long  = frm;
			}
		}
		frm += loc_step;
	}
	*ploc_short = loc_short;
	*ploc_long  = loc_long;

	return loc_cand;
}

//---------------------------------------------------------------------
// 全フレーム−変化位置検出
// ロゴ期間を検出（開始終了位置を調整）
// 各ロゴ表示ON/OFF切り替わり位置を決定する
//---------------------------------------------------------------------
void LogoFind_fine(LOGO_FRAMEREC *plogof){
	int i;
	long loc_st, loc_ed;
	long loc_cand, loc_short, loc_long;

	for(i=0; i<plogof->num_find; i++){

		// revise rise edge
		loc_ed = plogof->res[i].frm_rise;
		if (i == 0){
			loc_st = plogof->frm_leftstart;
		}
		else{
			loc_st = plogof->res[i-1].frm_fall + 1;
		}
//		if (loc_ed - loc_st > 1800){
//			loc_st = loc_ed - 1800;
//		}
		loc_cand = LogoFind_fine_getloc(&loc_short, &loc_long, plogof, loc_ed, loc_st);
#ifdef DEBUG_PRINT_DET
		if (plogof->res[i].frm_rise != loc_cand){  // only debug
			printf("find-fine %ld > %ld\n", plogof->res[i].frm_rise, loc_cand);
		}
#endif
		plogof->res[i].frm_rise = loc_cand;
		plogof->res[i].frm_rise_l = loc_long;
		plogof->res[i].frm_rise_r = loc_short;

		// revise fall edge
		loc_ed = plogof->res[i].frm_fall + 1;
		loc_st = plogof->res[i].frm_rise + 1;
//		if (loc_ed - loc_st > 1800){
//			loc_st = loc_ed - 1800;
//		}
		loc_cand = LogoFind_fine_getloc(&loc_short, &loc_long, plogof, loc_st, loc_ed);
#ifdef DEBUG_PRINT_DET
		if (plogof->res[i].frm_fall != loc_cand){  // only debug
			printf("find-fine %ld > %ld\n", plogof->res[i].frm_fall, loc_cand);
		}
#endif
		plogof->res[i].frm_fall = loc_cand;
		plogof->res[i].frm_fall_l = loc_short;
		plogof->res[i].frm_fall_r = loc_long;
	}
}


//---------------------------------------------------------------------
// 全フレーム−フェード−検索範囲位置検出
// フェード検出用の開始または終了位置（多少余裕を持たせた位置）を取得
// フェード割合算出に必要な最大値／最小値も同時に取得
//  入力
//   loc_start   : 確実にフェード前のフレーム番号
//   loc_end     : 確実にフェード後のフレーム番号
//   param_fade  : フェード検出する中間フレーム数（30フレームフェード対応）
//   npre        : 検出開始前に最大値／最小値だけ調べるフレーム数
//   flag_getmax : 最大／最小どちらを取得するか（0:最小値 1:最大値）
//  出力
//   返り値      : 余裕を持たせた開始または終了位置のフレーム番号
//   val_result  : 最大値または最小値
//---------------------------------------------------------------------
long LogoFind_fade_getloc(int *val_result, LOGO_FRAMEREC *plogof, long loc_start, long loc_end,
						  int param_fade, int npre, int flag_getmax){
	long frm;
	int val, val_d1, val_d2, val_d3;
	int val_i1, val_i2;
	int ncount, npost;
	int loc_step;
	int flag_1st;
	int flag_nofadesm;
	int difchk, npost_thres;
	static const int interlace = 2;		// 1:インターレース考慮なし 2:インターレース考慮あり

	if (param_fade > 10){	// for 30frame fade
		difchk = 2;
		npost_thres = param_fade;
	}
	else{
		difchk = 10;
		npost_thres = 7;
	}

	if (loc_start <= loc_end){
		loc_step = 1;
	}
	else{
		loc_step = -1;
	}
	npost = 0;
	frm = loc_start - loc_step;
	val_d1 = 0;
	val_d2 = 0;
	val_d3 = 0;
	ncount = 0;
	flag_1st = 1;
	while( (ncount < 3 || npost < npost_thres) && frm != loc_end){
		if (npre > 0){
			npre --;
			ncount = 0;
		}
		else{
			npost ++;
		}
		frm += loc_step;
		if (interlace == 2){
			val_i1 = plogof->rate_fadei1[frm];
			val_i2 = plogof->rate_fadei2[frm];
			flag_nofadesm = plogof->flag_nofadesm[frm] & 0x2;
			if (flag_nofadesm != 0){
				flag_nofadesm = plogof->flag_nofadesm[frm] & 0x1;
				val = val_i1;
			}
			else if ((plogof->flag_nofadesm[frm] & 0x1) != 0){
				val = val_i2;
			}
			else if (flag_getmax == 0){
				if (val_i1 <= val_i2){
					val = val_i1;
				}
				else{
					val = val_i2;
				}
			}
			else{
				if (val_i1 >= val_i2){
					val = val_i1;
				}
				else{
					val = val_i2;
				}
			}
		}
		else{
			flag_nofadesm = plogof->flag_nofadesm[frm];
			val = plogof->rate_fade[frm];
		}
		if (flag_nofadesm != 0){
		}
		else if (flag_getmax == 0){
			if (flag_1st != 0){
				val_d1 = val;
				flag_1st = 0;
			}
			else if (val + difchk < val_d1){
				ncount = 0;
			}
			else if ((val + difchk < val_d2) && (ncount > 0)){
				ncount = 1;
			}
			else if ((val + difchk < val_d3) && (ncount > 1)){
				ncount = 2;
			}
			else{
				ncount ++;
			}
			val_d3 = val_d2;
			val_d2 = val_d1;
			if (val < val_d1){
				val_d1 = val;
			}
		}
		else{
			if (flag_1st != 0){
				val_d1 = val;
				flag_1st = 0;
			}
			else if (val > val_d1 + difchk){
				ncount = 0;
			}
			else if ((val > val_d2 + difchk) && (ncount > 0)){
				ncount = 1;
			}
			else if ((val > val_d3 + difchk) && (ncount > 1)){
				ncount = 2;
			}
			else{
				ncount ++;
			}
			val_d3 = val_d2;
			val_d2 = val_d1;
			if (val > val_d1){
				val_d1 = val;
			}
		}
	}
	*val_result = val_d1;
	return frm;
}


//---------------------------------------------------------------------
// 全フレーム−フェード−位置検出
// フェードの最適値となる位置を検出
//  入力
//   param_fade : フェード検出する中間フレーム数
//   loc_left   : 検索開始するフレーム番号
//   loc_right  : 検索終了するフレーム番号
//   val_min    : 最低輝度値
//   val_max    : 最大輝度値
//   flag_rise  : 0:フェードアウト 1:フェードイン
//  出力
//   返り値     : フェード位置となるフレーム番号
//   num_fade   : フェード値（0の時フェードなし）
//   num_intl   : 検出インターレース（0:両方 1:Bottom 2:Top）
//---------------------------------------------------------------------
long LogoFind_fade_detect(int *num_fade, int *num_intl, LOGO_FRAMEREC *plogof,
						  int param_fade, long loc_left, long loc_right,
						  int val_min, int val_max, int flag_rise){
	int j, k, n, it, knum, ks;
	long frm;
	long loc_start, loc_end;
	long loc_avgdif_hold, loc_sumdif_hold;
	int val_avgdif_hold, val_sumdif_hold;
	int val_avgdif, val_sumdif;
	int val_exp, val_cur, val_tmp;
	int dif_it, sel_it, flag_nofadesm;
	int step, num_fade_hold;
	int ncnt_nosample;
	int ncnt_nearsample, val_ref1, val_ref2;		// for 30frame fade
	int intl_sumdif_hold, num_intl_hold;
	static const int interlace = 2;		// 1:インターレース考慮なし 2:インターレース考慮あり

//printf("[%ld,%ld]\n", loc_left, loc_right);

	loc_avgdif_hold = -1;
	val_avgdif_hold = 0;
	num_fade_hold = 0;
	num_intl_hold = 0;
	intl_sumdif_hold = 0;
	for(j=0; j<2; j++){
		if (j==0){
			step = 1;
		}
		else{
			step = param_fade + 1;
			if (step <= 0){
				step = 1;
			}
		}
		loc_start = loc_left;
		loc_end = loc_right - (step+2);
		loc_sumdif_hold = -1;
		val_sumdif_hold = 0;
		for(frm = loc_start; frm <= loc_end; frm++){
			dif_it = 0;
			for(n=0; n<(step+3); n++){
				if (plogof->flag_nofadesm[frm + n] == 0){
					dif_it += plogof->rate_fadei1[frm + n] - plogof->rate_fadei2[frm + n];
				}
			}
			for(it=0; it<interlace; it++){
				ncnt_nosample = 0;
				ncnt_nearsample = 0;		// for 30frame sample
				val_sumdif = 0;
				for(k=0; k<(step+3)*interlace; k++){
					knum = (it+k) / interlace;
					ks = k / interlace;
					if (interlace == 2){		// select interlace
						if ((((it + k) % 2 == 0) ^ (dif_it <= 0) ^ (flag_rise == 0)) == 0){
							sel_it = 1;			// select interlace TOP
						}
						else{
							sel_it = 2;			// select interlace BOTTOM
						}
					}
					else{
						sel_it = 0;				// select NOT interlace
					}
					if (sel_it == 1){
						flag_nofadesm = plogof->flag_nofadesm[frm + knum] & 0x1;
						val_cur = plogof->rate_fadei1[frm + knum];
					}
					else if (sel_it == 2){
						flag_nofadesm = plogof->flag_nofadesm[frm + knum] & 0x2;
						val_cur = plogof->rate_fadei2[frm + knum];
					}
					else{
						flag_nofadesm = plogof->flag_nofadesm[frm + knum];
						val_cur = plogof->rate_fadei2[frm + knum];
					}
					// get expect value
					if (flag_rise != 0){
						if (ks == 0){
							val_exp = val_min;
						}
						else if (ks == step+2){
							val_exp = val_max;
						}
						else{
							val_exp = ((ks - 1) * (val_max - val_min) + (step/2)) / step + val_min;
						}
					}
					else{
						if (ks == 0){
							val_exp = val_max;
						}
						else if (ks == step+2){
							val_exp = val_min;
						}
						else{
							val_exp = ((step + 1 - ks) * (val_max - val_min) + (step/2)) / step + val_min;
						}
					}
					if (flag_nofadesm != 0){
						ncnt_nosample ++;
					}
					else{
						val_tmp = abs(val_cur - val_exp);
						val_sumdif += val_tmp;
						if (val_tmp * 2 > (val_max - val_min)){		// differ over 50%
							ncnt_nosample ++;
						}
						if (step > 10){							// for 30frame fade
							if (val_cur < (val_exp-val_min)/2+val_min && ks < step){
								ncnt_nosample ++;
								val_sumdif -= val_tmp;
							}
							else if (val_tmp * 4 <= val_max - val_min){	// differ under 25%
								ncnt_nearsample ++;

								// emphasis logo appear point
								if ((flag_rise != 0 && (ks==0 || ks==1)) ||
									(flag_rise == 0 && (ks==step+1 || ks==step+2))){
									val_sumdif += val_tmp;
								}
							}
							// emphasis fade start/stop point
							val_ref1 = -1;
							if (ks == 2){
								if (sel_it == 1){
									val_ref1 = plogof->rate_fadei1[frm+knum-1];
									val_ref2 = plogof->rate_fadei1[frm+knum-2];
								}
								else{
									val_ref1 = plogof->rate_fadei2[frm+knum-1];
									val_ref2 = plogof->rate_fadei2[frm+knum-2];
								}
							}
							else if (ks == step){
								if (sel_it == 1){
									val_ref1 = plogof->rate_fadei1[frm+knum+1];
									val_ref2 = plogof->rate_fadei1[frm+knum+2];
								}
								else{
									val_ref1 = plogof->rate_fadei2[frm+knum+1];
									val_ref2 = plogof->rate_fadei2[frm+knum+2];
								}
							}
							if (val_ref1 >= 0){
								if ((flag_rise != 0 && ks == 2) || (flag_rise == 0 && ks == step)){
									if (val_cur <= val_ref1){
										val_sumdif += step/2;
									}
									if (val_ref2 < val_ref1 && val_ref1 < val_cur){
										val_sumdif += step;
									}
									if (val_ref2 < val_ref1){
										val_sumdif += step/4;
									}
								}
								else if ((flag_rise == 0 && ks == 2) || (flag_rise != 0 && ks == step)){
									if (val_cur >= val_ref1){
										val_sumdif += step/2;
									}
								}
							}
						}
					}
				}
				if ((ncnt_nosample > 0) && (ncnt_nosample < step * interlace)){
					val_sumdif += ((val_sumdif * ncnt_nosample) / ((step + 3) * interlace - ncnt_nosample));
				}
//if (j > 0){
//	  printf("%d:%ld:%d,%d ", val_sumdif, frm, ncnt_nosample, ncnt_nearsample);
//}
				if (((val_sumdif < val_sumdif_hold) || (loc_sumdif_hold < 0)) &&
					(ncnt_nosample < step*interlace/2) &&
					((step > 1) || (ncnt_nosample == 0)) &&
					(step <= 10 || (ncnt_nearsample > (step + 3)*interlace*3/5))){	// for 30frame fade
					val_sumdif_hold = val_sumdif;
					if (flag_rise != 0){
						loc_sumdif_hold = frm + 2;
						if (it == 1){
							intl_sumdif_hold = (dif_it < 0)? 2 : 1;
						}
						else{
							intl_sumdif_hold = 0;
						}
					}
					else{
						loc_sumdif_hold = frm + step + it;
						if (it == 1){
							intl_sumdif_hold = (dif_it < 0)? 2 : 1;
						}
						else{
							intl_sumdif_hold = 0;
						}
					}
				}

			}
		}
//		printf("@@%ld:%d:%d ", loc_sumdif_hold, j, val_sumdif_hold);
		if (loc_sumdif_hold >= 0){
			val_avgdif = (val_sumdif_hold * 100 + (step + 3)/2) / (step + 3);
			if ((val_avgdif < val_avgdif_hold) || (loc_avgdif_hold < 0) ||
				(step > 10 && loc_sumdif_hold >= 0)){			// add for 30frame fade
				val_avgdif_hold = val_avgdif;
				loc_avgdif_hold = loc_sumdif_hold;
				num_fade_hold = step - 1;
				num_intl_hold = intl_sumdif_hold;
//				if (j == 0){			// give priority to fade
//					val_avgdif_hold = val_avgdif_hold * 2;
//				}
			}
		}
	}

	// when not detect edge
	if (val_max - val_min < 20 || num_fade_hold == 0){
		num_fade_hold = 0;
		num_intl_hold = 0;
		loc_avgdif_hold = -1;
	}

	*num_fade = num_fade_hold;
	*num_intl = num_intl_hold;
	return loc_avgdif_hold;
}



//---------------------------------------------------------------------
// 全フレーム−フェード
// ロゴ期間を検出（フェード）
//---------------------------------------------------------------------
void LogoFind_fade(LOGO_FRAMEREC *plogof, LOGO_THRESREC *plogot){
	int i;
	long loc_target, loc_border_min, loc_border_max;
	long loc_start, loc_end, loc_left, loc_right;
	long loc_newrise, loc_newfall;
	int val_min, val_max;
	int num_fade_rise, num_fade_fall;
	int num_intl_rise, num_intl_fall;
	int npre;
	int num_fadein, num_fadeout;
	int total_fade;

	if (plogot->auto_fade > 0){
		num_fadein  = plogot->auto_fade;
		num_fadeout = plogot->auto_fade;
	}
	else{
		num_fadein  = plogot->num_fadein;
		num_fadeout = plogot->num_fadeout;
	}
	total_fade = 0;
	for(i=0; i<plogof->num_find; i++){
		// rise edge
		loc_target = plogof->res[i].frm_rise;
		if (i == 0){
			loc_border_min = plogof->frm_leftstart;
		}
		else{
			loc_border_min = plogof->res[i-1].frm_fall;
		}
		loc_border_max = plogof->res[i].frm_fall;

		npre = 5;
		loc_start = loc_target + npre;
		if (loc_start > loc_border_max){
			loc_start = loc_border_max;
			npre = loc_border_max - loc_target;
		}
		loc_end = loc_border_min;
		loc_left =  LogoFind_fade_getloc(&val_min, plogof, loc_start, loc_end, num_fadein, npre, 0);

		npre = 5;
		loc_start = loc_target - npre;
		if (loc_start < loc_border_min){
			loc_start = loc_border_min;
			npre = loc_target - loc_border_min;
		}
		loc_end = loc_border_max;
		loc_right =  LogoFind_fade_getloc(&val_max, plogof, loc_start, loc_end, num_fadein, npre, 1);

		loc_newrise = LogoFind_fade_detect(&num_fade_rise, &num_intl_rise, plogof, num_fadein,
						  loc_left, loc_right, val_min, val_max, 1);
		if (loc_newrise >= 0){
			total_fade ++;
#ifdef DEBUG_PRINT_DET
			if (plogof->res[i].frm_rise != loc_newrise){  // only debug
				printf("find-fade %ld > %ld\n", plogof->res[i].frm_rise, loc_newrise);
			}
#endif
		}
		// set to work area
		plogof->workres[i].frm_rise = loc_newrise;
		plogof->workres[i].fade_rise = num_fade_rise;
		plogof->workres[i].intl_rise = num_intl_rise;

		// fall edge
		loc_target = plogof->res[i].frm_fall;
		loc_border_min = plogof->res[i].frm_rise;
		if (i == (plogof->num_find - 1)){
			loc_border_max = plogof->frm_rightend;
		}
		else{
			loc_border_max = plogof->res[i+1].frm_rise;
		}

		npre = 5;
		loc_start = loc_target + npre;
		if (loc_start > loc_border_max){
			loc_start = loc_border_max;
			npre = loc_border_max - loc_target;
		}
		loc_end = loc_border_min;
		loc_left =  LogoFind_fade_getloc(&val_max, plogof, loc_start, loc_end, num_fadeout, npre, 1);

		npre = 5;
		loc_start = loc_target - npre;
		if (loc_start < loc_border_min){
			loc_start = loc_border_min;
			npre = loc_target - loc_border_min;
		}
		loc_end = loc_border_max;
		loc_right =  LogoFind_fade_getloc(&val_min, plogof, loc_start, loc_end, num_fadeout, npre, 0);

		loc_newfall = LogoFind_fade_detect(&num_fade_fall, &num_intl_fall, plogof, num_fadeout,
						  loc_left, loc_right, val_min, val_max, 0);
		if (loc_newfall >= 0){
			total_fade ++;
#ifdef DEBUG_PRINT_DET
			if (plogof->res[i].frm_fall != loc_newfall){  // only debug
				printf("find-fade %ld > %ld\n", plogof->res[i].frm_fall, loc_newfall);
			}
#endif
		}
		plogof->workres[i].frm_fall = loc_newfall;
		plogof->workres[i].fade_fall = num_fade_fall;
		plogof->workres[i].intl_fall = num_intl_fall;
	}

	// update fade data
	if ((total_fade >= plogof->num_find && total_fade > 1) ||
		plogot->auto_fade == 0){
		for(i=0; i<plogof->num_find; i++){
			if (plogof->workres[i].frm_rise >= 0){
				if (plogof->res[i].frm_rise_l > plogof->workres[i].frm_rise){
					plogof->res[i].frm_rise_l = plogof->workres[i].frm_rise;
				}
				plogof->res[i].frm_rise  = plogof->workres[i].frm_rise;
				plogof->res[i].fade_rise = plogof->workres[i].fade_rise;
				plogof->res[i].intl_rise = plogof->workres[i].intl_rise;
			}
			if (plogof->workres[i].frm_fall >= 0){
				if (plogof->res[i].frm_fall_r < plogof->workres[i].frm_fall){
					plogof->res[i].frm_fall_r = plogof->workres[i].frm_fall;
				}
				plogof->res[i].frm_fall  = plogof->workres[i].frm_fall;
				plogof->res[i].fade_fall = plogof->workres[i].fade_fall;
				plogof->res[i].intl_fall = plogof->workres[i].intl_fall;
			}
		}
	}
}


//---------------------------------------------------------------------
// 全フレーム−インターレース
// ロゴ期間を検出（インターレース検出）
//---------------------------------------------------------------------
void LogoFind_interlace(LOGO_FRAMEREC *plogof){
	int i, j;
	int val_max, val_dif, val_qi1, val_qi2, val_di1, val_di2;
	int flag_intl;
	long loc, loc_target, loc_max;

	for(i=0; i<plogof->num_find; i++){
		// rise edge
		loc_target = plogof->res[i].frm_rise;
		loc_max = 0;
		val_max = 0;
		flag_intl = 0;
		if (plogof->res[i].fade_rise == 0){
			if ((loc_target >= plogof->frm_leftstart + 2) && (loc_target <= plogof->frm_rightend - 1)){
				for(j=0; j<=3; j++){
					loc = loc_target + j - 2;
					val_dif = abs(plogof->rate_logooni1[loc] - plogof->rate_logooni2[loc]);
					if (val_dif > val_max){
						val_max = val_dif;
						if (j >= 1 && j<= 2){
							loc_max = loc;
						}
					}
				}
				if ((loc_max > 0) && (val_max > 20) &&
					((plogof->rate_logooni1[loc_max] <= 50 && plogof->rate_logooni2[loc_max] >= 50) ||
					 (plogof->rate_logooni1[loc_max] >= 50 && plogof->rate_logooni2[loc_max] <= 50))){
					val_qi1 = ((plogof->rate_logooni1[loc_max+1] - plogof->rate_logooni1[loc_max-1]) + 2) / 4;
					val_qi2 = ((plogof->rate_logooni2[loc_max+1] - plogof->rate_logooni2[loc_max-1]) + 2) / 4;
					val_di1 = plogof->rate_logooni1[loc_max] - plogof->rate_logooni1[loc_max-1];
					val_di2 = plogof->rate_logooni2[loc_max] - plogof->rate_logooni2[loc_max-1];
					if ((val_di1 > val_qi1*2) && (val_di2 < val_qi2*2) &&
						(val_di1 - val_qi1*3 > val_di2 - val_qi2)){
						flag_intl = 1;
					}
					else if ((val_di2 > val_qi2*2) && (val_di1 < val_qi1*2) &&
						(val_di2 - val_qi2*3 > val_di1 - val_qi1)){
						flag_intl = 2;
					}
				}
			}
			if (flag_intl > 0){
				if (plogof->res[i].frm_rise_l == plogof->res[i].frm_rise){
					plogof->res[i].frm_rise_l = loc_max;
				}
				if (plogof->res[i].frm_rise_r == plogof->res[i].frm_rise){
					plogof->res[i].frm_rise_r = loc_max;
				}
				plogof->res[i].frm_rise = loc_max;
			}
			plogof->res[i].intl_rise = flag_intl;
		}

		// fall edge
		loc_target = plogof->res[i].frm_fall;
		loc_max = 0;
		val_max = 0;
		flag_intl = 0;
		if (plogof->res[i].fade_fall == 0){
			if ((loc_target >= plogof->frm_leftstart + 1) && (loc_target <= plogof->frm_rightend - 2)){
				for(j=0; j<=3; j++){
					loc = loc_target + j - 1;
					val_dif = abs(plogof->rate_logooni1[loc] - plogof->rate_logooni2[loc]);
					if (val_dif > val_max){
						val_max = val_dif;
						if (j >= 1 && j<= 2){
							loc_max = loc;
						}
					}
				}
				if ((loc_max > 0) && (val_max > 20) &&
					((plogof->rate_logooni1[loc_max] <= 50 && plogof->rate_logooni2[loc_max] >= 50) ||
					 (plogof->rate_logooni1[loc_max] >= 50 && plogof->rate_logooni2[loc_max] <= 50))){
					val_qi1 = ((plogof->rate_logooni1[loc_max-1] - plogof->rate_logooni1[loc_max+1]) + 2) / 4;
					val_qi2 = ((plogof->rate_logooni2[loc_max-1] - plogof->rate_logooni2[loc_max+1]) + 2) / 4;
					val_di1 = plogof->rate_logooni1[loc_max] - plogof->rate_logooni1[loc_max+1];
					val_di2 = plogof->rate_logooni2[loc_max] - plogof->rate_logooni2[loc_max+1];
					if ((val_di1 > val_qi1*2) && (val_di2 < val_qi2*2) &&
						(val_di1 - val_qi1*3 > val_di2 - val_qi2)){
						flag_intl = 1;
					}
					else if ((val_di2 > val_qi2*2) && (val_di1 < val_qi1*2) &&
						(val_di2 - val_qi2*3 > val_di1 - val_qi1)){
						flag_intl = 2;
					}
				}
			}
			if (flag_intl > 0){
				if (plogof->res[i].frm_fall_l == plogof->res[i].frm_fall){
					plogof->res[i].frm_fall_l = loc_max;
				}
				if (plogof->res[i].frm_fall_r == plogof->res[i].frm_fall){
					plogof->res[i].frm_fall_r = loc_max;
				}
				plogof->res[i].frm_fall = loc_max;
			}
			plogof->res[i].intl_fall = flag_intl;
		}
	}
}


//=====================================================================
// Public関数：全画像検出完了後に実行してロゴ表示区間を検出
// ロゴ期間を検出
//=====================================================================
void LogoFind(LOGO_DATASET *pl){
	LOGO_FRAMEREC *plogof = &(pl->framedat);
	LOGO_THRESREC *plogot = &(pl->thresdat);

	LogoFind_rough(plogof, plogot);
	LogoFind_fine(plogof);
	LogoFind_fade(plogof, plogot);
	LogoFind_interlace(plogof);
}


//=====================================================================
// Public関数：ロゴ表示区間の総フレーム数を取得
// ロゴON期間全体のフレーム数を検出
//=====================================================================
long LogoGetTotalFrame(LOGO_DATASET *pl){
	LOGO_FRAMEREC *plogof = &(pl->framedat);
	int i;
	long total_frame, tmp_frame;

	total_frame = 0;
	for(i=0; i < plogof->num_find; i++){
		tmp_frame = plogof->res[i].frm_fall - plogof->res[i].frm_rise;
		if (tmp_frame > 0){
			total_frame += tmp_frame;
		}
	}
	return total_frame;
}


//=====================================================================
// Public関数：ロゴ表示区間の総フレーム数を取得（不明確なロゴは無効化）
// ロゴON期間全体のフレーム数を検出するが、不明確なロゴは無効とする
//=====================================================================
long LogoGetTotalFrameWithClear(LOGO_DATASET *pl){
	LOGO_FRAMEREC *plogof = &(pl->framedat);
	LOGO_THRESREC *plogot = &(pl->thresdat);
	int i, j;
	int loc_st, loc_ed;
	int unclear;
	long frame_sum;
	char rank;
	char logoon;

	if (plogot->num_clrrate == 0){
		unclear = 0;
	}
	else{
		unclear = 1;
	}
	i = 0;
	while( unclear > 0 && i < plogof->num_find ){
		loc_st = plogof->res[i].frm_rise_r;
		loc_ed = plogof->res[i].frm_fall_l;
		j = loc_st;
		while( unclear > 0 && j <= loc_ed ){
			rank   = plogof->rank_unclear[j];
			logoon = plogof->flag_logoon[j];
			if (rank >= 0 && rank <= 1 && logoon > 0){
				unclear = 0;
			}
			j ++;
		}
		i ++;
	}

	frame_sum = LogoGetTotalFrame(pl);		// get sum of logo-on frames
	if (unclear > 0){
		if (frame_sum * plogot->num_clrrate <= plogof->num_frames){
			plogof->num_frames = 0;			// invalidate logo
			frame_sum = 0;
		}
	}

	return frame_sum;
}


//---------------------------------------------------------------------
// ロゴ表示区間の結果を出力
//   fpo_ana : 出力ファイルハンドル
//---------------------------------------------------------------------
void LogoWriteFind_Sub(LOGO_OUTSUBREC *ps, int num_find, FILE *fpo_ana){
	int i;
	int n1, n2;
	static const char mark_intl[3][4] = {"ALL", "TOP", "BTM"};

	for(i=0; i<num_find; i++){
		if ((ps[i].intl_rise >= 0) && (ps[i].intl_rise <= 2)){
			n1 = ps[i].intl_rise;
		}
		else{
			n1 = 0;
		}
		if ((ps[i].intl_fall >= 0) && (ps[i].intl_fall <= 2)){
			n2 = ps[i].intl_fall;
		}
		else{
			n2 = 0;
		}
		fprintf(fpo_ana, "%6ld S %d %s %6ld %6ld",
			ps[i].frm_rise, ps[i].fade_rise,
			mark_intl[n1], ps[i].frm_rise_l, ps[i].frm_rise_r);
		fprintf(fpo_ana, "\n");
		fprintf(fpo_ana, "%6ld E %d %s %6ld %6ld",
			ps[i].frm_fall, ps[i].fade_fall,
			mark_intl[n2], ps[i].frm_fall_l, ps[i].frm_fall_r);
		fprintf(fpo_ana, "\n");
	}
}


//=====================================================================
// Public関数：ロゴ表示区間の結果を出力
// ロゴ期間結果を表示
//   fpo_ana : 出力ファイルハンドル
//=====================================================================
void LogoWriteFind(LOGO_DATASET *pl, FILE *fpo_ana){
	LOGO_FRAMEREC *plogof = &(pl->framedat);

	LogoWriteFind_Sub( plogof->res, plogof->num_find, fpo_ana);
}


//=====================================================================
// Public関数：フレーム最終結果を出力
//   prs     : 結果格納先
//   fpo_ana : 出力ファイルハンドル
//=====================================================================
void LogoResultWrite(LOGO_RESULTOUTREC *prs, FILE *fpo_ana){

	LogoWriteFind_Sub( prs->res, prs->num_find, fpo_ana);
}


//=====================================================================
// Public関数：フレーム最終結果の初期化
//   prs     : 結果格納先
//=====================================================================
void LogoResultInit(LOGO_RESULTOUTREC *prs){
	prs->num_find = 0;
}


//---------------------------------------------------------------------
// フレーム結果データ（１回分）代入
//   pdst    : 作成先データ
//   ptarget : 作成元データ
//   type    : 0:両エッジ  1:riseエッジ  2:fallエッジ
//---------------------------------------------------------------------
void LogoResultAdd_Copy(LOGO_OUTSUBREC *pdst, LOGO_OUTSUBREC *ptarget, int type){

	if (type == 0 || type == 1){
		pdst->frm_rise   = ptarget->frm_rise;
		pdst->frm_rise_l = ptarget->frm_rise_l;
		pdst->frm_rise_r = ptarget->frm_rise_r;
		pdst->fade_rise  = ptarget->fade_rise;
		pdst->intl_rise  = ptarget->intl_rise;
	}

	if (type == 0 || type == 2){
		pdst->frm_fall   = ptarget->frm_fall;
		pdst->frm_fall_l = ptarget->frm_fall_l;
		pdst->frm_fall_r = ptarget->frm_fall_r;
		pdst->fade_fall  = ptarget->fade_fall;
		pdst->intl_fall  = ptarget->intl_fall;
	}
}

//---------------------------------------------------------------------
// フレーム最終結果に１ロゴ分のデータ結果を追加
//   prs : 結果格納先
//   pl  : 既存ロゴデータ
//---------------------------------------------------------------------
void LogoResultAdd_set(LOGO_RESULTOUTREC *prs, LOGO_DATASET *pl){
	LOGO_FRAMEREC *plogof = &(pl->framedat);
	int i,j,k,m;
	LOGO_OUTSUBREC *ptarget;	// set logo data to insert
	LOGO_OUTSUBREC *pdst;		// set insert point to add
	const int spc = 30;			// treat as continuous logo within the frame

	for(i=0; i<plogof->num_find; i++){
		ptarget = &(plogof->res[i]);
		//--- search insert point ---
		k = 0;
		pdst = &(prs->res[k]);
		while(ptarget->frm_rise_l > pdst->frm_fall_r + spc && k < prs->num_find){
			k ++;
			if (k < prs->num_find){
				pdst = &(prs->res[k]);
			}
		}
		//--- divide if same logodata overlapped ---
		if (i > 0 && k < prs->num_find){
			if (plogof->res[i-1].frm_fall_r == pdst->frm_fall_r){
				k ++;
				if (k < prs->num_find){
					pdst = &(prs->res[k]);
				}
			}
		}
		//--- check insert point ('k' is insert point)---
		if (prs->num_find >= LOGO_FIND_MAX){			// already data full
		}
		else if (k == prs->num_find){					// add at last point
			LogoResultAdd_Copy( &(prs->res[k]), ptarget, 0 );	// insert data
			prs->num_find += 1;
		}
		else if (ptarget->frm_fall_r + spc < pdst->frm_rise_l){	// insert at the point of 'k'
			for(j=prs->num_find-1; j >= k; j--){
				LogoResultAdd_Copy( &(prs->res[j+1]), &(prs->res[j]), 0 );	// shift data
			}
			LogoResultAdd_Copy( &(prs->res[k]), ptarget, 0 );				// insert data
			prs->num_find += 1;
		}
		else{										// change logo edge (overlapped two logo)
			if (pdst->frm_rise > ptarget->frm_rise){		// check new rise edge of the logo
				LogoResultAdd_Copy( pdst, ptarget, 1 );		// change only rise edge of 'k'
			}
			//--- check insert logo if continued to the next logo ---
			m = k;
			while(prs->res[m+1].frm_rise_l < ptarget->frm_fall_r + spc && m+1 < prs->num_find){
				m ++;
			}
			if (k < m){			// unify logodata from k to m
				LogoResultAdd_Copy( &(prs->res[k]), &(prs->res[m]), 2 );	// change only fall edge of 'k'
				for(j=1; j < prs->num_find - m; j++){
					LogoResultAdd_Copy( &(prs->res[k+j]), &(prs->res[m+j]), 0 );	// shift data
				}
				prs->num_find -= (m - k);
			}
			//--- compare fall edge of exist logo and insert logo ---
			pdst = &(prs->res[k]);
			if (pdst->frm_fall < ptarget->frm_fall){
				LogoResultAdd_Copy( pdst, ptarget, 2 );		// change only fall edge of 'k'
			}
		}
	}
}


//---------------------------------------------------------------------
// ロゴ結果を追加するか判断する（自動判断時のみ使用）
// 既存結果と重なりがなければ追加と判断している
//   prs : 結果格納先
//   pl  : 既存ロゴデータ
//  出力
//   返り値     : 0:追加しない  1:追加する
//---------------------------------------------------------------------
int LogoResultAdd_check(LOGO_RESULTOUTREC *prs, LOGO_DATASET *pl){
	LOGO_FRAMEREC *plogof = &(pl->framedat);
	int i,j;
	int nst, ned, valid;
	LOGO_OUTSUBREC *ptarget;	// set logo data to check
	LOGO_OUTSUBREC *pdst;		// set current result point
	const int mgn = 3;			// frame overlap margin

	valid = 1;
	if (plogof->num_find == 0){		// no logo insert
		valid = 0;
	}
	else if (prs->num_find == 0){	// 1st insert
		valid = 1;
	}
	else{
		valid = 1;
		for(i=0; i<plogof->num_find; i++){
			ptarget = &(plogof->res[i]);
			nst = -1;
			ned = -1;
			for(j=0; j<prs->num_find; j++){
				pdst = &(prs->res[j]);
				if (ptarget->frm_rise_r + mgn >= pdst->frm_fall_l){
					nst = j;
				}
				if (ptarget->frm_fall_l > pdst->frm_rise_r + mgn){
					ned = j;
				}
			}
			if (nst != ned){        // overlapped current result
				valid = 0;
			}
		}
	}
	return valid;
}


//=====================================================================
// Public関数：フレーム最終結果に１ロゴ分のデータ結果を追加
//   prs     : 結果格納先
//   pl      : 既存ロゴデータ
//   autosel : 0:無条件に実行  1:挿入するか判断して実行
//  出力
//   返り値  : 0:追加なし  1:追加を実行
//=====================================================================
int LogoResultAdd(LOGO_RESULTOUTREC *prs, LOGO_DATASET *pl, int autosel){
	int flag;

	if (autosel == 1){
		flag = LogoResultAdd_check(prs, pl);
	}
	else{
		flag = 1;
	}
	if (flag > 0){
		LogoResultAdd_set(prs, pl);
	}
	return flag;
}


//=====================================================================
// Public関数：ロゴ表示区間結果を使ったサンプルを出力
// ロゴ結果を使ったavsファイル出力
//   logofname : ロゴデータのパス名
//   fpout     : 出力ファイルハンドル
//   outform   : 出力フォーマット（0:EraseLOGOのみ使用 1:追加スクリプトも使用）
//=====================================================================
void LogoWriteOutput(LOGO_DATASET *pl, const char *logofname, FILE *fpout, int outform){
	LOGO_FRAMEREC *plogof = &(pl->framedat);
	int i;
	int intl_rise, intl_fall;

	for(i=0; i<plogof->num_find; i++){
		if (outform == 1){
			// when fade set, not set interlace
			intl_rise = plogof->res[i].intl_rise;
			intl_fall = plogof->res[i].intl_fall;
			if (plogof->res[i].fade_rise > 1){
				intl_rise = 0;
			}
			if (plogof->res[i].fade_fall > 1){
				intl_fall = 0;
			}
			// output
			fprintf(fpout, "%s(logofile=\"%s\",\n", "ExtErsLOGO", logofname);
			fprintf(fpout, "\\          start=%ld, end=%ld, itype_s=%d, itype_e=%d, fadein=%d, fadeout=%d)\n",
				plogof->res[i].frm_rise, plogof->res[i].frm_fall,
				intl_rise, intl_fall,
				plogof->res[i].fade_rise, plogof->res[i].fade_fall);
		}
		else{
			fprintf(fpout, "%s(logofile=\"%s\",\n", "EraseLOGO", logofname);
			fprintf(fpout, "\\           start=%ld, end=%ld, fadein=%d, fadeout=%d)\n",
				plogof->res[i].frm_rise, plogof->res[i].frm_fall,
				plogof->res[i].fade_rise, plogof->res[i].fade_fall);
		}
	}
}

