const fs = require("fs-extra");
const readline = require("readline");

exports.create = async (settings) => {
  const { OUTPUT_AVS_CUT, 
          JLSCP_OUTPUT,
          FILE_TXT_CPT_ORG,
          FILE_TXT_CPT_CUT,
          FILE_TXT_CPT_TVT
        } = settings;
  try {
    //前提ファイルの有無確認
    if (!fs.existsSync(OUTPUT_AVS_CUT)||!fs.existsSync(JLSCP_OUTPUT)) { 
      process.exit(-1);
    }
    // Trimデータの読み込み
    let nItemTrim = await TrimReader(OUTPUT_AVS_CUT);
    // Chapterデータをobs_jlscpから生成する
    let ChapterData = await CreateChapter(nItemTrim, JLSCP_OUTPUT);
    //console.log(ChapterData);
    // Chapterデータからファイルを出力する
    OutputData(ChapterData, 0, FILE_TXT_CPT_ORG);
    OutputData(ChapterData, 1, FILE_TXT_CPT_CUT);
    OutputData(ChapterData, 2, FILE_TXT_CPT_TVT);
  } catch (e) {
    console.error(e);
    process.exit(-1);
  }
  return;
};

//--------------------------------------------------
// Trimによるカット情報読み込み
// 読み込みデータ。開始位置を表すため終了位置では＋１する。
// nItemTrim : Trim位置情報（単位はフレーム）
//--------------------------------------------------
function TrimReader(file){
  let ReadStream;
  let Reader;
  let nItemTrim =[];
  return new Promise((resolve, reject) => {
    //--- ファイル読み込み ---
    ReadStream = fs.createReadStream(file);
    Reader = readline.createInterface({ input: ReadStream });
    const strRegTrim = 'Trim\\((\\d+)\,(\\d+)\\)'
    Reader.on("close", () => {
      resolve(nItemTrim);
    });
    Reader.on("line", (data) => {
      for(let r of data.match(RegExp(strRegTrim,'g'))){
        nItemTrim.push(Number(r.match(new RegExp(strRegTrim))[1]));
        nItemTrim.push(Number(r.match(new RegExp(strRegTrim))[2]) + 1);
      }
    });
  })
}

