const spawnSync = require("child_process").spawnSync;

const { TSDIVIDER_COMMAND, TSDIVIDER_OUTPUT } = require("../settings");

exports.exec = filename => {
  const args = ["-i", filename,"--overlap_front",0 , "-o", TSDIVIDER_OUTPUT];
  try {
    spawnSync(TSDIVIDER_COMMAND, args, { stdio: "inherit" });
  } catch (e) {
    console.error(e);
    process.exit(-1);
  }
};
