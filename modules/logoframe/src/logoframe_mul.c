//====================================================================
// 複数ロゴ処理用の関数
//====================================================================

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif
#include <string.h>
#include <fcntl.h>
#include "logo.h"
#include "logoset.h"
#include "logoset_mul.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>
#include <unistd.h>
#include <strings.h>
#define stricmp strcasecmp
#define strnicmp strncasecmp
typedef unsigned char BYTE;
#ifdef __GNUC__
typedef long long int INT64;
#else
typedef __int64 INT64;
#endif
#endif

// logo function of logoframe
extern void LogoInit(LOGO_DATASET *pl);
extern void LogoFree(LOGO_DATASET *pl);
extern int  LogoRead(const char *logofname, LOGO_DATASET *pl);
extern int  LogoFrameInit(LOGO_DATASET *pl, long num_frames);
extern void LogoCalc(LOGO_DATASET *pl, const BYTE *data, int pitch, int nframe);
extern void LogoWriteFrameParam(LOGO_DATASET *pl, int nframe, FILE *fpo_ana2);
extern void LogoFind(LOGO_DATASET *pl);
extern long LogoGetTotalFrame(LOGO_DATASET *pl);
extern long LogoGetTotalFrameWithClear(LOGO_DATASET *pl);
extern void LogoWriteFind(LOGO_DATASET *pl, FILE *fpo_ana);
extern void LogoWriteOutput(LOGO_DATASET *pl, const char *logofname, FILE *fpout, int outform);
extern void LogoResultWrite(LOGO_RESULTOUTREC *prs, FILE *fpo_ana);
extern void LogoResultInit(LOGO_RESULTOUTREC *prs);
extern int  LogoResultAdd(LOGO_RESULTOUTREC *prs, LOGO_DATASET *pl, int autosel);



