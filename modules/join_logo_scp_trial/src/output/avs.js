const fs = require("fs-extra");
async = require("async");

const createfile = (inputfiles, outputfile) =>{
  return new Promise((resolve, reject) => {
    const writer = fs.createWriteStream(outputfile, {encoding: 'utf-8'});
    async.mapSeries(inputfiles, (fn, callback) => {
      const reader = fs.createReadStream(fn, {encoding: 'utf-8'});
      reader.on("end", () => {
        reader.unpipe();
        callback();
      });
      reader.pipe(writer, { end: false });
    }, () => {
      // finally, close the WritableStream
      writer.end();
      console.log(`create_done: ${outputfile}`);
      resolve();
    });
  });
};

exports.create = async (inputFile) => {
  const { OUTPUT_AVS_CUT, 
          LOGOFRAME_AVS_OUTPUT, 
          OUTPUT_AVS_IN_CUT,
          OUTPUT_AVS_IN_CUT_LOGO 
        } = require("../settings");
  try {
    await createfile([inputFile, OUTPUT_AVS_CUT], OUTPUT_AVS_IN_CUT);
    await createfile([inputFile, LOGOFRAME_AVS_OUTPUT, OUTPUT_AVS_CUT], OUTPUT_AVS_IN_CUT_LOGO);
  } catch (e) {
    console.error(e);
    process.exit(-1);
  }
  return;
};

