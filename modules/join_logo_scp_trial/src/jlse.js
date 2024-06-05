#!/usr/bin/env node
const fs = require("fs-extra");
const path = require("path");

const argv = require("yargs")
  .option("input", {
    alias: "i",
    type: "string",
    describe: "path to ts file"
  })
  .option("filter", {
    alias: "f",
    type: "boolean",
    default: false,
    describe: "enable to ffmpeg filter output"
  })
  .option("encode", {
    alias: "e",
    type: "boolean",
    default: false,
    describe: "enable to ffmpeg encode"
  })
  .option("target", {
    alias: "t",
    choices: ["cutcm", "cutcm_logo"],
    default: "cutcm_logo",
    describe: "select encord target"
  })
  .option("option", {
    alias: "o",
    type: "string",
    default: "",
    describe: "set ffmpeg option"
  })
  .option("outdir", {
    alias: "d",
    type: "string",
    default: "",
    describe: "set encorded file dir"
  })
  .option("outname", {
    alias: "n",
    type: "string",
    default: "",
    describe: "set encorded file name"
  })
  .option("remove", {
    alias: "r",
    type: "boolean",
    default: false,
    describe: "remove avs files"
  })
  .demandOption(
    ["input"],
    "Please provide input arguments to work with this tool"
  )
  .check(function(argv) {
    const ext = path.extname(argv.input);
    if (ext !== ".ts") {
      console.error(`invalid file extension ${ext}.`);
      return false;
    }

    try {
      fs.statSync(argv.input);
    } catch (err) {
      console.error(`File ${argv.input} not found.`);
      return false;
    }
    return true;
  })
  .help().argv;

const createAvs = (path, filename, index) => {
  fs.writeFileSync(
    path,
//`LoadPlugin("/usr/local/lib/libffms2.so")
//FFIndex("${filename}")
//FFMpegSource2("${filename}", atrack=-1)`
`TSFilePath="${filename}"
LWLibavVideoSource(TSFilePath, repeat=true, dominance=1)
AudioDub(last,LWLibavAudioSource(TSFilePath, stream_index=${index}, av_sync=true))
`
  );
  return path;
};

const main = async () => {
  const inputFile =  path.resolve(argv.input);
  const inputFileName = path.basename(inputFile, path.extname(inputFile));
  const inputFileDir = path.dirname(inputFile);
  const settings = require("./settings").init(inputFileName);  //settings init
  const parseChannel = require("./channel").parse;
  const parseParam = require("./param").parse;
  const logoframe = require("./command/logoframe").exec;
  const chapterexe = require("./command/chapterexe").exec;
  const tsdivider = require("./command/tsdivider").exec;
  const joinlogoframe = require("./command/join_logo_frame").exec;
  const createFilter = require("./output/ffmpeg_filter").create;
  const createOutAvs = require("./output/avs").create;
  const createChapter = require("./output/chapter_jls").create;
  const encode = require("./command/ffmpeg").exec;
  const { INPUT_AVS, 
          OUTPUT_AVS_CUT, 
          OUTPUT_FILTER_CUT, 
          SAVE_DIR,
          TSDIVIDER_OUTPUT
        } = settings;
  const channel = parseChannel(inputFile);
  const param = parseParam(channel, inputFileName);
  let avsFile = createAvs(INPUT_AVS, inputFile, 1);
  if(param.use_tssplit == 1){
    console.log("TS spliting ...");
    tsdivider(inputFile);
    console.log("TS split done");
    avsFile = createAvs(INPUT_AVS, TSDIVIDER_OUTPUT, -1);
  };

  await chapterexe(avsFile);
  await logoframe(param, channel, avsFile);
  await joinlogoframe(param);

  await createOutAvs(avsFile);
  await createChapter(settings);

  if(argv.filter) {createFilter(inputFile, OUTPUT_AVS_CUT, OUTPUT_FILTER_CUT); }

  if(argv.encode) {
    encode(argv.outdir? argv.outdir : inputFileDir, argv.outname? argv.outname : inputFileName, argv.target, argv.option);
  }
  if(argv.remove) {
    fs.removeSync(SAVE_DIR);
    fs.removeSync(path.join(inputFileDir,`${inputFileName}.ts.lwi`));
  }
};

main();
