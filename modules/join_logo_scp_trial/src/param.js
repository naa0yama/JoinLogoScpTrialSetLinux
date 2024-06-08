const csv = require("csv/lib/sync");
const fs = require("fs-extra");
const path = require("path");
const jaconv = require("jaconv");

const { PARAM_LIST_1, PARAM_LIST_2, OBS_PARAM_PATH } = require("./settings");

const SymRExp = /[\.\*\+\?\|\[\]\^]/;

const search = (paramList, channel, filename) => {
  const result = {};
  const short = channel ? channel.short : "__normal";

  for (param of paramList) {
    let match_flag = false;
    // コメント行は処理しない
    if (param.channel.match(/^#/)) {
      continue;
    }
    //ファイル名とパラメータを変換
    //全角英数記号→半角
    //半角カナ    →全角
    const normal_filename = jaconv.normalize(filename);
    const normal_param_title = jaconv.normalize(param.title);
    
    // 放送局の一致確認
    const channel_flag = short == param.channel ? true : false;
    //titleが指定されているか
    const title_flag = param.title != '' ? true : false;

    if (channel_flag && title_flag){//放送局一致かつタイトル指定あり
      if (SymRExp.test(param.title)){
        //正規表現が含まれているときは正規表現でチェックする
        regexp = new RegExp(`${normal_param_title}`); //正規表現を作成
        const matchTitle = normal_filename.match(regexp);  //正規表現確認
        match_flag = matchTitle ? true : false;
      }else{
      //タイトルがファイル名に含まれているかどうか
      match_flag = normal_filename.indexOf(normal_param_title) != -1 ? true : false ;
      }

    }else if (channel_flag){
      //タイトル指定なしで放送局はマッチした
      match_flag = true;
    }

    if (match_flag) {
      for (key of Object.keys(param)) {
        if (param[key] === "@") {
          result[key] = "";
        } else if (param[key] !== "") {
          result[key] = param[key];
        }
      }
    }
  }
  //一致するものがなければ1行目を見る（JL_標準）
  if (Object.keys(result) == 0) {
    //JLparam_set2は1行目がコメントなのでコメントだった時はスルー
    if (!paramList[0].channel.match(/^#/)) {
      for (key of Object.keys(paramList[0])) {
        if (paramList[0][key] === "@") {
          result[key] = "";
        } else if (paramList[0][key] !== "") {
          result[key] = paramList[0][key];
        }
      }
    }
  }
  return result;
};

exports.parse = (channel, filepath) => {
  let result = {};
  for (file of [PARAM_LIST_1, PARAM_LIST_2]) {
    const data = fs.readFileSync(file);
    const paramList = csv.parse(data, {
      from: 2,
      columns: [
        "channel",
        "title",
        "JLOGO_CMD",
        "JL_FLAGS",
        "JLOGO_OPT1",
        "JLOGO_OPT2",
        "JLOGO_NOLOGO",
        "LOGOSUBHEAD",
        "use_tssplit",
        "use_intools",
        "tffbff",
        "comment1",
        "comment2"
      ]
    });
    const param = search(paramList, channel, path.basename(filepath));
    result = Object.assign(result, param);
  }

  //利用JLとチャンネルを適当に保存しておく
  fs.outputJsonSync(OBS_PARAM_PATH, Object.assign(result,channel));
  return result;
};
