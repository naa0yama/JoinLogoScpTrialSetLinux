const childProcess = require('child_process');
const path = require("path");
const fs = require("fs-extra");

const {
  LOGOFRAME_COMMAND,
  LOGO_PATH,
  LOGOFRAME_AVS_OUTPUT,
  LOGOFRAME_TXT_OUTPUT,
} = require("../settings");

const getLogo = logoName => {
  let logo = path.join(LOGO_PATH, `${logoName}.lgd`);
  if (fs.existsSync(logo)) {
    return logo;
  }

  logo = path.join(LOGO_PATH, `${logoName}.lgd2`);
  if (fs.existsSync(logo)) {
    return logo;
  }
  return null;
};

const selectLogo = channel => {
  if (!channel) {
    console.log('放送局はファイル名から検出できませんでした');
  }else{
    console.log(`放送局：${channel["short"]}`);
    for (key of ["install", "short", "recognize"]) {
      const logo = getLogo(channel[key]);
      if (logo) {
        return logo;
      }
    }
    console.log("放送局のlgd(lgd2)ファイルが見つかりませんでした");
  }
  console.log("ロゴファイルすべてを入力します");
  return LOGO_PATH;
};

exports.exec = (param, channel, filename) => {
  return new Promise((resolve)=>{
    const args = [filename, "-oa", LOGOFRAME_TXT_OUTPUT, "-o", LOGOFRAME_AVS_OUTPUT];
    const logo = selectLogo(channel);
    let logosub = null;
    if (param.LOGOSUBHEAD) {
      logosub = getLogo(param.LOGOSUBHEAD);
    }

    if (!logosub && !logo) {
      return;
    }

    if (logo) {
      args.push("-logo");
      args.push(logo);
    }

    if (logosub) {
      args.push("-logo99");
      args.push(logosub);
    }

    const child = childProcess.spawn(LOGOFRAME_COMMAND, args);
    child.on('exit', (code)=>{
      resolve();
    });
    child.stderr.on('data', (data)=>{
      console.error("logoframe " + data);
    });
  })
};
