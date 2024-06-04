# CM自動カット位置情報作成 join_logo_scp for Linux

## 概要

これは、 [tobitti0氏](https://github.com/tobitti0) が公開していた
[join_logo_scp](https://github.com/tobitti0/join_logo_scp) のファイル構造を参考に [Yobi氏の v5.1.0](https://github.com/yobibi/join_logo_scp/tree/v5.1.0) へ更新したものになります。  

v5.1.0 から Linux での Make に対応した模様なので改変は最小限としています。

## 機能

事前に別ソフトで検出した `ロゴ表示区間`, `無音＆シーンチェンジ` の情報を基にして、CMカット情報(Trim)を記載したAVSファイルを作成します。

## ビルド方法

src で Make してください。  

## 使用方法

````bash
join_logo_scp -inlogo ファイル名 -inscp ファイル名 -incmd ファイル名 -o ファイル名 その他オプション

````

詳細はオリジナルの[readme](readme.txt)を参照してください。

## 謝辞

オリジナルの作成者であるYobi氏  
Linuxに移植されたsogaani氏  
ツール群としてまとめて公開頂いたtobitti0氏  
に深く感謝いたします。
