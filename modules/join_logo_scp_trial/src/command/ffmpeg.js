const spawnSync = require("child_process").spawnSync;
const path = require("path");
const fs = require("fs-extra");

const {
	FFMPEG_COMMAND,
	FILE_TXT_CPT_CUT,
	OUTPUT_AVS_IN_CUT,
	OUTPUT_AVS_IN_CUT_LOGO,
} = require("../settings");

const __HALF_WIDTH_NAME = process.env.HALF_WIDTH_NAME;
const __HALF_WIDTH_DESCRIPTION = process.env.HALF_WIDTH_DESCRIPTION;
const __HALF_WIDTH_EXTENDED = process.env.HALF_WIDTH_EXTENDED;

exports.exec = (save_dir, save_name, target, ffoption, addchapter) => {
	const args = ["-hide_banner", "-y", "-ignore_unknown", "-i"];

	if (target == "cutcm") {
		args.push(OUTPUT_AVS_IN_CUT);
	} else {
		args.push(OUTPUT_AVS_IN_CUT_LOGO);
	}

	// --- Chapter input 追加
	if (addchapter) {
		args.push("-i");
		args.push(FILE_TXT_CPT_CUT);
		args.push("-map_metadata");
		args.push("1");

		/** metadata
		 * 0: OUTPUT_AVS_IN_CUT   avs file
		 * 1: FILE_TXT_CPT_CUT    obs_chapter_cut.chapter.txt
		 */
		args.push("-metadata", "title=" + `${__HALF_WIDTH_NAME}`);
		args.push("-metadata", "comment="
			+ '\=\=\= Description\n'
			+ `${__HALF_WIDTH_DESCRIPTION}\n`
			+ '\n'
			+ '\=\=\= Extended\n'
			+ `${__HALF_WIDTH_EXTENDED}\n`
		);
		args.push("-movflags", "+use_metadata_tags");
		// --- End
	}

	if (ffoption) {
		const option_args = ffoption.split(' ');
		for (let i = 0; i < option_args.length; i++) {
			if (option_args[i]) {
				args.push(option_args[i]);
			}
		}
	}
	args.push(path.join(save_dir, `${save_name}.mp4`));
	console.log('ffmpeg.js[\'args\']= ' + args);
	try {
		spawnSync(FFMPEG_COMMAND, args, { stdio: "inherit" });
	} catch (e) {
		console.error(e);
		process.exit(-1);
	}
};