//--------------------------------------------------
// 構成解析ファイルとカット情報からCHAPTERを作成
//  (input)
//    trim:TRIMデータ
//    file:obs_jlscpファイルパス    
//  (output)
//    ChapterCata:生成したCHAPTER
//--------------------------------------------------
function CreateChapter(trim, file){
  //--- CHAPTER情報取得に必要な変数 ---
  let clsChapter
  let bCutOn, bShowOn, bShowPre, bPartExist
  let nTrimNum, nType, nLastType, nPart
  let nFrmTrim, nFrmSt, nFrmEd, nFrmMgn, nFrmBegin
  let nSecRd, nSecCalc
  let strCmt, strChapterName, strChapterLast
  //--- CHAPTER情報格納用変数 ---
  var ChapterData = {
                  m_nMSec:[],
                  m_bCutOn:[],
                  m_strName:[]
                };
  let nItemTrim = trim;
  let nTrimTotal = nItemTrim.length;

  return new Promise((resolve, reject) => {
    //--- ファイルオープン ---
    let ReadStream = fs.createReadStream(file);
    let Reader = readline.createInterface({ input: ReadStream });

    //--- trimパターン ---
    const strRegJls  = /^\s*(\d+)\s+(\d+)\s+(\d+)\s+([-\d]+)\s+(\d+).*:(\S+)/
    //--- 初期設定 ---
    nFrmMgn    = 30;  // Trimと読み込み構成を同じ位置とみなすフレーム数
    bShowOn    = 1;   // 最初は必ず表示
    nTrimNum   = 0;   // 現在のTrim位置番号
    nFrmTrim   = 0;   // 現在のTrimフレーム
    nLastType  = 0;   // 直前状態クリア
    nPart      = 0;   // 初期状態はAパート
    bPartExist = 0;   // 現在のパートは存在なし
    nFrmBegin  = 0;   // 次のchapter開始地点

    //--- 開始地点設定 ---
    // nTrimNum が偶数：次のTrim開始位置を検索
    // nTrimNum が奇数：次のTrim終了位置を検索
    if(nTrimTotal > 0){
      if(nItemTrim[0] <= nFrmMgn){    // 最初の立ち上がりを0フレームと同一視
        nTrimNum = 1;
      }
    }else{
      nTrimNum =1;
    }

    //--- 構成情報データを順番に読み出し ---
    Reader.on("line", (data) => {
      matches = data.match(strRegJls);
      if(matches.length > 0 ){
        //--- 読み出しデータ格納 ---
        nFrmSt = Number(matches[1]);  // 開始フレーム
        nFrmEd = Number(matches[2]);  // 終了フレーム
        nSecRd = Number(matches[3]);  // 期間秒数
        strCmt = matches[6];          // 構成コメント

        //--- 現在検索中のTrim位置データ取得 ---
        if(nTrimNum < nTrimTotal){
          nFrmTrim = nItemTrim[nTrimNum];
        }

        //--- 現構成終了位置より手前にTrim地点がある場合の設定処理 ---
        while((nFrmTrim < (nFrmEd - nFrmMgn)) && (nTrimNum < nTrimTotal)){
          bCutOn  = (nTrimNum+1) % 2;   // Trimのカット状態（１でカット）
          //--- CHAPTER文字列取得処理 ---
          [nType, nSecCalc] = ProcChapterTypeTerm(nFrmBegin, nFrmTrim);
          [strChapterName, bPartExist] = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecCalc);
          //--- CHAPTER挿入処理 ---
          InsertFrame(ChapterData, nFrmBegin, bCutOn, strChapterName);
          nFrmBegin = nFrmTrim;     // chapter開始位置変更
          nTrimNum = nTrimNum + 1;  // Trim番号を次に移行
          if(nTrimNum < nTrimTotal){
              nFrmTrim = nItemTrim[nTrimNum];   // 次のTrim位置検索に変更
          }
        }

        //--- 現構成位置の判断開始 ---
        bShowPre = 0;
        bShowOn = 0;
        bCutOn  = (nTrimNum+1) % 2;   // Trimのカット状態（１でカット）
        //--- 現終了位置にTrim地点があるか判断（あればCHAPTER表示確定） ---
        if ((nFrmTrim <= (nFrmEd + nFrmMgn)) && (nTrimNum < nTrimTotal)){
          nFrmEd  = nFrmTrim;         // Trim位置にフレームを変更
          bShowOn = 1;                // 表示を行う
          nTrimNum = nTrimNum + 1;    // Trim位置を次に移行
        }

        //--- コメントからCHAPTER表示種類を判断 ---
        // nType 0:スルー 1:CM部分 10:独立構成 11:part扱いにしない独立構成
        nType = ProcChapterTypeCmt(strCmt, nSecRd);
        //--- CHAPTER区切りを確認（前回と今回の構成で区切るか判断） ---
        if (bCutOn != 0){           // カットする部分
          if (nType == 1){          // 明示的なCM時
            if (nLastType != 1){    // 前回CM以外だった場合表示
              bShowPre = 1;         // 前回終了（今回開始）にchapter表示
            }
          }else{                    // 明示的なCM以外
            if (nLastType == 1){    // 前回CMだった場合表示
              bShowPre = 1;         // 前回終了（今回開始）にchapter表示
            }
          }
        }

        //--- CHAPTER挿入（前回終了位置） ---
        if (bShowPre > 0 || nType >= 10){       // 位置確定のフラグ確認
          if (nFrmBegin < (nFrmSt - nFrmMgn)){  // chapter開始位置が今回開始より前
            if (nLastType != 1){                // 前回CM以外の時は種類再確認
              [nLastType, nSecCalc] = ProcChapterTypeTerm(nFrmBegin, nFrmSt);
            }
            //--- CHAPTER名文字列を決定し挿入 ---
            [strChapterLast, bPartExist, nPart] = ProcChapterName(bCutOn, nLastType, nPart, bPartExist, nSecCalc);
            InsertFrame(ChapterData, nFrmBegin, bCutOn, strChapterLast);
            nFrmBegin = nFrmSt;     // chapter開始位置を今回開始位置に
          }
        }
        //--- CHAPTER挿入（現終了位置） ---
        if (bShowOn > 0 || nType >= 10){
          if (nFrmEd > (nFrmBegin + nFrmMgn)){    // chapter開始位置が今回終了より前
            //--- CHAPTER名文字列を決定し挿入 ---
            [strChapterName,bPartExist, nPart] = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecRd);
            InsertFrame(ChapterData, nFrmBegin, bCutOn, strChapterName);
            nFrmBegin = nFrmEd;   // chapter開始位置を今回終了位置に
          }
        }

        //--- 次回確認用の処理 ---
        nLastType = nType;
      }
    });
    Reader.on("close", () => {
      //--- Trim位置の出力完了していない場合の処理 ---
      while(nTrimNum < nTrimTotal){
        nFrmTrim = nItemTrim[nTrimNum];
        //--- Trim位置をchapterへ出力 ---
        bCutOn  = (nTrimNum+1) % 2;                  // Trimのカット状態（１でカット）
        [nType, nSecCalc] = ProcChapterTypeTerm(nFrmBegin, nFrmTrim);
        [strChapterName,bPartExist, nPart] = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecCalc);
        //--- CHAPTER挿入処理 ---
        InsertFrame(ChapterData, nFrmBegin, bCutOn, strChapterName);
        nTrimNum = nTrimNum + 1;                           // Trim番号を次に移行
      }
      //--- 最終chapterの出力 ---
      if (nFrmBegin < (nFrmEd - nFrmMgn)){
        bCutOn  = (nTrimNum+1) % 2;                  // Trimのカット状態（１でカット）
        [nType, nSecCalc] = ProcChapterTypeTerm(nFrmBegin, nFrmEd);
        [strChapterName,bPartExist, nPart] = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecCalc);
        //--- CHAPTER挿入処理 ---
        InsertFrame(ChapterData, nFrmBegin, bCutOn, strChapterName);
      }

      resolve(ChapterData);
    });
  })
}

