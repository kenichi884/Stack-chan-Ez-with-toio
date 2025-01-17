# Stack-chan-Ez-with-toio

先人たちのｽﾀｯｸﾁｬﾝファームウェアを改造して、サーボではなくtoio core cubeで動くようにしたやつを置いてあります。

2024.07.20時点でM5Burner用イメージは公開していません。
PlatfomIOでビルドしてください。

ｽﾀｯｸﾁｬﾝお誕生会2024 LT資料 「[簡単ｽﾀｯｸﾁｬﾝ w/ toio](
https://docs.google.com/presentation/d/186M8a8ouff_oBr4j4CmEP-brTQKRSvgxdxwYsm4thco/edit?usp=sharing)」

## 動かすのに必要なもの
- M5Stack Core Basic/Core2/StickCplus2/AtomS3など
- StickCplus2のようなスピーカー非搭載機の場合はスピーカー([HAT SPK2](https://ssci.to/8864))
- AtomS3、Dialのようなマイク非搭載機の場合はマイク([MINI UNIT PDM](https://ssci.to/6620))
- [toio core cube](https://ssci.to/6300)
- [専用充電器](https://ssci.to/9578)
    - [toio本体セット](https://ssci.to/6302)を買った場合は充電機能があるので不要
- toio用プレイマット
    - 位置が取れるマットであればなんでもかまいません。
    - toio core cube単品版付属の簡易プレイマット
    - 開発用プレイマット([A3](https://ssci.to/6650)、[A4](https://ssci.to/8144))
    - [トイオコレクション](https://ssci.to/6305)付属のプレイマットなど。
- 接続用3Dプリントパーツ
    - Thingiverse
        - https://www.thingiverse.com/search?q=Stack-chan+Ez
    - MakerWorld
        - https://makerworld.com/en/search/models?keyword=Stack-chan+toio

## 動かし方
- ビルドしたファームウェアをM5Stack本体に書き込む
    - RadikoなどWiFi接続が必要なものはWiFi SSID、PASSWORD設定が必要です。
- toio core cubeとM5Stack本体を接続用プリントパーツで合体させる
- toio core cubeとM5Stack本体の電源を入れる
- すぐにプレイマットの上に置く。
- 起動してM5Stack本体とtoio core cubeとのBLE通信が確立した後、マットの座標を読み取りその位置を中心に動きます。マットに置けなかったり、異常な動作をしている場合は、M5Stack本体を再起動してやりなおしてください。
- ファームウェアの機能によりますが、マイクから拾った音、あるいはBluetooth A2DPで再生している音、Radikoの音声に合わせてAvatarとtoio core cubeが動き、Avatarの口パクに合わせてtoio core cubeのLEDも赤く明滅します。

## サーボの代わりにtoio core cubeで動くようにしたｽﾀｯｸﾁｬﾝファームウェアのリポジトリ

- m5stack-avatar-mic-with-toio
    - mongonta716さんの https://github.com/mongonta0716/m5stack-avatar-mic を改造
        - マイクから拾った音に反応して動きます
    - Core Basic(Metal)/Core2(AWS)/ATOMS3 + PDMUnit/M5Dial + PDMUnit(Port.A)で動作を確認
- stackchan-bluetooth-simple-with-toio
    - mongonta716さんの https://github.com/mongonta0716/stackchan-bluetooth-simple を改造
    - Bluetooth A2DP で鳴らした音に反応して動きます
    - (注)Bluetooth A2DPを使う場合はnimBLEでは使えないのでToioCore.hのUSE_NIMBLEをオフにしてビルドする必要があります。　//#define USE_NIMBLE 1
    - Bluetooth A2DPの機能がないESP32-S3系では動きません
    -  Core Basic(Metal)/Core2(AWS)で動作を確認
- M5StickC_WebRadio_Radiko_Avatar-with-toio
    - robo8080さんの https://github.com/robo8080/M5StickC_WebRadio_Radiko_Avatar を改造
    - Radikoを受信してその音に反応して動きます(M5StickCplus版)  
    - オリジナルはPlusでも動きますが本バージョンはPSRAMのあるPlus2でないと動きません
- M5Unified_StackChan_Radiko-with-toio
    - robo8080さんの https://github.com/robo8080/M5Unified_StackChan_Radiko を改造
    - Radikoを受信してその音その音に反応して動きます
    - Core2(AWS)、CoreS3 で動作を確認 要PSRAM搭載機
