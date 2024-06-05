//====================================================================
// 複数ロゴ処理用ヘッダファイル
//====================================================================

#ifndef ___LOGOSET_MUL_H
#define ___LOGOSET_MUL_H


#define FILE_BUFSIZE 512

// max number of logo data
#define LOGONUM_MAX 100
// "-logo"オプションで取得するファイル名最大数
#define FILELISTNUM_MAX LOGONUM_MAX
// "-oanum"オプションの初期値
#define DEF_MLOGO_OANUM  90


// ロゴデータの拡張子
#define EXTNAME_LOGODATA   ".lgd"
#define EXTNAME_LOGOPARAM  ".logoframe.txt"

// logoframe初期読み込みオプションファイル名
#define INIFILE_NAME       "logoframe.ini"
// logoframe用セクション名
#define SECTION_LOGOFRAME  "[logoframe]"

// フォルダ区切り記号
#ifdef _WIN32
#define DELIMITER_DIR     '\\'
#define DELIMITER_STRDIR  "\\"
#else
#define DELIMITER_DIR     '/'
#define DELIMITER_STRDIR  "/"
#endif

// 閾値パラメータ更新保存用（更新フラグ）
typedef struct {
	char	up_num_fadein;
	char	up_num_fadeout;
	char	up_num_cutleft;
	char	up_num_cutright;
	char	up_num_onwidth;
	char	up_num_onlevel;
	char	up_num_offwidth;
	char	up_thres_ymax;
	char	up_thres_ymin;
	char	up_thres_yedge;
	char	up_thres_ydif;
	char	up_thres_ysetdif;
	char	up_thres_yoffedg;
	char	up_num_areaset;
	char	up_num_clrrate;
	char	up_auto_fade;
	char	up_auto_bs11;
} MLOGO_UPTHRESREC;

// 閾値パラメータ更新保存用
typedef struct {
	LOGO_THRESREC		dat;
	MLOGO_UPTHRESREC	up;
} MLOGO_THRESREC;


// 複数ロゴデータ格納
typedef struct {
	// 呼び出し元でも参照する値
	short dispoff;									// オプション "-nodisp"
	short paramoff;									// オプション "-nodispparam"
	short num_deflogo;								// 定義ロゴ数

	// logoframe複数ロゴ対応で使用するオプション
	short oanum;									// オプション "-oanum"
	short oasel;									// オプション "-oasel"
	short oamask;									// オプション "-oamask"
	short outform;									// オプション "-outform"
	char* opt_logofilename;							// オプション "-logo"
	char* opt_logoparamfile;						// オプション "-logoparam"
	char* opt_outfile;								// オプション "-o"
	char* opt_anafile;								// オプション "-oa"
	char* opt_ana2file;								// オプション "-oa2"
	char* all_logofilename[LOGONUM_MAX];			// ロゴファイル名リスト

	FILE*             fpo_ana2[LOGONUM_MAX];		// デバッグ用ファイル保持用
	LOGO_DATASET*     all_logodata[LOGONUM_MAX];	// 各ロゴデータ格納
	MLOGO_THRESREC    thres_arg;					// 閾値（引数指定）
	LOGO_RESULTOUTREC logoresult;					// 全体検出結果格納

	long  image_frames;								// 画像データフレーム数
	long  total_frame[LOGONUM_MAX];					// ロゴ表示期間フレーム合計
	short total_valid[LOGONUM_MAX];					// 有効ロゴ判別
	short priority_list[LOGONUM_MAX];				// ロゴ優先順位リスト
	short num_detect;								// CM検出有効ロゴ数
	short num_others;								// CM検出外ロゴ数
	short num_disable;								// 無効判断ロゴ数
} MLOGO_DATASET;

#endif
