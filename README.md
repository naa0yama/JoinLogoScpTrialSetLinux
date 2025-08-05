# JoinLogoScpTrialSet for Linux and Avisynth+3.7.x

## 概要

[sogaani][1]氏が移植された[Linux対応版join_logo_scp][2]を元に改造し  
Native Linux に対応した[AviSynth+ 3.7.x][3]で動作できるようにしたもののセット。  
DockerとDocker-composeを用いて動作させます。  

[1]:https://github.com/sogaani
[2]:https://github.com/sogaani/JoinLogoScp
[3]:https://github.com/AviSynth/AviSynthPlus

### フォークによる改造

- [x] [yobibi/join_logo_scp at v5.1.0](https://github.com/yobibi/join_logo_scp/tree/v5.1.0) への対応
- [x] `-c` フラグを立てると EPGStation の放送局名(`CHNNELNAME`)を取得する事でロゴ検索を高速化
  - [x] `-1` などの末尾数字は最も大きい物を利用するため世代管理でも問題ありません。
  - [x] これにより Amatsukaze で生成されたロゴデータをそのまま利用出来ます。
- [x] L-SMASH-Works の対応がどうも FFmpeg 6.1.1 までの様子のため modules 郡用の FFmpeg は 6.1.1 を、エンコード用はFFmpeg 7.x となるようにしています。そのため FFmpeg 6.1.1 の `ffmpeg` コマンド自体は生成していません。
- [x] 弊宅で動作が確認出来た, Dockerfile を追加しました
  - [x] Ubuntu 24.04
- [x] `--addchapter` CMカット後のチャプターを追加するオプション追加

## 動作確認用環境セットアップ方法

このセットアップには Docker が必要です。  
あくまで、動作確認用ですので使い込みたい方は自前で Dockerfile を作成することや、他の Dockerfile に組み込むことを検討してください。
初回は次の通りに実行します。

```bash
git clone https://github.com/naa0yama/JoinLogoScpTrialSetLinux.git
cd JoinLogoScpTrialSetLinux

docker pull ghcr.io/naa0yama/joinlogoscptrialsetlinux:ubuntu2404

docker run --user $(id -u):$(id -g) --rm -it \
  -v $PWD/videos/source:/source \
  -v $PWD/modules/join_logo_scp_trial/JL:/join_logo_scp_trial/JL \
  -v $PWD/modules/join_logo_scp_trial/logo:/join_logo_scp_trial/logo \
  -v $PWD/modules/join_logo_scp_trial/result:/join_logo_scp_trial/result \
  -v $PWD/modules/join_logo_scp_trial/setting:/join_logo_scp_trial/setting \
  -v $PWD/modules/join_logo_scp_trial/src:/join_logo_scp_trial/src \
  ghcr.io/naa0yama/joinlogoscptrialsetlinux:ubuntu2404 /bin/bash

```

logoフォルダが生成されていると思うので、そこにロゴデータを入れておきます。

## 使用方法

- ソースになる ts ファイルを `videos/source` に配置します。

上記の `docker run --user $(id -u):$(id -g) --rm -it` でログインした Bash 上で下記 find コマンドを実行すれば  
`videos/source` 内のファイルを順番に処理し libx264 でエンコードされたファイルが出来ると思います。

```bash
find /source -type f -name '*.ts' -exec env INPUT="{}" \
  jlse --input "{}" \
  --encode --option ' -vf yadif -map 0:v -aspect 16:9 -c:v libx264 -preset veryfast -movflags faststart -f mp4 -map 0:a -c:a aac -bsf:a aac_adtstoasc' \;

```

`modules/join_logo_scp_trial/result` フォルダの中のファイル名のフォルダに解析結果と、カット用の avs が保存されます。  
join_logo_scp_trialの詳しい使用方法は、[こちら][5]を確認してください。

[5]:https://github.com/tobitti0/join_logo_scp_trial/blob/master/README.md

### EPGStationで使用する

LinuxなEPGStationでDocker環境の場合の導入方法は[こちら][6]

[6]:https://tobitti.net/blog/Ubuntu-EPGStation-JoinLogoScpTrial/

（私はEPGStationで呼び出し、CM解析をし、ロゴ消し、CMカット、エンコードまで動作させています。）  
Dockerで動作しているEPGStationを利用していますが、動作にはHOMEの環境変数が必須です。  
ないとchapter_exe,logoframe,join_logo_scpから、avsファイルを見つけることができず動作しません。  
Dockerでの動作しか確認していませんが、spawnする際に次のようにすることで動作します。  

```js
var env = Object.create( process.env );
env.HOME = '/root';
const child = spawn('jlse', jlse_args, {env: env});

```

（Dockerで動作させていない場合は `HOME` の値は異なると思います。Dockerだといじっていなければ `root` です。）

### ファイル構成

```text
* logoframe           : 透過ロゴ表示区間検出 ver1.16（要AviSynth環境）
* chapter_exe         : 無音＆シーンチェンジ検索chapter_exeの改造版（要AviSynth環境）
* join_logo_scp       : ロゴと無音シーンチェンジを使ったCM自動カット位置情報作成
* join_logo_scp_trial : join_logo_scp動作確認用スクリプト

```

## 開発

### Dockerfile での開発

基本的には `dev-build.sh` にまとめてあり、 `bash dev-build.sh --tag ubuntu2404` を実行することで生成と jlse の実行が行われます

```bash
# 通常
bash dev-build.sh --tag ubuntu2404

# build-stage を指定する場合
bash dev-build.sh --tag ubuntu2404 --target build-jlstsl

# SHELL に入りたい場合
bash dev-build.sh --tag ubuntu2404 --shell

# ROOT で SHELL に入りたい場合
bash dev-build.sh --tag ubuntu2404 --shell --root

```

### Claude Code の導入

```bash
npm config set prefix=$HOME/.npm_global
npm install -g @anthropic-ai/claude-code

echo "export PATH=\$PATH:\$HOME/.npm_global/bin/" >> ~/.bashrc
exec $SHELL -l

```

## 謝辞

各種ツールを作成された方々、  
Linuxに移植されたsogaani氏、ツール群をユースケースと一緒に公開された tobitti0氏
に深く感謝いたします。

## License

本レポジトリに含むライセンスは下記の通りです。  
本レポジトリは GPL-3.0 とし、オリジナルの部分は下記表のとおりとします。

| Source                                                                   | License         |
| :----------------------------------------------------------------------- | :-------------- |
| [tobitti0/JoinLogoScpTrialSetLinux][7]                                   | Unknown         |
| [tobitti0/chapter_exe][8]                                                | GPL-2.0 license |
| [yobibi/join_logo_scp][9]                                                | GPL-2.0 license |
| [tobitti0/join_logo_scp_trial][10]<br>[Till0196/join_logo_scp_trial][14] | Unknown         |
| [tobitti0/logoframe][11]                                                 | GPL-2.0 license |
| [tobitti0/tsdivider][12]                                                 | GPL-3.0 license |
| [tobitti0/delogo-AviSynthPlus-Linux][13]                                 | GPL-2.0 license |

[7]:https://github.com/tobitti0/JoinLogoScpTrialSetLinux
[8]:https://github.com/tobitti0/chapter_exe
[9]:https://github.com/yobibi/join_logo_scp
[10]:https://github.com/tobitti0/join_logo_scp_trial
[11]:https://github.com/tobitti0/logoframe
[12]:https://github.com/tobitti0/tsdivider
[13]:https://github.com/tobitti0/delogo-AviSynthPlus-Linux
[14]:https://github.com/Till0196/join_logo_scp_trial
