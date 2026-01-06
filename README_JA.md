# Q mini

[![README in English](https://img.shields.io/badge/English-d9d9d9)](./README.md)
[![日本語版 README](https://img.shields.io/badge/日本語-d9d9d9)](./README_JA.md)

![GitHub contributors](https://img.shields.io/github/contributors/TeamSOBY/soby_mini)
![GitHub issues](https://img.shields.io/github/issues/TeamSOBY/soby_mini)
![GitHub fork](https://img.shields.io/github/forks/TeamSOBY/soby_mini)
![GitHub stars](https://img.shields.io/github/stars/TeamSOBY/soby_mini)

![SOBY logo](img/soby_logo03.png)


## 目次
1. [**Title A**](#title-a)
    1. [Sub title A1](#sub-title-a1)
    2. [Sub title A2](#sub-title-a2)

2. [**Title B**](#title-b)
    1. [Sub title B1](#sub-title-b1)
    2. [Sub title B2](#sub-title-b2)

3. [**Title C**](#title-c)
    1. [Sub title C1](#sub-title-c1)
    2. [Sub title C2](#sub-title-c2)

# M5Stack FIRE 動作メモ
M5Stack FIREは，基本的に普通のM5Stackと同じように動作する．

### 基本動作
- 電源ボタン (左側面のボタン) を1回押すとオン(or リセット)，2回押すとオフ．

### ビルド & ラン
- Arduino IDEでコンパイルするには「M5Stack」ライブラリのインストールが必要．
- この際，Arduino IDEの設定は「Tools/Board」は「M5Stack」，「Tools/Port」は「COM3 (任意のUSBポート)」にする．
- 下記に基本的な関数の使い方が記載されている．
  - https://github.com/m5stack/M5Stack/tree/master/examples/Basics

# BALA2 FIRE 動作メモ

### 基本動作
動作モードは下記の3つ (ドキュメントにもちゃんと書いてない)．

- 倒立モード
  - 電源を入れると自動的に倒立する．

- キャリブレーションモード
  - ボタンBを押しながら電源を入れるとキャリブレーションできる．
  - この際，本体を横に寝かせることでキャリブレーションを行う．
  - 詳細：https://docs.m5stack.com/en/guide/hobby_kit/bala2/bala2_quick_start

- 充電モード
  - ボタンCを押しながら電源を入れると，充電を確認できる．
  - 充電が完了するとディスプレイに「Charge completed!」と表示される．

### ビルド & ラン
- M5Stack FIREと同じ手順で，下記をそのままコンパイルすると動く．
- https://github.com/m5stack/M5-ProductExampleCodes/tree/master/Application/Bala2


---

# M5StickC-Plus 動作メモ

### 基本動作
- 電源ボタン (左側面のボタン) を1回押すとオン，6秒押すとオフ．
- 下記に基本的な関数の使い方が記載されている．
  - https://github.com/m5stack/M5StickC-Plus/tree/master/examples/Basics

### ビルド&ラン
- Arduino IDEでコンパイルするには「M5StickC-Plus」ライブラリのインストールが必要．
- この際，Arduino IDEの設定は「Tools/Board」は「M5StickCPlus」，「Tools/Port」は「COM3 (任意のUSBポート)」にする．

# JoyC 動作メモ

### 基本動作
- 左右のスティックの強弱，押し込みを取得可能．
- JoyCのスイッチを左側にすると，Joyスティック周りのLEDが点灯し，JoyCのスイッチを右側にすると，Joyスティック周りのLEDが消灯する．
- JoyCは基板裏のリチウムバッテリで動作．これを充電する際は，JoyCとM5StickC-Plusを接続した状態で，M5StickC-PlusにUSBケーブルを接続する．

### ビルド&ラン
- Arduino IDEでコンパイルするには「M5Hat-JoyC」ライブラリのインストールが必要．
-  下記の「#include <M5StickC.h>」をコメントアウトし，「#include <M5StickCPlus.h>」のコメントアウトを外し， M5StickC-Plusと同じ手順でコンパイルすると動く．
  - https://github.com/m5stack/M5Hat-JoyC/blob/master/examples/GetPosition/GetPosition.ino


## ライセンス
このプロジェクトは〇〇ライセンスのもと公開されています。詳細はLICENSEファイルをご覧ください。

## 貢献方法
誰でもこのプロジェクトに貢献できます！
バグの報告、プルリクエスト、ドキュメントの改善など、お待ちしています。

---

[トップに戻る](#リポジトリテンプレート)

