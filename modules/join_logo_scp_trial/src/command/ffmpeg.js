const spawnSync = require("child_process").spawnSync;
const path = require("path");
const fs = require("fs-extra");

const {
  FFMPEG_COMMAND,
  OUTPUT_AVS_IN_CUT,
  OUTPUT_AVS_IN_CUT_LOGO,
} = require("../settings");

exports.exec = (save_dir, save_name, target, ffoption) => {
  const args = ["-y", "-i"];

  if (target == "cutcm") {
    args.push(OUTPUT_AVS_IN_CUT);
  }else{
    args.push(OUTPUT_AVS_IN_CUT_LOGO);
  }
  if (ffoption) {
    const option_args=ffoption.split(' ');
    for(let i = 0; i < option_args.length; i++){
      if(option_args[i]){
        args.push(option_args[i]);
      } 
    }
  }
  args.push(path.join(save_dir,`${save_name}.mp4`));
  //console.log(args);
  try {
    spawnSync(FFMPEG_COMMAND, args, { stdio: "inherit" });
  } catch (e) {
    console.error(e);
    process.exit(-1);
  }
};