//---------------------------------------------
// CHAPTER情報をファイルに出力
//  (input)
//    ChapterDate:チャプターデータ
//    nCutType : 1)MODE_ORG / 2)MODE_CUT / 3)MODE_TVT / 4)MODE_TVC
//    file:出力先
//---------------------------------------------
function OutputData(ChapterData, nCutType, file){
  //定数
  const PREFIX_TVTI = "ix"     // カット開始時文字列（tvtplay用）
  const PREFIX_TVTO = "ox"     // カット終了時文字列（tvtplay用）
  const PREFIX_ORGI = ""       // カット開始時文字列（カットなしchapter）
  const PREFIX_ORGO = ""       // カット終了時文字列（カットなしchapter）
  const PREFIX_CUTO = ""       // カット終了時文字列（カット後）
  const SUFFIX_CUTO = ""       // カット終了時末尾追加文字列（カット後）

  const MODE_ORG = 0
  const MODE_CUT = 1
  const MODE_TVT = 2
  const MODE_TVC = 3
  
  const MSEC_DIVMIN = 100      //チャプター位置を同一としない時間間隔（msec単位）
  let i, inext;
  let bSkip;
  let strName;

  let nSumTime = 0; // 現在の位置（ミリ秒単位）
  let nCount    = 1            // CHAPTER出力番号
  let bCutState = 0            // 前回の状態（0:非カット用 1:カット用）
  let m_strOutput = ""         // 出力
  //--- tvtplay用初期文字列 ---
  if (nCutType == MODE_TVT || nCutType == MODE_TVC){
    m_strOutput = "c-";
  }

  //--- CHAPTER設定数だけ繰り返し ---
  inext = 0;
  for (i in ChapterData.m_nMSec){
    //console.log(i);
    //--- 次のCHAPTERと重なっている場合は除く ---
    bSkip = 0;
    if (inext > i){
        bSkip = 1;
    }else{
      inext = Number(i) + 1;
      if (inext < (ChapterData.m_nMSec.length - 1)){
        if (ChapterData.m_nMSec[inext + 1] - ChapterData.m_nMSec[inext] < MSEC_DIVMIN ){
          inext = inext + 1;
        }
      }
    }
    if (bSkip == 0){
      //--- 全部表示モードorカットしない位置の時に出力 ---
      if ((nCutType == MODE_ORG) || (nCutType == MODE_TVT) || (ChapterData.m_bCutOn[i] == 0)){
        //--- 最初が0でない時の補正 ---
        if ((nCutType == MODE_ORG) || (nCutType == MODE_TVT)){
          if ((i == 0) && (ChapterData.m_nMSec[i] > 0)){
            nSumTime = nSumTime + ChapterData.m_nMSec[i];
          }
        }
        //--- tvtplay用 ---
        if((nCutType == MODE_TVT) || (nCutType == MODE_TVC)){
          //--- CHAPTER名を設定 ---
          if(nCutType == MODE_TVC){   // カット済み
            if ((bCutState > 0) && (ChapterData.m_bCutOn[i] == 0)){   // カット終了
              strName = ChapterData.m_strName[i] + SUFFIX_CUTO;
            }else{
              strName = ChapterData.m_strName[i];
            }
          }else if ((bCutState == 0) && (ChapterData.m_bCutOn[i] > 0)){ // カット開始
            strName = PREFIX_TVTI + ChapterData.m_strName[i];
          }else if ((bCutState > 0) && (ChapterData.m_bCutOn[i] == 0)){ // カット終了
            strName = PREFIX_TVTO + ChapterData.m_strName[i];
          }else{
            strName = ChapterData.m_strName[i];
          }
          strName = strName.replace(/-/g, '－');
          //--- tvtplay用CHAPTER出力文字列設定 ---
          m_strOutput = m_strOutput + nSumTime + 'c' + strName + '-';
        }else{          //--- 通常のchapter用 ---
          //--- CHAPTER名を設定 ---
          if ((bCutState == 0 ) && (ChapterData.m_bCutOn[i]>0)){    // カット開始
            strName = PREFIX_ORGI + ChapterData.m_strName[i];
          }else if ((bCutState > 0) && (ChapterData.m_bCutOn[i] == 0)){   // カット終了
            if (nCutType == MODE_CUT){
              strName = PREFIX_CUTO + ChapterData.m_strName[i] + SUFFIX_CUTO;
            }else{
              strName = PREFIX_ORGO + ChapterData.m_strName[i];
            }
          }else{
            strName = ChapterData.m_strName[i];
          }
          //--- CHAPTER出力文字列設定 ---
          m_strOutput += GetDispChapter(i, nCount, nSumTime, strName);
        }
        //--- 書き込み後共通設定 ---
        nSumTime  = nSumTime + (ChapterData.m_nMSec[inext] - ChapterData.m_nMSec[i]);
        nCount    = nCount + 1;
      }
      //--- 現CHAPTERに状態更新 ---
      bCutState = ChapterData.m_bCutOn[i];
    }
  }
  //--- tvtplay用最終文字列 ---
  if (nCutType == MODE_TVT){
    if (bCutState > 0){   // CM終了処理
      m_strOutput = m_strOutput + "0e" + PREFIX_TVTO + "-";
    }else{
      m_strOutput = m_strOutput + "0e-";
    }
    m_strOutput = m_strOutput + "c";
  }else if (nCutType == MODE_TVC){
    m_strOutput = m_strOutput + "c";
  }
  //console.log(m_strOutput);
  //--- 結果出力 ---
  fs.writeFile(file, m_strOutput, (err, data) => {
    if(err) console.log(err);
  });
}

