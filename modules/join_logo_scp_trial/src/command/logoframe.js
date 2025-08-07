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

	// Amatsukaze の SID103-1.lgd のようなロゴファイル名に対応
	// SID103-1.lgd, SID103-2.lgd, ... のように番号が付いている場合、最大の番号のファイルを選択
	if (logoName.startsWith("SID")) {
		let maxNumber = 0;
		const logoFiles = fs.readdirSync(LOGO_PATH);
		for (const file of logoFiles) {
			if (file.startsWith(`${logoName}-`)) {
				const number = parseInt(file.split("-")[1]);
				if (number > maxNumber) {
					maxNumber = number;
				}
			}
		}
		if (maxNumber > 0) {
			logo = path.join(LOGO_PATH, `${logoName}-${maxNumber}.lgd`)
			return logo;
		}
	}

	return null;
};

const selectLogo = channel => {
	if (!channel) {
		console.log('放送局はファイル名から検出できませんでした');
	} else {
		console.log(`放送局：${channel["short"]}`);
		for (key of ["install", "short", "recognize", "serviceid"]) {
			let logoKey = key === "serviceid" ? "SID" + channel[key] : channel[key];
			const logo = getLogo(logoKey);
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
	return new Promise((resolve, reject) => {
		const args = [filename, "-oa", LOGOFRAME_TXT_OUTPUT, "-o", LOGOFRAME_AVS_OUTPUT];
		const logo = selectLogo(channel);
		let logosub = null;
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
		child.on('exit', (code) => {
			if (code === 0) {
				resolve();
			} else {
				console.error(`${LOGOFRAME_COMMAND} command failed with exit code: ${code}`);
				reject(new Error(`${LOGOFRAME_COMMAND} exited with code ${code}`));
				process.exit(code);
			}
		});
		child.stderr.on('data', (data) => {
			console.error(`${LOGOFRAME_COMMAND} ` + data);
		});
	})
};
