const csv = require("csv-parse/sync");
const fs = require("fs-extra");
const path = require("path");
const jaconv = require("jaconv");

const { CHANNEL_LIST } = require("./settings");

exports.parse = (filepath, channelName) => {
	const data = fs.readFileSync(CHANNEL_LIST);
	const channelList = csv.parse(data, {
		from: 2,
		columns: ["recognize", "install", "short", "serviceid"]
	});
	const filename = jaconv.normalize(path.basename(filepath));
	if (channelName !== "") {
		channelName = jaconv.normalize(channelName);
	}
	let result = null;
	let priority = 0;
	let defaultAction = false;
	// 引数のチャンネル名の指定があれば、そちらを優先する
	if (channelName !== "" && !defaultAction) {
		for (channel of channelList) {
			const recognize = jaconv.normalize(channel.recognize);
			const short = jaconv.normalize(channel.short);
			const serviceid = channel.serviceid;
			// 放送局名（認識用）：引数のチャンネル名から前方一致で探す（優先度1）
			if (channelName.match(new RegExp(`^${recognize}`))) {
				result = channel;
				return channel;
			}

			// 放送局略称       ：引数のチャンネル名から前方一致で探す（優先度1）
			if (channelName.match(new RegExp(`^${short}`))) {
				result = channel;
				return channel;
			}

			// サービスID       ：引数のチャンネル名から前方一致で探す（優先度1）
			if (channelName.match(new RegExp(`^${serviceid}`))) {
				result = channel;
				return channel;
			}

			// 放送局名（認識用）：末尾以外で1文字ちょうどの数字が含まれる場合、数字を消して探す（優先度1）
			const channelNameWithoutNumbers = channelName.replace(/(?<!\d)\d(?!\d|$)/g, "");
			if (channelNameWithoutNumbers.match(new RegExp(`^${recognize}`))) {
				result = channel;
				return channel;
			}
		}
		// どれにも合致しなかったらファイル名から探すフラグを立てる
		defaultAction = true;
	}

	// ファイル名から探す
	if (defaultAction || channelName === "") {
		for (channel of channelList) {
			const recognize = jaconv.normalize(channel.recognize);
			const short = jaconv.normalize(channel.short);
			const serviceid = channel.serviceid;

			// 放送局名（認識用）：ファイル名先頭または" _"の後（優先度1）
			let regexp = new RegExp(`^${recognize}| _${recognize}`);
			let match = filename.match(regexp);
			if (match) {
				return channel;
			}

			// 放送局略称       ：ファイル名の先頭、_の後または括弧の後で、略称直後は空白か括弧か"_"（優先度1）
			regexp = new RegExp(`^${short}[_\s]| _${short}| [(〔[{〈《｢『【≪]${short}[)〕\\]}〉》｣』】≫ _]`);
			match = filename.match(regexp);
			if (match) {
				return channel;
			}

			// サービスID       ：ファイル名の先頭、_の後または括弧の後で、略称直後は空白か括弧か"_"（優先度1）
			regexp = new RegExp(`^${serviceid}[_\s]| _${serviceid}| [(〔[{〈《｢『【≪]${serviceid}[)〕\\]}〉》｣』】≫ _]`);
			match = filename.match(regexp);
			if (match) {
				return channel;
			}

			// 放送局名（認識用）：括弧の後（優先度2）
			regexp = new RegExp(`[(〔[{〈《｢『【≪]${recognize}`);
			match = filename.match(regexp);
			if (match) {
				result = channel;
				priority = 2;
				continue;
			}

			// 放送局略称       ：前が"_"、空白のいずれかかつ後が括弧、"_"、空白のいずれか（優先度3）
			if (priority < 3) {
				continue;
			}
			regexp = new RegExp(`[ _]${short}[)〕\\]}〉》｣』】≫ _]`);
			match = filename.match(regexp);
			if (match) {
				result = channel;
				priority = 3;
				continue;
			}

			// サービスID       ：前が"_"、空白のいずれかかつ後が括弧、"_"、空白のいずれか（優先度3）
			if (priority < 3) {
				continue;
			}
			regexp = new RegExp(`[ _]${serviceid}[)〕\\]}〉》｣』】≫ _]`);
			match = filename.match(regexp);
			if (match) {
				result = channel;
				priority = 3;
				continue;
			}

			// 放送局名（認識用）："_"、空白の後（優先度4）
			if (priority < 4) {
				continue;
			}
			regexp = new RegExp(`|_${recognize}| ${recognize}`);
			match = filename.match(regexp);
			if (match) {
				result = channel;
				priority = 4;
			}
		}
	}
	return result;
};
