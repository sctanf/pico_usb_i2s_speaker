# これなに
Raspberry Pi Picoとtinyusbを使ったマスタークロック付きのi2sを出力するusbスピーカーです。

# build
vscodeの拡張機能(Raspberry Pi Pico)でインポートし、ビルドしてください。
pico-sdkは2.1.0を使っています。

# i2s
https://github.com/BambooMaster/pico-i2s-pio.git を使っています。

|name|pin|
|----|---|
|DATA|GPIO2|
|LRCLK|GPIO3|
|BCLK|GPIO4|
|MCLK|GPIO21|

# 対応機種
Windows11で動作確認をしています。Android (Pixel6a Android14)ではフィードバックが動作しませんでした。