const childProcess = require('child_process');
const path = require("path");

const {
	JL_DIR,
	JLSCP_COMMAND,
	LOGOFRAME_TXT_OUTPUT,
	CHAPTEREXE_OUTPUT,
	JLSCP_OUTPUT,
	OUTPUT_AVS_CUT
} = require("../settings");

exports.exec = param => {
	return new Promise((resolve, reject) => {
		let args = ["-inlogo",
			LOGOFRAME_TXT_OUTPUT,
			"-inscp",
			CHAPTEREXE_OUTPUT,
			"-incmd",
			path.join(JL_DIR, param.JL_RUN),
			"-o",
			OUTPUT_AVS_CUT,
			"-oscp",
			JLSCP_OUTPUT,
			"-flags",
			param.FLAGS
		];

		if (param.OPTIONS && param.OPTIONS !== "") {
			args = args.concat(param.OPTIONS.split(" "));
		}

		const child = childProcess.spawn(JLSCP_COMMAND, args);
		child.on('exit', (code) => {
			if (code === 0) {
				resolve();
			} else {
				console.error(`${JLSCP_COMMAND.split('/').pop()} command failed with exit code: ${code}`);
				reject(new Error(`${JLSCP_COMMAND.split('/').pop()} exited with code ${code}`));
				process.exit(code || 1);
			}
		});
		child.stderr.on('data', (data) => {
			console.error(`${JLSCP_COMMAND.split('/').pop()} ` + data);
		});
	})
};
