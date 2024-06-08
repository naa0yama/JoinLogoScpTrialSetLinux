const path = require("path");
const uuid = require("uuid/v4")();
const fs = require("fs-extra");

exports.CHANNEL_LIST = path.join(__dirname, "../setting/ChList.csv");
exports.PARAM_LIST_1 = path.join(__dirname, "../setting/JLparam_set1.csv");
exports.PARAM_LIST_2 = path.join(__dirname, "../setting/JLparam_set2.csv");

exports.LOGOFRAME_COMMAND = path.join(__dirname, "../bin/logoframe");
exports.CHAPTEREXE_COMMAND = path.join(__dirname, "../bin/chapter_exe");
exports.TSDIVIDER_COMMAND = path.join(__dirname, "../bin/tsdivider");
exports.JLSCP_COMMAND = path.join(__dirname, "../bin/join_logo_scp");
exports.FFPROBE_COMMAND = "/usr/local/bin/ffprobe";
exports.FFMPEG_COMMAND = "/usr/local/bin/ffmpeg";

exports.JL_DIR = path.join(__dirname, "../JL");
exports.LOGO_PATH = path.join(__dirname, "../logo");

OUTPUT_FOLDER_DIR = path.join(__dirname, `../result`);

exports.init = filename=> {
  const save_dir = path.join(OUTPUT_FOLDER_DIR, filename);
  exports.SAVE_DIR = save_dir;
  fs.ensureDirSync(save_dir);
  exports.INPUT_AVS = path.join(save_dir, "in_org.avs");
  exports.TSDIVIDER_OUTPUT = path.join(save_dir, `${filename}_split.ts`);

  exports.CHAPTEREXE_OUTPUT = path.join(save_dir, "obs_chapterexe.txt");

  exports.LOGOFRAME_TXT_OUTPUT = path.join(save_dir,"obs_logoframe.txt");
  exports.LOGOFRAME_AVS_OUTPUT = path.join(save_dir,"obs_logo_erase.avs");

  exports.OBS_PARAM_PATH = path.join(save_dir, "obs_param.txt");
  exports.JLSCP_OUTPUT = path.join(save_dir, "obs_jlscp.txt");
  exports.OUTPUT_AVS_CUT = path.join(save_dir, "obs_cut.avs");

  exports.OUTPUT_AVS_IN_CUT = path.join(save_dir, "in_cutcm.avs");
  exports.OUTPUT_AVS_IN_CUT_LOGO = path.join(save_dir, "in_cutcm_logo.avs");
  exports.OUTPUT_FILTER_CUT = path.join(save_dir, "ffmpeg.filter");

  exports.FILE_TXT_CPT_ORG = path.join(save_dir, "obs_chapter_org.chapter.txt");
  exports.FILE_TXT_CPT_CUT = path.join(save_dir, "obs_chapter_cut.chapter.txt");
  exports.FILE_TXT_CPT_TVT = path.join(save_dir, "obs_chapter_tvtplay.chapter");
  return this;
};


