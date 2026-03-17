# UIAP-psgplayer

## 概要

[CH32V003](https://www.wch-ic.com/products/CH32V003.html) を使用した
[UIAPduino](https://www.uiap.jp/uiapduino/pro-micro/ch32v003/v1dot4)
上で動作する PSG プレーヤーです。

ROM 上のデータとして配置した P6 PSG ドライバ形式の PSG 楽曲データを演奏します。

ビルド環境には Arduino ではなく [`ch32fun`](https://github.com/cnlohr/ch32fun)
を使用しています。

ハードウェア依存バックエンドの実装、演奏データを ROM 上に保持する構成、
画面 UI を持たない点を除き、全体構造は
[`rpi-psgplayer`](https://github.com/tsutsui/rpi-psgplayer) を
概ね流用しています。

つまり、`rpi-psgplayer` と同様の PSG 演奏プログラムを
UIAPduino 上でも動かせないか、という発想で移植したものです。

## 想定ハードウェア構成

https://x.com/tsutsuii/status/2030436745960214937 のポストを参照してください。

回路図を書かずに作ってしまったのですが、後追いで書いた回路図と補足説明を
[hw](hw) のディレクトリ内に入れてあります。

UIAPduino の GPIO と YM2149F の接続は [ym2149_hw.c](ym2149_hw.c) のコメント
および以下の表を参照してください。

| YM2149F  | UIAPduino | CH32V003 | 備考          |
| -------- | --------- | -------- | ------------- |
| DA0      | 2         | PC0      | LED 接続      |
| DA1      | 3         | PC1      |               |
| DA2      | 4         | PC2      |               |
| DA3      | 5         | PC3      |               |
| DA4      | A2        | PC4      |               |
| DA5      | 7         | PC5      |               |
| DA6      | 8         | PC6      |               |
| DA7      | 9         | PC7      |               |
| BC1      | A1        | PA1      | 制御線        |
| BDIR     | A0        | PA2      | 制御線        |
| RESET    | 10        | PD0      | active-low (RPi版とは逆) |
| CLOCK    | A3        | PD2      | 2MHz TIM1 PWM |

BC2 は H 固定、 A8, A9 は未接続（デフォルトの内部プルアップ・プルダウン）です。

## ソース構成

ソースコード構成は以下の `rpi-psgplayer` の基本的な分離構造を継承しています。

- ハードウェア依存の PSG アクセス部
- 演奏ドライバコア
- 埋め込み演奏データ

PSG ドライバの演奏処理およびデータ処理は Raspberry Pi 版と同様の設計です。

一方、低レベルの PSG アクセス、2ms 周期処理のためのタイマ割り込み処理、
PSG 2MHz クロック出力については、UIAPduino / CH32V003 向けに作り直しています。

## リポジトリ構成

`ch32fun` をビルド環境として使用するため git submodule として使用します。

ファイル構成は以下のとおりです。

```text
UIAP-psgplayer/
  Makefile
  *.c
  *.h
  ch32fun/
```

ビルド用 `Makefile` で submodule 内の `ch32fun/ch32fun/ch32fun.mk` を
`include` することで `ch32fun` によるビルドができるようにしてあります。

## ビルド

submodule を含めて clone してください。

```sh
git clone --recursive https://github.com/tsutsui/UIAP-psgplayer.git
cd UIAP-psgplayer
```

`--recursive` なしで clone した場合は、submodule を手動で初期化してください。

```sh
git submodule update --init --recursive
```

その後、`make` でビルドします。`ch32fun` 側の仕様で GNU make が必要です。

```sh
make
```

または

```sh
gmake
```

`ch32fun` 側に [NetBSDサポート](https://github.com/cnlohr/ch32fun/pull/873)
を入れてもらったので NetBSD 上でも pkgsrc の `cross/riscv64-none-elf-gcc`
を使用してのビルドも可能です。

## 書き込み

UIAPduino への書き込みには、`ch32fun` 同梱の `minichlink` を使用します。

デフォルトの `ch32fun` の flash コマンドは、元の rv003usb bootloader の
VID:PID を前提としているため、そのままでは UIAPduino への書き込みに失敗します。

このため、`Makefile` 内で flash コマンドを上書きし、UIAPduino の
PID (`0xb803`) を明示的に指定することで、`make flash` で書き込めるようにしています。

前述の `ch32fun` へのプルリクエストのマージされたので NetBSD 上でも pkgsrc の
[libhidapi](https://cdn.netbsd.org/pub/pkgsrc/current/pkgsrc/comms/libhidapi/index.html) と
[libusb1](https://cdn.netbsd.org/pub/pkgsrc/current/pkgsrc/devel/libusb1/index.html)
をインストールすれば `minichlink` を使った書き込みは可能ですが、
`xhci(4)` の場合は
[`xhci(4)` ドライバのバグ](https://gnats.netbsd.org/60074)
あるため書き込みができない状況が発生しやすいです。

書き込みは以下で行います。

```sh
sudo make flash
```

## サンプル演奏データ

P6 PSG ドライバ演奏のサンプルデータとして、
[ニコニコ動画 おどる初音ミクGB feat. PC6001VX](https://www.nicovideo.jp/watch/sm45628241)
で使用している、茶葉のぎか さんによる ["meow bell"](https://youtu.be/XjxQew29tCc)
の PSG アレンジ版を `psg_data.c` に収録しています。

## 関連プロジェクト

* [`rpi-psgplayer`](https://github.com/tsutsui/rpi-psgplayer)  
  Raspberry Pi 版の YM2149F PSG プレーヤー

* [`p6psgmmlc`](https://github.com/tsutsui/p6psgmmlc)  
  PC-6001 PSG 音源ドライバ用 MML コンパイラ（C 言語コマンドライン版）  
  新たな PSG 曲を作成したい場合はこちらを使用してください。

* [`ch32fun`](https://github.com/cnlohr/ch32fun)  
  UIAPduino のビルド環境として使用している CH32V003 向け開発フレームワーク

## ライセンス

後述のものを除き 2条項 BSD ライセンスです。
詳細は添付の [`LICENSE`](LICENSE) ファイルを参照してください。

`psg_data.c` に含まれるサンプル演奏データのライセンスについては、
当該ファイル内のコメントを参照してください。

`Makefile` および `funconfig.h` については
`ch32fun` の `examples/template/` 以下のファイルをベースにしています。
元のテンプレートは MIT license で配布されています。

また、`ch32fun` の git submodule 自体についても、
当該プロジェクトのライセンス条件を参照してください

## 注意事項

現時点では、UIAPduino 上でも P6 PSG 楽曲データが演奏できることを実証するための
最速かつ最小限の実装になっています。

そのため、ディレクトリ階層やファイル構成については、今後リファクタリングを行う可能性があります。

自分用に作ったものなので、 issue や pull request には対応できない場合があります。
