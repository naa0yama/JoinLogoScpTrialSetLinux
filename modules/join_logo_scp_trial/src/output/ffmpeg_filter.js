const fs = require("fs");
const ffprobe = require("../command/ffprobe");

const MIN_START_FRAME = 30;

exports.create = (tsFile, trimFile, outputFile) => {
  try {
    const trimString = fs.readFileSync(trimFile).toString();
    let result;
    const reg = /Trim\((\d*)\,(\d*)/g;
    const trimFrames = [];

    while ((result = reg.exec(trimString))) {
      const trimFrame = {
        start: result[1] < MIN_START_FRAME ? MIN_START_FRAME : result[1],
        end: result[2] < MIN_START_FRAME ? MIN_START_FRAME : result[2]
      };
      trimFrames.push(trimFrame);
    }

    const fps = ffprobe.getFrameRate(tsFile);

    let filterString = "";
    let concatString = "";
    for (let i = 0; i < trimFrames.length; i++) {
      const trimFrame = trimFrames[i];

      const startTime = parseFloat(
        (trimFrame.start * fps.fpsDenominator) / fps.fpsNumerator
      );
      const endTime = parseFloat(
        (trimFrame.end * fps.fpsDenominator) / fps.fpsNumerator
      );
      filterString += `[0:v]trim=${startTime}:${endTime},setpts=PTS-STARTPTS[v${i}];`;
      filterString += `[0:a]atrim=${startTime}:${endTime},asetpts=PTS-STARTPTS[a${i}];`;
      concatString += `[v${i}][a${i}]`;
    }
    filterString += `${concatString}concat=n=${
      trimFrames.length
    }:v=1:a=1[video][audio];`;

    fs.writeFileSync(outputFile, filterString);
  } catch (e) {
    console.error(e);
    process.exit(-1);
  }
};
