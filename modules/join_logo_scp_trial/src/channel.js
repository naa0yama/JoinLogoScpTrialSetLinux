const csv = require("csv/lib/sync");
const fs = require("fs-extra");
const path = require("path");
const jaconv = require("jaconv");

const { CHANNEL_LIST } = require("./settings");

exports.parse = filepath => {
  const data = fs.readFileSync(CHANNEL_LIST);
  const channelList = csv.parse(data, {
    from: 2,
    columns: ["recognize", "install", "short"]
  });
  const filename = jaconv.normalize(path.basename(filepath));
  let result = null;
  let priority = 0;
  for (channel of channelList) {
    const recognize = jaconv.normalize(channel.recognize);
    const short = jaconv.normalize(channel.short);

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

  return result;
};