//---------------------------------------------------------------------
// Shift-JISの２バイト文字チェック
//---------------------------------------------------------------------
int sjis_multibyte(char *str){
	unsigned char code;

	code = (unsigned char)*str;
	if ((code >= 0x81 && code <= 0x9F) ||
		(code >= 0xE0 && code <= 0xFC)){		// Shift-JIS 1st-byte
		code = (unsigned char)*(str+1);
		if ((code >= 0x40 && code <= 0x7E) ||
			(code >= 0x80 && code <= 0xFC)){	// Shift-JIS 2nd-byte
			return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------
// Shift-JISコード考慮のstrrchr
//---------------------------------------------------------------------
char *strrchrj(char *str, char ch){
	char *dst;

	dst = NULL;
	if (str != NULL){
		while(*str != '\0'){
			if (*str == ch){
				dst = str;
			}
			if (sjis_multibyte(str) > 0){
				str ++;
			}
			str ++;
		}
	}
	return dst;
}
//---------------------------------------------------------------------
// Shift-JISコード考慮のstrchr
//---------------------------------------------------------------------
//char *strchrj(char *str, char ch){
//	char *dst;
//
//	dst = NULL;
//	if (str != NULL){
//		while(dst == NULL && *str != '\0'){
//			if (*str == ch){
//				dst = str;
//			}
//			if (sjis_multibyte(str) > 0){
//				str ++;
//			}
//			str ++;
//		}
//	}
//	return dst;
//}


//---------------------------------------------------------------------
// 実行ファイルのパスを取得
// ***** windows APIを使用 *****
//
// 失敗時は返り値=0
//---------------------------------------------------------------------
int MultLogo_GetModuleFileName(char *str, int maxlen){
#ifdef _WIN32
	return GetModuleFileName(NULL, str, maxlen);
#else
  ssize_t len = readlink("/proc/self/exe", str, maxlen - 1);
  if (len != -1) str[len] = '\0';
  return (len != -1);
#endif
}

//---------------------------------------------------------------------
// 拡張子前までを完全に含み、拡張子も同じファイルを展開
// ***** windows APIを使用 *****
//
// get filename (using WIN32API)
// input:  filename_src      filename before adding wildcard
// output: *baselist[]       matched list of filename (malloc string)
// return:                   0:normal 1:error by filename 2:error by malloc
//---------------------------------------------------------------------
int MultLogo_FileListGet(char *baselist[], const char *filename_src)
{
#ifdef _WIN32
	WIN32_FIND_DATA FindFileData;
	HANDLE hFile;
#else
  glob_t globbuf;
  struct stat st;
#endif
	char *strtmp;
	char *strext;
	char *newstr;      // for serach (add asterisk from filename_src)
	char *newstr2;     // for folder (extract folder from filename_src)
	int n, endf, nlen, nlen2;
	int errnum;

	errnum = 0;
	if (filename_src == NULL){
		return 1;
	}
	else if (strlen(filename_src) < 1){
		return 1;
	}

	// set filename to find
	// newstr  : search string. malloc=( filename_src + "\*" + EXTNAME_LOGODATA )
	// newstr2 : folder string. malloc=( filename_src + "\" )
	newstr = (char *)malloc( (strlen(filename_src)+strlen(EXTNAME_LOGODATA)+3) * sizeof(char) );
	if (newstr == NULL){
		fprintf(stderr, "error:failed in memory allocation.\n");
		errnum = 2;
	}
	newstr2 = (char *)malloc( (strlen(filename_src)+2) * sizeof(char) );
	if (newstr2 == NULL){
		fprintf(stderr, "error:failed in memory allocation.\n");
		errnum = 2;
	}
	if (errnum == 0){
		strcpy(newstr,  filename_src);
		strcpy(newstr2, filename_src);

		// add asterisk
		strext = strrchr(filename_src, '.');      // get ext location
		if (strext != NULL){
			if (strrchrj(strext, DELIMITER_DIR) != NULL){   // check if '.' is folder name
				strext = NULL;
			}
		}
		if (strext != NULL){                      // add '*' before ext
			strtmp = strrchr(newstr, '.');
			strcpy(strtmp, "*");
			strcat(strtmp, strext);
		}
		else{
			// check if folder name
#ifdef _WIN32
			hFile = FindFirstFile( newstr, &FindFileData );
			if (hFile != INVALID_HANDLE_VALUE){
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
					strcat(newstr,  DELIMITER_STRDIR);
					strcat(newstr2, DELIMITER_STRDIR);
				}
				FindClose(hFile);
			}
#else
      int ret = glob(newstr, 0, NULL, &globbuf);
      if (ret == 0){
        memset(&st, 0, sizeof(st));
        lstat(globbuf.gl_pathv[0], &st);
        if (S_ISDIR(st.st_mode)){
          strcat(newstr, DELIMITER_STRDIR);
          strcat(newstr2, DELIMITER_STRDIR);
        }
      }
      globfree(&globbuf);
#endif
			strcat(newstr, "*");
			strcat(newstr, EXTNAME_LOGODATA);   // 拡張子がない場合の追加
		}

		// get folder (newstr2 is folder)
		strtmp = strrchrj(newstr2, DELIMITER_DIR);
		if (strtmp != NULL){
			strcpy(strtmp, DELIMITER_STRDIR);
		}
		else{
			strcpy(newstr2, "");
		}
		nlen2 = strlen(newstr2);

		// free if data already exist
		for(n=0; n<LOGONUM_MAX; n++){
			if (baselist[n] != NULL){
				free(baselist[n]);
			}
		}
	}

	// get filename
	if (errnum == 0){
		n = 0;
#ifdef _WIN32
		hFile = FindFirstFile( newstr, &FindFileData );
		if (hFile != INVALID_HANDLE_VALUE) {
#else
    int ret = glob(newstr, 0, NULL, &globbuf);
    if (ret == 0){
#endif
			endf = 0;
			while(n < FILELISTNUM_MAX && endf == 0){
#ifdef _WIN32
				nlen = strlen(FindFileData.cFileName);
#else
        nlen = strlen(globbuf.gl_pathv[n]);
#endif
				baselist[n] = (char *)malloc( (nlen + nlen2 + 1) * sizeof(char) );
				if (baselist[n] == NULL){
					fprintf(stderr, "error:failed in memory allocation.\n");
					errnum = 2;
					endf = 1;
				}
				else{
#ifdef _WIN32
					strcpy(baselist[n], newstr2);                  // folder
					strcat(baselist[n], FindFileData.cFileName);   // name
#else
					strcpy(baselist[n], globbuf.gl_pathv[n]);   // name
#endif
					// sort
					for(int i=0; i<n; i++){
						if (strcmp(baselist[i], baselist[n]) > 0){
							strtmp = baselist[i];
							baselist[i] = baselist[n];
							baselist[n] = strtmp;
						}
					}
					// get next filename
#ifdef _WIN32
					if ( !FindNextFile( hFile, &FindFileData ) ){
#else
          if ( n+1 == globbuf.gl_pathc ){
#endif
						endf = 1;
					}
					n ++;
				}
			}
#ifdef _WIN32
			FindClose(hFile);
#else
      globfree(&globbuf);
#endif
		}
		if (n==0){
			fprintf(stderr, "warning:no logo found(%s)\n", newstr);
		}
	}
	if (newstr  != NULL) free(newstr);
	if (newstr2 != NULL) free(newstr2);

	return errnum;
}


//---------------------------------------------------------------------
// 文字列をメモリ確保して設定
//  入力
//   name_src   : 設定するファイル名
//  出力
//   name_dst   : 設定後ポインタ（mallocでメモリ確保済み）
//   返り値     : 0=正常, 2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogo_FileStrGet(char** name_dst, const char* name_src)
{
	char* newstr;
	char* tmpstr;
	int nlen;
	int pos, wlen, flag;

	pos= 0;
	wlen = 0;
	flag = 0;

	if (*name_dst != NULL){
		free(*name_dst);
		*name_dst = NULL;
	}
	if (name_src == NULL){
		return 0;
	}

	nlen = strlen(name_src);
	newstr = (char *)malloc( (nlen + 1) * sizeof(char) );
	if (newstr == NULL){
		fprintf(stderr, "error:failed in memory allocation.\n");
		return 2;
	}
	*name_dst = newstr;

	tmpstr = strchr(name_src, '\"');
	if (tmpstr != NULL){                 // if double quotes found
		tmpstr += 1;                     // move inside of the double quotes
		strcpy(newstr, tmpstr);
		tmpstr = strchr(newstr, '\"');   // next double quotes
		if (tmpstr != NULL){
			strcpy(tmpstr, "");          // delete after the double quotes
			flag = 1;
		}
	}
	if (flag == 0){
		while( name_src[pos] >= ' ' || name_src[pos] < 0){
			newstr[wlen++] = name_src[pos++];
		}
		newstr[wlen] = '\0';
	}
	if (strlen(newstr) == 0){
		free(newstr);
		*name_dst = NULL;
	}
	return 0;
}


//---------------------------------------------------------------------
// 文字列から項目と値を取得
//  入力
//   buf     : 文字列
//   sizestr : 項目文字列最大長
//  出力
//   strw    : 項目文字列
//   返り値  : 0以上=値のbuf内文字列位置, -1=値なし
//---------------------------------------------------------------------
int MultLogo_StrArgGet(char *strw, const char* buf, int sizestr){
	int pos, wlen, type;

	pos = 0;
	wlen = 0;
	type = 0;
	if (buf[0] == ';' || buf[0] == '#'){
		strcpy(strw, "");
		pos = -1;
	}
	else{
		while( buf[pos] == ' ' ) pos++; 	// trim left
		if (buf[pos] != '-'){
			if (strchr(buf, '=') != NULL){
				type = 1;					// .ini type
			}
		}
		if (type == 1){
			strw[wlen++] = '-';
		}
		while( (buf[pos] > ' ' || buf[pos] < 0) &&
			   (type != 1 || buf[pos] != '=') &&
			   wlen < sizestr-1 ){
			strw[wlen++] = buf[pos++];
		}
		strw[wlen] = '\0';
		while( buf[pos] == ' ' ) pos++;
		if (type == 1 && buf[pos] == '=') pos++;
		if (buf[pos] >= 0 && buf[pos] < ' '){	// not exist 2nd arg
			pos = -1;
		}
	}
	return pos;
}


//##### オプション関連処理

//---------------------------------------------------------------------
// 閾値パラメータの初期化
//  入出力
//   p : 仮設定領域の閾値パラメータ
//---------------------------------------------------------------------
void MultLogo_ParamClear(MLOGO_THRESREC *p){
	MLOGO_UPTHRESREC*  pup;

	pup = &(p->up);
	memset(pup, 0, sizeof(MLOGO_UPTHRESREC));
}

//---------------------------------------------------------------------
// 閾値パラメータを仮設定
//  入出力
//   p      : 仮設定領域の閾値パラメータ
//  入力
//   name   : 閾値パラメータ名前
//   value  : 閾値パラメータ値
//  出力
//   返り値 : 0=設定, 1=パラメータ非検出
//---------------------------------------------------------------------
int MultLogo_ParamPreSet(MLOGO_THRESREC *p, const char *name, const char *value){
    int n;

    n = atoi(value);
    if (!stricmp(name, "-fadein")) {
        p->dat.num_fadein     = n;
        p->dat.auto_fade      = 0;
        p->up.up_num_fadein   = 1;
        p->up.up_auto_fade    = 1;
    }
    else if (!stricmp(name, "-fadeout")) {
        p->dat.num_fadeout    = n;
        p->dat.auto_fade      = 0;
        p->up.up_num_fadeout  = 1;
        p->up.up_auto_fade    = 1;
    }
    else if (!stricmp(name, "-mrgleft")) {
        p->dat.num_cutleft    = n;
        p->up.up_num_cutleft  = 1;
    }
    else if (!stricmp(name, "-mrgright")) {
        p->dat.num_cutright   = n;
        p->up.up_num_cutright = 1;
    }
    else if (!stricmp(name, "-onwidth")) {
        p->dat.num_onwidth    = n;
        p->up.up_num_onwidth  = 1;
    }
    else if (!stricmp(name, "-onlevel")) {
        p->dat.num_onlevel    = n;
        p->up.up_num_onlevel  = 1;
    }
    else if (!stricmp(name, "-offwidth")) {
        p->dat.num_offwidth    = n;
        p->up.up_num_offwidth  = 1;
    }
    else if (!stricmp(name, "-ymax")) {
        p->dat.thres_ymax     = n;
        p->up.up_thres_ymax   = 1;
    }
    else if (!stricmp(name, "-ymin")) {
        p->dat.thres_ymin     = n;
        p->up.up_thres_ymin   = 1;
    }
    else if (!stricmp(name, "-yedge")) {
        p->dat.thres_yedge     = n;
        p->dat.auto_bs11       = 0;
        p->up.up_thres_yedge   = 1;
        p->up.up_auto_bs11     = 1;
    }
    else if (!stricmp(name, "-ydif")) {
        p->dat.thres_ydif     = n;
        p->up.up_thres_ydif   = 1;
    }
    else if (!stricmp(name, "-ysetdif")) {
        p->dat.thres_ysetdif     = n;
        p->up.up_thres_ysetdif   = 1;
    }
    else if (!stricmp(name, "-yoffedg")) {
        p->dat.thres_yoffedg     = n;
        p->up.up_thres_yoffedg   = 1;
    }
    else if (!stricmp(name, "-areaset")) {
        p->dat.num_areaset       = n;
        p->up.up_num_areaset     = 1;
    }
    else if (!stricmp(name, "-clrrate")) {
        p->dat.num_clrrate    = n;
        p->up.up_num_clrrate  = 1;
    }
    else{
        return 1;
	}

	return 0;
}


//---------------------------------------------------------------------
// 閾値パラメータを設定（仮設定領域内容を各ロゴデータにコピー）
//  入力
//   p      : 仮設定領域の閾値パラメータ
//  出力
//   plogot : 各ロゴデータの閾値パラメータ
//---------------------------------------------------------------------
void MultLogo_ParamCopy(LOGO_THRESREC *plogot, MLOGO_THRESREC *p){
	MLOGO_UPTHRESREC*  pup;
	LOGO_THRESREC*     pdat;

	pup  = &(p->up);
	pdat = &(p->dat);
	if (pup->up_num_fadein > 0){
		plogot->num_fadein   = pdat->num_fadein;
	}
	if (pup->up_num_fadeout > 0){
		plogot->num_fadeout  = pdat->num_fadeout;
	}
	if (pup->up_num_cutleft > 0){
		plogot->num_cutleft  = pdat->num_cutleft;
	}
	if (pup->up_num_cutright > 0){
		plogot->num_cutright = pdat->num_cutright;
	}
	if (pup->up_num_onwidth > 0){
		plogot->num_onwidth  = pdat->num_onwidth;
	}
	if (pup->up_num_onlevel > 0){
		plogot->num_onlevel  = pdat->num_onlevel;
	}
	if (pup->up_num_offwidth > 0){
		plogot->num_offwidth  = pdat->num_offwidth;
	}
	if (pup->up_thres_ymax > 0){
		plogot->thres_ymax   = pdat->thres_ymax;
	}
	if (pup->up_thres_ymin > 0){
		plogot->thres_ymin   = pdat->thres_ymin;
	}
	if (pup->up_thres_yedge > 0){
		plogot->thres_yedge  = pdat->thres_yedge;
	}
	if (pup->up_thres_ydif > 0){
		plogot->thres_ydif   = pdat->thres_ydif;
	}
	if (pup->up_thres_ysetdif > 0){
		plogot->thres_ysetdif   = pdat->thres_ysetdif;
	}
	if (pup->up_thres_yoffedg > 0){
		plogot->thres_yoffedg   = pdat->thres_yoffedg;
	}
	if (pup->up_num_areaset > 0){
		plogot->num_areaset    = pdat->num_areaset;
	}
	if (pup->up_num_clrrate > 0){
		plogot->num_clrrate  = pdat->num_clrrate;
	}
	if (pup->up_auto_fade > 0){
		plogot->auto_fade    = pdat->auto_fade;
	}
	if (pup->up_auto_bs11 > 0){
		plogot->auto_bs11    = pdat->auto_bs11;
	}
}


//=====================================================================
// Public関数：オプション設定
// オプションを読み込む
//  入力
//   strcmd : オプション名前文字列
//   strval : パラメータ値文字列
//  出力
//   返り値 : 使用引数（0=パラメータ検出せず, -1=エラー）
//=====================================================================
int MultLogoOptionAdd(MLOGO_DATASET* pml, const char* strcmd, const char* strval){
	int nlist;

	if (strcmd == NULL){
		nlist = 0;
	}
	else if (strval == NULL){
		nlist = 1;
	}
	else{
		nlist = 2;
	}
	if (nlist == 0){
		return 0;
	}
	if (strcmd[0] == '-' && strcmd[1] != '\0'){
		if (!stricmp(strcmd, "-logo")) {
			MultLogo_FileStrGet( &(pml->opt_logofilename), strval );
			return 2;
		}
		else if (!stricmp(strcmd, "-logoparam")) {
			MultLogo_FileStrGet( &(pml->opt_logoparamfile), strval );
			return 2;
		}
		else if (!stricmp(strcmd, "-oanum")) {
			if (nlist < 2) {
				fprintf(stderr, "-oanum needs an argument\n");
				return -1;
			}
			pml->oanum = atoi(strval);
			if (pml->oanum > LOGONUM_MAX){
				pml->oanum = LOGONUM_MAX;
			}
			return 2;
		}
		else if (!stricmp(strcmd, "-oasel")) {
			if (nlist < 2) {
				fprintf(stderr, "-oasel needs an argument\n");
				return -1;
			}
			pml->oasel = atoi(strval);
			return 2;
		}
		else if (!stricmp(strcmd, "-oamask")) {
			if (nlist < 2) {
				fprintf(stderr, "-oamask needs an argument\n");
				return -1;
			}
			pml->oamask = atoi(strval);
			return 2;
		}
		else if (!stricmp(strcmd, "-outform")) {
			if (nlist < 2) {
				fprintf(stderr, "-outform needs an argument\n");
				return -1;
			}
			pml->outform = atoi(strval);
			return 2;
		}
		else if (!stricmp(strcmd, "-dispoff")) {
			if (nlist < 2) {
				fprintf(stderr, "-dispoff needs an argument\n");
				return -1;
			}
			pml->dispoff = atoi(strval);
			return 2;
		}
		else if (!stricmp(strcmd, "-paramoff")) {
			if (nlist < 2) {
				fprintf(stderr, "-paramoff needs an argument\n");
				return -1;
			}
			pml->paramoff = atoi(strval);
			return 2;
		}
		else if (!stricmp(strcmd, "-o")) {
			MultLogo_FileStrGet( &(pml->opt_outfile), strval );
			return 2;
		}
		else if (!stricmp(strcmd, "-oa")) {
			MultLogo_FileStrGet( &(pml->opt_anafile), strval );
			return 2;
		}
		else if (!stricmp(strcmd, "-oa2")) {
			MultLogo_FileStrGet( &(pml->opt_ana2file), strval );
			return 2;
		}
		else if (!strnicmp(strcmd, "-logo", 5) &&
				 (strcmd[5] >= '0' && strcmd[5] <= '9')){
			int k = atoi(&strcmd[5]);
			if (k >= 1 && k <= LOGONUM_MAX){
				MultLogo_FileStrGet( &(pml->all_logofilename[k-1]), strval );
			}
			else{
				fprintf(stderr, "-logoN : N>=1 and N<=%d\n", LOGONUM_MAX);
				return -1;
			}
			return 2;
		}
		else if (!stricmp(strcmd, "-fadein")   ||
				 !stricmp(strcmd, "-fadeout")  ||
				 !stricmp(strcmd, "-mrgleft")  ||
				 !stricmp(strcmd, "-mrgright") ||
				 !stricmp(strcmd, "-onwidth")  ||
				 !stricmp(strcmd, "-onlevel")  ||
				 !stricmp(strcmd, "-offwidth") ||
				 !stricmp(strcmd, "-ymax")     ||
				 !stricmp(strcmd, "-ymin")     ||
				 !stricmp(strcmd, "-yedge")    ||
				 !stricmp(strcmd, "-ydif")     ||
				 !stricmp(strcmd, "-ysetdif")  ||
				 !stricmp(strcmd, "-yoffedg")  ||
				 !stricmp(strcmd, "-areaset")  ||
				 !stricmp(strcmd, "-clrrate")) {			// set threshold
			if (nlist < 2) {
				fprintf(stderr, "%s needs an argument\n", strcmd);
				return -1;
			}
			MultLogo_ParamPreSet( &(pml->thres_arg), strcmd, strval );
			return 2;
		}
	}

	return 0;
}


//=====================================================================
// Public関数：ファイルからのオプション設定
// ファイルに記載されているオプションを読み込む
//  入力
//   fname     : ファイル名
//  出力
//   返り値 : 設定したオプション項目数（0=設定なし, -1=エラー）
//=====================================================================
int MultLogoOptionFile(MLOGO_DATASET* pml, const char* fname){
	FILE* fpr;
	int pos, nopt, ncount, invalid;
	char buf[FILE_BUFSIZE];
	char str[FILE_BUFSIZE];
	const char* strval;

	ncount = 0;
	invalid = 0;
	fpr = fopen(fname, "r");
	if (!fpr){
//		fprintf(stderr, "warning:not found '%s'\n", fname);
		return -1;
	}
	else{
		while( fgets(buf, FILE_BUFSIZE, fpr) != NULL ){
			if (buf[0] == '['){				// when section
				if (!strnicmp(buf, SECTION_LOGOFRAME, strlen(SECTION_LOGOFRAME))){
					invalid = 0;			// section-in
				}
				else{
					invalid = 1;			// out of section
				}
			}
			else if (invalid == 0){
				pos = MultLogo_StrArgGet(str, buf, FILE_BUFSIZE);
				if (strlen(str) > 0){
					if (pos > 0){
						strval = &buf[pos];
					}
					else{
						strval = NULL;
					}
					
					nopt = MultLogoOptionAdd(pml, str, strval);
					if (nopt > 0){
						ncount ++;
					}
				}
			}
		}
		fclose(fpr);
	}
	return ncount;
}


//=====================================================================
// Public関数：初期読み込みファイルからのオプション設定
// ファイルに記載されているオプションを読み込む
//  出力
//   返り値 : 設定したオプション項目数（0=設定なし, -1=エラー）
//=====================================================================
int MultLogoOptionOrgFile(MLOGO_DATASET* pml){
	char buf[FILE_BUFSIZE];
	char* tmpstr;
	int nlen, ncount;

	//--- get full-path filename ---
	if (MultLogo_GetModuleFileName(buf, FILE_BUFSIZE) == 0){
		return -1;
	}

	//--- get path ---
	tmpstr = strrchrj(buf, DELIMITER_DIR);
	if (tmpstr != NULL){
		*(tmpstr+1) = '\0';			// get only path
	}
	else{
		strcpy(buf, "");			// when no path found
	}

	//--- set path + ini-filename ---
	nlen = strlen(buf) + strlen(INIFILE_NAME);
	if (nlen >= FILE_BUFSIZE){
		return -1;
	}
	strcat(buf, INIFILE_NAME);

	//--- set options ---
	ncount = MultLogoOptionFile(pml, buf);

	return ncount;
}



//##### 初期化・終了処理

//=====================================================================
// Public関数：起動時に１回だけ呼び出す
// 変数初期化処理
//=====================================================================
void MultLogoInit(MLOGO_DATASET* pml){
	int i;

	pml->dispoff     = 0;
	pml->paramoff    = 0;
	pml->num_deflogo = 0;

	pml->oanum   = DEF_MLOGO_OANUM;
	pml->oasel   = 0;
	pml->oamask  = 0;
	pml->outform = 0;

	pml->opt_logofilename  = NULL;
	pml->opt_logoparamfile = NULL;
	pml->opt_outfile       = NULL;
	pml->opt_anafile       = NULL;
	pml->opt_ana2file      = NULL;
	for(i=0; i<LOGONUM_MAX; i++){
		pml->all_logofilename[i] = NULL;
	}
	for(i=0; i<LOGONUM_MAX; i++){
		pml->fpo_ana2[i]          = NULL;
		pml->all_logodata[i]      = NULL;
	}
	pml->logoresult.num_find = 0;
	for(i=0; i<LOGONUM_MAX; i++){
		pml->total_valid[i]   = 0;
		pml->total_frame[i]   = 0;
		pml->priority_list[i] = 0;
	}
	pml->num_detect  = 0;
	pml->num_others  = 0;
	pml->num_disable = 0;
	MultLogo_ParamClear(&(pml->thres_arg));
}


//=====================================================================
// Public関数：終了時に１回だけ呼び出す
// 領域確保メモリの解放処理
//=====================================================================
void MultLogoFree(MLOGO_DATASET* pml){
	int i;

	// デバッグ用ファイルは通常クローズ済みであるが異常終了時に使用
	for(i = 0; i < LOGONUM_MAX; i++){
		if (pml->fpo_ana2[i])
	        fclose(pml->fpo_ana2[i]);
	        pml->fpo_ana2[i] = NULL;
	}

	// ロゴ関連のメモリ解放
	for(i = 0; i < LOGONUM_MAX; i++){
		if (pml->all_logodata[i] != NULL){
		    LogoFree( pml->all_logodata[i] );
		    free( pml->all_logodata[i] );
		    pml->all_logodata[i] = NULL;
		}
	}
	// 設定ファイル名のメモリ解放
	if (pml->opt_logofilename != NULL){
		free( pml->opt_logofilename );
		pml->opt_logofilename = NULL;
	}
	if (pml->opt_logoparamfile != NULL){
		free( pml->opt_logoparamfile );
		pml->opt_logoparamfile = NULL;
	}
	if (pml->opt_outfile != NULL){
		free( pml->opt_outfile );
		pml->opt_outfile = NULL;
	}
	if (pml->opt_anafile != NULL){
		free( pml->opt_anafile );
		pml->opt_anafile = NULL;
	}
	if (pml->opt_ana2file != NULL){
		free( pml->opt_ana2file );
		pml->opt_ana2file = NULL;
	}
	for(i = 0; i < LOGONUM_MAX; i++){
		if (pml->all_logofilename[i] != NULL){
			free( pml->all_logofilename[i] );
			pml->all_logofilename[i] = NULL;
		}
	}
}



//##### 検出開始前の設定処理

//---------------------------------------------------------------------
// 拡張子前までを完全に含み拡張子も同じファイルを展開し、ロゴ登録する
//  入力
//   pml->opt_logofilename  : "-logo"オプションファイル名
//  出力
//   pml->all_logofilename  : 各ロゴファイル名（mallocにより領域確保）
//   返り値                 : 0=正常, 2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogoSetup_ExpandFilename(MLOGO_DATASET* pml){
	int i, idst, ibase, fin_flag;
	int errnum;
	const char* basename;
	char* strbase;
	char* baselist_filename[FILELISTNUM_MAX];

	errnum = 0;
	basename = pml->opt_logofilename;
	if (basename != NULL){
		for(i=0; i<FILELISTNUM_MAX; i++){
			baselist_filename[i] = NULL;
		}
		// get filename list
		errnum = MultLogo_FileListGet(baselist_filename, basename);
		if (errnum == 0){
			// add the list to all_logofilename[]
			fin_flag = 0;
			ibase = 0;
			while(ibase < FILELISTNUM_MAX && fin_flag == 0){
				strbase = baselist_filename[ibase];
				if (strbase == NULL){
					fin_flag = 1;
				}
				else{
					idst = -1;
					for(i = 0; i < LOGONUM_MAX; i++){
						if (pml->all_logofilename[i] == NULL){
							if (idst < 0){                    // if 1st NULL point
								idst = i;                     // set point
							}
						}
						else if (!stricmp(strbase, pml->all_logofilename[i])){
							idst = LOGONUM_MAX;               // disable if exist
						}
					}
					if (idst == LOGONUM_MAX){                 // skip by disable
						fprintf(stderr, "already defined logo:%s\n", strbase);
					}
					else if (idst >= pml->oanum){                  // until oanum option
						fprintf(stderr, "too many logo found(skip):%s\n", strbase);
					}
					else if (idst >= 0 && idst < pml->oanum){ // set filename
						MultLogo_FileStrGet( &(pml->all_logofilename[idst]), strbase );
					}
				}
				ibase ++;
			}
		}
		// ファイル名展開用に保持したメモリ解放
		for(i = 0; i < FILELISTNUM_MAX; i++){
			if (baselist_filename[i] != NULL){
				free( baselist_filename[i] );
			}
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// 各ロゴファイルを初期化
// ロゴファイル名が定義されているロゴのみ領域を確保する
//  出力
//   返り値 : 0=正常 2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogoSetup_EachInit(MLOGO_DATASET* pml){
	int i;

	pml->num_deflogo = 0;
	for(i = 0; i < LOGONUM_MAX; i++){
		pml->all_logodata[i] = NULL;
		if (pml->all_logofilename[i] != NULL){
			if (strlen(pml->all_logofilename[i]) > 0){
				pml->all_logodata[i] = (LOGO_DATASET*)malloc(sizeof(LOGO_DATASET));
				if (pml->all_logodata[i] == NULL){
					fprintf(stderr, "error:failed in memory allocation.\n");
					return 2;
				}
				LogoInit( pml->all_logodata[i] );
				pml->num_deflogo ++;
			}
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// ファイルから閾値パラメータを読み込む
//  入力
//   fname : ファイル名
//  出力
//   pmt    : 取得用仮領域の閾値パラメータ
//   返り値 : 0=正常
//---------------------------------------------------------------------
int MultLogoSetup_ParamRead_File(MLOGO_THRESREC *pmt, const char* fname, int dispoff){
	FILE* fpr;
    int pos, wlen;
    char buf[FILE_BUFSIZE];
    char str[FILE_BUFSIZE];

	// set from param-file
	fpr = fopen(fname, "r");
	if (!fpr){
//        fprintf(stderr, "warning:not found '%s'\n", fname);
	}
	else{
		while( fgets(buf, FILE_BUFSIZE, fpr) != NULL ){
	        if (buf[0] != '#'){
				pos = 0;
				wlen = 0;
				while( buf[pos] == ' ' ) pos++;
				while( buf[pos] > ' ' || buf[pos] < 0 ){
					str[wlen++] = buf[pos++];
				}
				str[wlen] = '\0';
				while( buf[pos] == ' ' ) pos++;
				if ( buf[pos] > ' ' || buf[pos] < 0 ){
					MultLogo_ParamPreSet( pmt, str, &buf[pos] );
				}
			}
		}
		fclose(fpr);
		if (dispoff == 0){
	        printf("info:read parameter from '%s'\n", fname);
	    }
	}
	return 0;
}

//---------------------------------------------------------------------
// 各ロゴの閾値パラメータを設定
//  出力
//   返り値 : 0=正常  2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogoSetup_ParamRead(MLOGO_DATASET* pml){
	MLOGO_THRESREC thres_file;
	LOGO_DATASET* plogo;
	const char* logofilename;
	char* newstr;
	char* tmpstr;
	int i, dispoff;

	dispoff = (pml->dispoff > 0 || pml->paramoff > 0)? 1: 0;

	// read parameter (common)
	if (pml->opt_logoparamfile != NULL){
		MultLogo_ParamClear( &thres_file );
		MultLogoSetup_ParamRead_File( &thres_file, pml->opt_logoparamfile, dispoff );

		for(i = 0; i < LOGONUM_MAX; i++){
			plogo = pml->all_logodata[i];
			if (plogo != NULL){
				MultLogo_ParamCopy( &(plogo->thresdat), &thres_file );
			}
		}
	}
    // read parameter (each)
	for(i = 0; i < LOGONUM_MAX; i++){
		logofilename  = pml->all_logofilename[i];
		plogo         = pml->all_logodata[i];
		if (plogo != NULL && logofilename != NULL){
			newstr = (char *)malloc(
						(strlen(logofilename) + strlen(EXTNAME_LOGOPARAM) + 1)
						 * sizeof(char) );
			if (newstr == NULL){
				fprintf(stderr, "error:failed in memory allocation.\n");
				return 2;
			}
			strcpy(newstr, logofilename);
			tmpstr = strstr(newstr, EXTNAME_LOGODATA);
			if (tmpstr != NULL){
				strcpy(tmpstr, EXTNAME_LOGOPARAM);
			}
			else{
				strcat(newstr, EXTNAME_LOGOPARAM);
			}
			MultLogo_ParamClear( &thres_file );
			MultLogoSetup_ParamRead_File( &thres_file, newstr, dispoff );
			MultLogo_ParamCopy( &(plogo->thresdat), &thres_file );
			if (newstr != NULL){
				free(newstr);
			}
		}
	}
	// set parameter from input option
	for(i = 0; i < LOGONUM_MAX; i++){
		logofilename  = pml->all_logofilename[i];
		plogo = pml->all_logodata[i];
		if (plogo != NULL && logofilename != NULL){
			MultLogo_ParamCopy( &(plogo->thresdat), &(pml->thres_arg) );
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// 各ロゴデータのフレーム数に対応した領域を初期化
//  入力
//   num_frames : 読み込む画像データのフレーム数
//  出力
//   返り値 : 0=正常  2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogoSetup_FrameInit(MLOGO_DATASET* pml, int num_frames){
	LOGO_DATASET* plogo;
	int i, errnum;

	// initialize framedata
	for(i = 0; i < LOGONUM_MAX; i++){
		plogo      = pml->all_logodata[i];
		if (plogo != NULL){
		    errnum = LogoFrameInit( plogo, num_frames );
		    if (errnum > 0){
		        return 2;
		    }
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// 各ロゴデータ読み込み
//  入力
//   pml->all_logofilename : 各ロゴファイル名
//  出力
//   pml->all_logodata     : 各ロゴデータ
//   返り値 : 0=正常  1=ファイルエラー  2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogoSetup_LogoRead(MLOGO_DATASET* pml){
	const char*   logofilename;
	LOGO_DATASET* plogo;
	int errnum;

	// read logodata
	for(int i = 0; i < LOGONUM_MAX; i++){
		logofilename  = pml->all_logofilename[i];
		plogo      = pml->all_logodata[i];
		if (plogo != NULL){
		    errnum = LogoRead( logofilename, plogo );
		    if (errnum > 0){
				fprintf(stderr, "error filename:%s\n", logofilename);
		        return errnum;
		    }
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// "-oa2"オプションで指定するデバッグ用毎フレーム情報ファイルオープン
//  入力
//   pml->opt_ana2file : ファイル名
//  出力
//   返り値 : 0=正常  1=ファイルエラー  2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogoSetup_FileAna2Open(MLOGO_DATASET* pml){
	LOGO_DATASET* plogo;
	const char* ana2file;
	char* newstr;
	char* newstr1;
	char* newstr2;
	char* tmpstr;
	int i, ret;

	ret = 0;
	if (pml->opt_ana2file != NULL){
		ana2file = pml->opt_ana2file;		// newstr : 文字列確保"_100"
		newstr  = (char *)malloc( (strlen(ana2file) + 4 + 1) * sizeof(char) );
		newstr1 = (char *)malloc( (strlen(ana2file)     + 1) * sizeof(char) );
		newstr2 = (char *)malloc( (strlen(ana2file)     + 1) * sizeof(char) );
		if (newstr == NULL || newstr1 == NULL || newstr2 == NULL){
			fprintf(stderr, "error:failed in memory allocation.\n");
			ret = 2;
		}
		else{
			strcpy(newstr1, ana2file);
			tmpstr = strrchr(newstr1, '.');
			if (tmpstr != NULL){
				strcpy(newstr2, tmpstr);		// newstr2 : 拡張子部分
				strcpy(tmpstr, "");				// newstr1 : 拡張子を除いた部分
			}
			else{
				strcpy(newstr2, "");
			}
			for(i = 0; i < LOGONUM_MAX; i++){
				plogo      = pml->all_logodata[i];
				if (plogo != NULL){
					if (pml->num_deflogo == 1){		// ロゴが１つだけなら従来通り
						strcpy(newstr, ana2file);
					}
					else{						// ロゴが２つ以上なら名前に番号を追加
						sprintf(newstr, "%s_%d%s", newstr1, i+1, newstr2);
					}
					pml->fpo_ana2[i] = fopen(newstr, "w");
					if (!pml->fpo_ana2[i]){
						fprintf(stderr, "error: failed to create/open '%s'\n", newstr);
						ret = 1;
						break;
					}
				}
			}
        }
        if (newstr2 != NULL) free(newstr2);
        if (newstr1 != NULL) free(newstr1);
        if (newstr  != NULL) free(newstr);
    }
    return ret;
}


//=====================================================================
// Public関数：開始前の設定初期化
// 設定初期化を行う
//  入力
//   num_frames : 読み込む画像データのフレーム数
//  出力
//   返り値 : 0=正常  1=エラー  2=メモリ確保エラー  3=ロゴデータなし
//=====================================================================
int MultLogoSetup(MLOGO_DATASET* pml, int num_frames){
	int ret;

	pml->image_frames = num_frames;

	ret = MultLogoSetup_ExpandFilename(pml);	// ロゴファイル名を展開
	if (ret == 0){					// 使用する各ロゴファイル領域を初期化
		ret = MultLogoSetup_EachInit(pml);
		if (pml->num_deflogo == 0){				// ロゴ定義なし
			fprintf(stderr, "warning:no logo definition found.\n");
			ret = 3;
		}
	}
	if (ret == 0){					// 閾値パラメータ読み込み
		ret = MultLogoSetup_ParamRead(pml);
	}
	if (ret == 0){					// ロゴデータ読み込み
		ret = MultLogoSetup_LogoRead(pml);
	}
	if (ret == 0){					// フレーム数に対応した領域確保・初期化
		ret = MultLogoSetup_FrameInit(pml, num_frames);
	}
	if (ret == 0){					// "-oa2"デバッグ用毎フレーム情報オープン
		ret = MultLogoSetup_FileAna2Open(pml);
	}
	return ret;
}



//=====================================================================
// Public関数：ロゴパラメータ情報を表示
//=====================================================================
void MultLogoDisplayParam(MLOGO_DATASET* pml){
	LOGO_DATASET* plogo;
	char str_autofade[11];
	char str_offwidth[11];
	int i;

	for(i = 0; i < LOGONUM_MAX; i++){
		plogo      = pml->all_logodata[i];
		if (plogo != NULL){
			// set auto fade
			if (plogo->thresdat.auto_fade > 0){
				strcpy(str_autofade, "autofade=1");
			}
			else{
				strcpy(str_autofade, "");
			}
			if (plogo->thresdat.num_offwidth != 0 &&
				plogo->thresdat.num_onwidth != plogo->thresdat.num_offwidth){
				sprintf(str_offwidth, "(%d)", plogo->thresdat.num_offwidth);
			}
			else{
				strcpy(str_offwidth, "");
			}
			// display
			printf("logo%d:%s\n", i+1, pml->all_logofilename[i]);
#ifdef DEBUG_PRINT
			printf("logo%d:loc(%d,%d) %dx%d Edge:%ld(%d) Sum:(%ld,%ld)-(%ld,%ld)%ld/%ld(%d/%d)\n",
				i+1,
				plogo->paramdat.yx, plogo->paramdat.yy,
				plogo->paramdat.yw, plogo->paramdat.yh,
				plogo->paramdat.total_dif, plogo->paramdat.most_logo_y,
				plogo->paramdat.area[0].xmin, plogo->paramdat.area[0].ymin,
				plogo->paramdat.area[0].xmax, plogo->paramdat.area[0].ymax,
				plogo->paramdat.area[0].total_area1, plogo->paramdat.area[0].total_area2,
				plogo->paramdat.thres_dp_y, plogo->paramdat.scale_area2 );
#endif

			if (plogo->thresdat.auto_bs11 == 2){
				printf("detect:BS11\n");
			}
			printf("params fadein:%d fadeout:%d mrgleft:%d mrgright:%d %s\n", 
				plogo->thresdat.num_fadein,  plogo->thresdat.num_fadeout,
				plogo->thresdat.num_cutleft, plogo->thresdat.num_cutright,
				str_autofade );
			printf("       onwidth:%d%s onlevel:%d Y:%d-%d Yedge:%d Ydif:%d Yoffedg:%d\n",
				plogo->thresdat.num_onwidth, str_offwidth,
				plogo->thresdat.num_onlevel,
				plogo->thresdat.thres_ymin, plogo->thresdat.thres_ymax,
				plogo->thresdat.thres_yedge, plogo->thresdat.thres_ydif,
				plogo->thresdat.thres_yoffedg );
		}
	}
}



//##### １フレーム毎の実行処理

//=====================================================================
// Public関数：画像１枚に対するロゴ検出を行う
// １枚の画像データのロゴ有無を取得
//   data      : 画像データ輝度値へのポインタ
//   pitch     : 画像データの１行データ数
//   nframe    : フレーム番号
//   height    : 画像データの高さ（ロゴデータが画像内か判断のみに使用）
//=====================================================================
void MultLogoCalc(MLOGO_DATASET* pml, const BYTE *data, int pitch, int nframe, int height){
	LOGO_DATASET* plogo;
	int i;

	for(i = 0; i < LOGONUM_MAX; i++){
		plogo      = pml->all_logodata[i];
		if (plogo != NULL){
			// 画像外メモリアクセスで落ちることを防ぐために追加
			if ((plogo->paramdat.yx + plogo->paramdat.yw <= pitch+8 &&
				 plogo->paramdat.yy + plogo->paramdat.yh <  height-1) ||
			    (plogo->paramdat.yx + plogo->paramdat.yw <  pitch   &&
				 plogo->paramdat.yy + plogo->paramdat.yh <  height  )){
		        LogoCalc( plogo, data, pitch, nframe );
		        if (pml->fpo_ana2[i] != NULL){     // for debug
					LogoWriteFrameParam( plogo, nframe, pml->fpo_ana2[i] );
				}
			}
		}
	}
}



//##### 全フレーム読み込み後の検出処理

//---------------------------------------------------------------------
// "-oa2"オプションで指定するデバッグ用毎フレーム情報ファイルクローズ
//---------------------------------------------------------------------
void MultLogoFind_FileAna2Close(MLOGO_DATASET* pml){
	int i;

	for(i = 0; i < LOGONUM_MAX; i++){
		if (pml->fpo_ana2[i])
	        fclose(pml->fpo_ana2[i]);
	        pml->fpo_ana2[i] = NULL;
	}
}

//---------------------------------------------------------------------
// ロゴ結果全体からの判別処理
// ロゴ結果から不要とするロゴを判別して無効化、必要なロゴは期間を合算
//  出力
//    pml->logoresult    : 検出ロゴまとめ表示期間
//    pml->priority_list : 検出フレームが多い順のリスト
//  （pml->total_valid[ロゴ番号-1] = 0 設定による不要ロゴ無効化）
//---------------------------------------------------------------------
void MultLogoFind_TotalResult(MLOGO_DATASET* pml){
	LOGO_DATASET* plogo;
	int i, j, k, ins;
	int oanum, autosel;
	int n_detect, n_others, n_disable;
	short isel, jsel;
	short prior_detect[LOGONUM_MAX];			// CM検出有効ロゴ優先順位
	short prior_others[LOGONUM_MAX];			// CM検出外ロゴ優先順位
	short prior_disable[LOGONUM_MAX];			// 無効判断ロゴ優先順位

	oanum = pml->oanum;
	// sort logo priority
	for(i = 0; i < LOGONUM_MAX; i++){
		pml->priority_list[i] = (short) i;
	}
	if (oanum > 1 && oanum <= LOGONUM_MAX){
		for(i = oanum-1; i > 0; i--){
			for(j = i-1; j >= 0; j--){
				isel = pml->priority_list[i];
				jsel = pml->priority_list[j];
				if (pml->total_frame[isel] > pml->total_frame[jsel]){
					pml->priority_list[i] = jsel;
					pml->priority_list[j] = isel;
				}
			}
		}
	}
	// add as result data
	LogoResultInit( &(pml->logoresult) );
	if (oanum > 0){
		if (pml->oasel == 0){       // option "-oasel 0"
			autosel = 1;
		}
		else{
			autosel = 0;
		}
		for(i = 0; i < LOGONUM_MAX; i++){
			isel     = pml->priority_list[i];
			plogo    = pml->all_logodata[isel];
			if (pml->total_valid[isel] == 0){
			}
			else if (isel < oanum){             // within -oanum option logo
				if (i == 0 || pml->oasel != 1){ // 優先順位が一番高いか全部調査時
					ins = LogoResultAdd( &(pml->logoresult), plogo, autosel );
					if (ins == 0){                  // ロゴ追加を行わない場合
						pml->total_valid[isel] = 0; // invalidate logo
					}
				}
				else{                               // ロゴ追加を行わない場合
					pml->total_valid[isel] = 0;     // invalidate logo
				}
			}
		}
	}
	// set each kind of logo priority
	n_detect = 0;
	n_others = 0;
	n_disable = 0;
	for(i=0; i < LOGONUM_MAX; i++){
		isel = pml->priority_list[i];
		if (pml->total_frame[isel] > 0){
			if (pml->total_valid[isel] == 0){
				prior_disable[n_disable++] = isel;	// 無効判断ロゴ
			}
			else if (isel >= oanum){
				prior_others[n_others++] = isel;	// CM検出外ロゴ
			}
			else{
				prior_detect[n_detect++] = isel;	// CM検出有効ロゴ
			}
		}
	}
	// CM検出有効ロゴ - CM検出外ロゴ - 無効判断ロゴ の順に並び替え
	k = 0;
	for(i=0; i<n_detect; i++){
		pml->priority_list[k++] = prior_detect[i];
	}
	for(i=0; i<n_others; i++){
		pml->priority_list[k++] = prior_others[i];
	}
	for(i=0; i<n_disable; i++){
		pml->priority_list[k++] = prior_disable[i];
	}
	for(i=k; i<LOGONUM_MAX; i++){
		pml->priority_list[k++] = 0;
	}
	pml->num_detect = n_detect;
	pml->num_others = n_others;
	pml->num_disable = n_disable;
}


//=====================================================================
// Public関数：全画像検出完了後に実行してロゴ表示区間を検出
// ロゴ期間を検出（全ロゴ検出し、必要なロゴ期間合算も実行）
//=====================================================================
void MultLogoFind(MLOGO_DATASET* pml){
	LOGO_DATASET* plogo;
	int i;

	// find logoframe
	for(i = 0; i < LOGONUM_MAX; i++){
		plogo      = pml->all_logodata[i];
		if (plogo != NULL){
			LogoFind( plogo );
			pml->total_frame[i] = LogoGetTotalFrameWithClear( plogo );
			if (pml->total_frame[i] > 0){
				pml->total_valid[i] = 1;
			}
		}
	}
	// 全体結果処理
	MultLogoFind_TotalResult( pml );

	//"-oa2"デバッグ用毎フレーム情報をクローズ
	MultLogoFind_FileAna2Close( pml );
}



//##### 結果のファイル出力

//---------------------------------------------------------------------
// "-o"オプションで指定するファイルを出力
//  出力
//   返り値 : 0=正常  1=ファイルエラー
//---------------------------------------------------------------------
int MultLogoWrite_OutputAvs(MLOGO_DATASET* pml){
	LOGO_DATASET* plogo;
	const char* logofilename;
	FILE* fpout;
	int i;

	// open files
	fpout = NULL;
	if (pml->opt_outfile != NULL){
		fpout = fopen(pml->opt_outfile, "w");
		if (!fpout){
			fprintf(stderr, "error: failed to create/open '%s'\n", pml->opt_outfile);
			return 1;
		}
	}

	// output for "-o" option
	if (fpout != NULL){
		for(i = 0; i < LOGONUM_MAX; i++){
			logofilename  = pml->all_logofilename[i];
			plogo         = pml->all_logodata[i];
			if (logofilename != NULL && pml->total_valid[i] > 0){
				LogoWriteOutput( plogo, logofilename, fpout, pml->outform );
			}
		}
	}

	// close file
	if (fpout){
		fclose(fpout);
	}

	return 0;
}


//---------------------------------------------------------------------
// 各個別ロゴの"-oa"オプションに対応するファイルのリストを出力
// 検出ロゴ数、各ロゴについてロゴ名、検出合計期間、元のロゴ番号を出力
//---------------------------------------------------------------------
void MultLogoWrite_OutputAnaEach_List(MLOGO_DATASET* pml, FILE* fpo_list){
	int i, nid, nmax_loop;
	short isel;
	char stradd[2];

	fprintf(fpo_list, "[logodata]\n");
	fprintf(fpo_list, "LogoTotalN=%d\n", pml->num_detect);
	fprintf(fpo_list, "LogoTotalS=%d\n", pml->num_others);
	fprintf(fpo_list, "LogoTotalX=%d\n", pml->num_disable);
	fprintf(fpo_list, "FrameTotal=%d\n", (int) pml->image_frames);
	nmax_loop = pml->num_detect + pml->num_others + pml->num_disable;
	for(i=0; i < nmax_loop; i++){
		isel = pml->priority_list[i];
		if (i < pml->num_detect){
			nid = i + 1;
			strcpy(stradd, "N");
		}
		else if (i < pml->num_detect + pml->num_others){
			nid = i - pml->num_detect + 1;
			strcpy(stradd, "S");
		}
		else{
			nid = i - (pml->num_detect + pml->num_others) + 1;
			strcpy(stradd, "X");
		}
		fprintf(fpo_list, "LogoName_%s%d=%s\n",
				stradd, nid, pml->all_logofilename[isel]);
		fprintf(fpo_list, "FrameSum_%s%d=%d\n",
				stradd, nid, (int) pml->total_frame[isel]);
		fprintf(fpo_list, "OrgLogoNum_%s%d=%d\n",
				stradd, nid, isel+1);
	}
}


//---------------------------------------------------------------------
// 各個別ロゴの"-oa"オプションに対応するファイルを出力
//  出力
//   返り値 : 0=正常  1=ファイルエラー  2=メモリ確保エラー
//---------------------------------------------------------------------
int MultLogoWrite_OutputAnaEach(MLOGO_DATASET* pml){
	LOGO_DATASET* plogo;
	const char* anafile;
	FILE* fpo_list;
	FILE* fpo_ana;
	char* newstr;
	char* newstr1;
	char* newstr2;
	char* tmpstr;
	char stradd[2];
	int i, ret;
	int nmax_loop, nid;
	short isel;

	ret = 0;
	anafile = pml->opt_anafile;
	// output each frame result (each logo result of "-oa" option)
	if (anafile != NULL){					// newstr : "_list.ini"領域確保
		newstr  = (char *)malloc( (strlen(anafile) +10 + 1) * sizeof(char) );
		newstr1 = (char *)malloc( (strlen(anafile)     + 1) * sizeof(char) );
		newstr2 = (char *)malloc( (strlen(anafile)     + 1) * sizeof(char) );
		if (newstr == NULL || newstr1 == NULL || newstr2 == NULL){
			fprintf(stderr, "error:failed in memory allocation.\n");
			ret = 2;
		}
		else{
			// 拡張子前をnewstr1、拡張子をnewstr2 に代入
			strcpy(newstr1, anafile);
			tmpstr = strrchr(newstr1, '.');
			if (tmpstr != NULL){
				strcpy(newstr2, tmpstr);
				strcpy(tmpstr, "");
			}
			else{
				strcpy(newstr2, "");
			}
			// 個別ロゴ情報リストを出力
			if ((pml->oamask & 8) == 0){
				// 出力するファイル名を newstr に代入（必要文字数mallocで事前確保）
				sprintf(newstr, "%s_list.ini", newstr1);
				fpo_list = fopen(newstr, "w");
				if (!fpo_list){
					fprintf(stderr, "error: failed to create/open '%s'\n", newstr);
					ret = 1;
				}
				else{	// INIヘッダ、その他情報を出力
					MultLogoWrite_OutputAnaEach_List(pml, fpo_list);
				}
			}
			else{
				fpo_list = NULL;
			}

			// 各ロゴデータの表示期間を出力
			nmax_loop = pml->num_detect + pml->num_others + pml->num_disable;
			for(i=0; i < nmax_loop; i++){
				// ロゴ種類と番号を取得
				isel = pml->priority_list[i];
				if (i < pml->num_detect){
					nid = i + 1;
					strcpy(stradd, "");
					if ((pml->oamask & 4) != 0){
						nid = -1;
					}
				}
				else if (i < pml->num_detect + pml->num_others){
					nid = i - pml->num_detect + 1;
					strcpy(stradd, "S");
					if ((pml->oamask & 2) != 0){
						nid = -1;
					}
				}
				else{
					nid = i - (pml->num_detect + pml->num_others) + 1;
					strcpy(stradd, "X");
					if ((pml->oamask & 1) != 0){
						nid = -1;
					}
				}
				// ロゴ情報出力
				plogo      = pml->all_logodata[isel];
				if (plogo != NULL && nid > 0){
					// 出力するファイル名を newstr に代入（必要文字数mallocで事前確保）
					sprintf(newstr, "%s_%s%d%s", newstr1, stradd, nid, newstr2);
					fpo_ana = fopen(newstr, "w");
					if (!fpo_ana){
						fprintf(stderr, "error: failed to create/open '%s'\n", newstr);
						ret = 1;
						break;
					}
					LogoWriteFind( plogo, fpo_ana );
					fclose(fpo_ana);

					// 出力したファイル名を個別ロゴ情報リストに追加する処理
					if (fpo_list != NULL){
						if (strlen(stradd) == 0){
							strcpy(stradd, "N");
						}
						fprintf(fpo_list, "oaFileName_%s%d=%s\n",
								stradd, nid, newstr);
					}
				}
			}
			if (fpo_list != NULL){
			    fclose(fpo_list);
			}
		}
		if (newstr2 != NULL) free(newstr2);
		if (newstr1 != NULL) free(newstr1);
		if (newstr  != NULL) free(newstr);
	}
	return ret;
}


//---------------------------------------------------------------------
// "-oa"オプションで指定するファイルを出力
//  出力
//   返り値 : 0=正常  1=ファイルエラー
//---------------------------------------------------------------------
int MultLogoWrite_OutputAnaTotal(MLOGO_DATASET* pml){
	const char* anafile;
	FILE* fpo_ana;

	// output frame result ("-oa" option)
	anafile = pml->opt_anafile;
	if (anafile != NULL){
		fpo_ana = fopen(anafile, "w");
		if (!fpo_ana){
			fprintf(stderr, "error: failed to create/open '%s'\n", anafile);
			return 1;
		}
		LogoResultWrite( &(pml->logoresult), fpo_ana );
		fclose(fpo_ana);
	}
	return 0;
}


//=====================================================================
// Public関数：結果ファイル出力
// 最終結果をファイルへ出力
//  出力
//   返り値 : 0=正常  1=ファイルエラー 2=メモリ確保エラー
//=====================================================================
int MultLogoWrite(MLOGO_DATASET* pml){
	int errnum, retval;

	retval = 0;
	// AVS形式ロゴ情報出力 "-o"オプション
	errnum = MultLogoWrite_OutputAvs(pml);
	if (errnum != 0){
		retval = errnum;
	}

	// 合算ロゴ検出結果を出力 "-oa"オプション
	errnum = MultLogoWrite_OutputAnaTotal(pml);
	if (errnum != 0){
		retval = errnum;
	}

	// 各ロゴ検出結果を出力 各個別ロゴの"-oa"オプション
	errnum = MultLogoWrite_OutputAnaEach(pml);
	if (errnum != 0){
		retval = errnum;
	}

	return retval;
}