//--------------------------------------------------
// フレーム数に対応する秒数取得
//--------------------------------------------------
function ProcGetSec(nFrame){
  //29.97fpsの設定で固定
  return (parseInt((nFrame*1001+30000/2)/30000))
}

//--------------------------------------------------
// Chapter種類を取得（開始終了位置から秒数も取得する）
//   nFrmS  : 開始フレーム
//   nFrmE  : 終了フレーム
//  出力
//   nSecRd : 期間秒数
//   nType  : 0:通常 1:明示的にCM 2:part扱いの判断迷う構成
//            10:単独構成 11:part扱いの判断迷う単独構成 12:空欄
//--------------------------------------------------
function ProcChapterTypeTerm(nFrmS, nFrmE){
  let nType, nSecRd;
  nSecRd = ProcGetSec(nFrmE - nFrmS);
  if(nSecRd == 0){
    nType = 12;
  } else if(nSecRd == 90){
    nType = 11;
  } else if(nSecRd < 15){
    nType = 2;
  } else{
    nType = 0;
  }
  return [nType, nSecRd];
}

//--------------------------------------------------
// Chapter種類を取得（コメント情報を使用する）
//   strCmt : コメント文字列
//   nSecRd : コメントの秒数
//  出力
//   nType  : 0:通常 1:明示的にCM 2:part扱いの判断迷う構成
//            10:単独構成 11:part扱いの判断迷う単独構成 12:空欄
//--------------------------------------------------
function ProcChapterTypeCmt(strCmt, nSecRd){
  let nType;
  //--- CHAPTER表示内容か判断 ---
  // nType  : 0:通常 1:明示的にCM 2:part扱いの判断迷う構成
  //          10:単独構成 11:part扱いの判断迷う単独構成 12:空欄
  if(strCmt.indexOf('Trailer(cut)') >= 0){
    nType = 0;
  }else if(strCmt.indexOf('Trailer') >= 0){
    nType = 10;
  }else if(strCmt.indexOf('Sponsor') >= 0){
    nType = 11;
  }else if(strCmt.indexOf('Endcard') >= 0){
    nType = 11;
  }else if(strCmt.indexOf('Edge') >= 0){
    nType = 11;
  }else if(strCmt.indexOf('Border') >= 0){
    nType = 11;
  }else if(strCmt.indexOf('CM') >= 0){
    nType = 1;    // 15秒単位CMとそれ以外を分ける必要なければ0にする
  }else if(nSecRd == 90){
    nType = 11;
  }else if(nSecRd == 60){
    nType = 10;
  }else if(nSecRd < 15){
    nType = 2;
  }else{
    nType = 0;
  }
  return nType;
}
//--------------------------------------------------
// CHAPTER名の文字列を決める
//   bCutOn : 0=カットしない部分 1=カット部分
//   nType  : 0:通常 1:明示的にCM 2:part扱いの判断迷う構成
//            10:単独構成 11:part扱いの判断迷う単独構成 12:空欄
//   nPart  : Aパートから順番に数字0～（function内で更新あり）
//   bPartExist : part構成の要素があれば2（function内で更新あり）
//   nSecRd     : 単独構成時の秒数
// 戻り値はCHAPTER名
//--------------------------------------------------
function ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecRd){
  let strChapterName;
  if(bCutOn == 0){    //残す部分
    strChapterName = String.fromCharCode('A'.charCodeAt(0)+ (nPart % 23));
    if (nType >= 10){
      strChapterName = strChapterName + nSecRd + "Sec";
    }else{
      strChapterName = strChapterName;
    }
    if (nType == 11 || nType == 2){     // part扱いの判断迷う構成
      if(bPartExist == 0){
        bPartExist = 1;
      }
    }else if(nType != 12){
      bPartExist = 2;
    }
  }else{      // カットする部分
    if (nType >= 10){
      strChapterName = "X" + nSecRd + "Sec";
    }else if (nType == 1){
      strChapterName = "XCM";
    }else{
      strChapterName = "X";
    }
    if (bPartExist > 0 && nType != 12){
      nPart = nPart + 1;
      bPartExist = 0;
    }
  }
  return [strChapterName,bPartExist,nPart];
}

