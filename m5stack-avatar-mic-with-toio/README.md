# m5stack-avatar-mic

kenichi884追加箇所開始
toio core cubeと組み合わせて使うことで、Avatarの動きに合わせてtoio core cubeが動きます。
toio用のマットが必要です。起動時にマットの位置を読み取ってそこを中心に首振りや前後移動します。
https://protopedia.net/prototype/5496
kenichi884追加箇所終わり

マイクを使ったM5Stack Avatarの例です。
音に合わせてAvatarが口パクしたり、傾いたりします。

# 対応デバイス

- M5Stack Core2/Core2 V1.1/AWS
- M5StickC
- M5StickCPlus
- ATOMS3 + PDMUnit
- M5Stack CoreS3
- M5Stack CoreS3SE
- M5Stack Fire
- M5Dial + PDMUnit(Port.A)

# 環境

・VSCode + PlatformIO

ArduinoIDEの場合は下記のように名前を変更してください。
- srcフォルダ -> m5stack-avatar-mic
- main.cpp -> m5stack-avatar-mic.ino

# 動作確認済みボード・ライブラリバーョン

詳細はplatformio.iniを見てください。

## ボード

- espressif 6.5.0

## ライブラリ
- M5Stack-Avatar v0.9.2
- M5Unified v0.1.16

# LICENSE
[MIT](https://github.com/mongonta0716/m5stack-avatar-mic/blob/main/LICENSE)

# Author

[Takao Akaki](https://github.com/mongonta0716)