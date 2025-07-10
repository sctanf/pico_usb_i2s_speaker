# pico_usb_i2s_speaker
Raspberry Pi Picoとtinyusbを使ったマスタークロック付きのi2sを出力するusbスピーカーです。

## build
### vscodeの拡張機能を使う場合
```
git clone https://github.com/BambooMaster/pico_usb_i2s_speaker.git
cd pico_usb_i2s_speaker
git submodule update --init
```
を実行した後、vscodeの拡張機能(Raspberry Pi Pico)でインポートし、ビルドしてください。

### vscodeの拡張機能を使わない場合
```
git clone https://github.com/BambooMaster/pico_usb_i2s_speaker.git
cd pico_usb_i2s_speaker
git submodule update --init
mkdir build && cd build
cmke .. && make -j4
```

## i2s
[pico-i2s-pio](https://github.com/BambooMaster/pico-i2s-pio.git)を使っています。RP2040/RP2350のシステムクロックをMCLKの整数倍に設定し、pioのフラクショナル分周を使わないlowジッタモードを搭載しています。

### デフォルト
|name|pin|
|----|---|
|DATA|GPIO18|
|LRCLK|GPIO20|
|BCLK|GPIO21|
|MCLK|GPIO22|

## 対応機種
Windows11で動作確認をしています。Android (Pixel6a Android14)ではフィードバックが動作しませんでした。