//---------------------------------------------
// CHAPTERに追加（ミリ秒で指定）
// nMSec   : 位置ミリ秒
// bCutOn  : 1の時カット
// strName : chapter表示用文字列
//---------------------------------------------
function InsertMSec(chapterdata, nMSec, bCutOn, strName){
  chapterdata.m_nMSec.push(nMSec);
  chapterdata.m_bCutOn.push(bCutOn);
  chapterdata.m_strName.push(strName);
}

//---------------------------------------------
// CHAPTERに追加（フレーム位置指定）
// nFrame  : フレーム位置
// bCutOn  : 1の時カット
// strName : chapter表示用文字列
//---------------------------------------------
function InsertFrame(chapterdata, nFrame, bCutOn, strName){
  //29.97fpsの設定で固定
  var nTmp = parseInt((nFrame*1001 + 30/2)/30);
  InsertMSec(chapterdata, nTmp, bCutOn, strName);
}

//------------------------------------------------------------
// CHAPTER表示用文字列を１個分作成（m_strOutputに格納）
// num     : 格納chapter通し番号
// nCount  : 出力用chapter番号
// nTime   : 位置ミリ秒単位
// strName : chapter名
//------------------------------------------------------------
function GetDispChapter(num, nCount, nTime, strName){
  let strBuf;
  let strCount, strTime;
  let strHour, strMin, strSec, strMsec;
  let nHour, nMin, nSec, nMsec;

  //--- チャプター番号 ---
  strCount = String(nCount);
  if (strCount.length == 1){
    strCount = '0' + strCount;
  }
  //--- チャプター時間 ---
  nHour = parseInt(nTime / (60*60*1000));
  nMin  = parseInt((nTime % (60*60*1000)) / (60*1000));
  nSec  = parseInt((nTime % (60*1000)) / 1000);
  nMsec = nTime % 1000;
  strHour = ('0' + nHour).slice(-2);
  strMin = ('0' + nMin).slice(-2);
  strSec = ('0' + nSec).slice(-2);
  strMsec = ('00' + nMsec).slice(-3);
  StrTime = strHour + ":" + strMin + ":" + strSec + "." + strMsec;
  //--- 出力文字列（１行目） ---
  strBuf = "CHAPTER" + strCount + "=" + StrTime + '\n';
  //--- 出力文字列（２行目） ---
  strBuf = strBuf + "CHAPTER" + strCount + "NAME=" + strName + '\n';
  return (strBuf);
}

