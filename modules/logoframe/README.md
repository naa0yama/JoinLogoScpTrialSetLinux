# 透過ロゴ表示区間検出 logoframe for AviSynth+ 3.5.x & Linux
## 概要
AviSynth+はVer3.5.0からNative Linuxをサポートした。  
これは[sogaani氏][1]がLinuxに移植された[logoframe][2]をAviSynth+3.5.xで動作するように改造したものである。

[1]:https://github.com/sogaani
[2]:https://github.com/sogaani/JoinLogoScp/tree/master/logoframe

## 機能
動画から固定位置半透明ロゴが表示されている区間を検出します。

特徴
* 自動処理化を目指した高精度なロゴ表示区間の検出
* ロゴ表示開始／終了時のフェードイン／フェードアウト有無を検知

## 使用方法
AviSynth+3.5.xを導入の上、srcフォルダでmakeしてください。  
実行方法は次のとおりです。
````
logoframe AVSファイル名 -logo ロゴファイル名 -oa 出力ファイル名 他オプション
````
詳細はオリジナルの[readme][3]を参照してください。

[3]:https://github.com/tobitti0/logoframe/blob/master/readme.txt

## 謝辞
オリジナルの作成者であるYobi氏  
Linuxに移植されたsogaani氏  
に深く感謝いたします。  
