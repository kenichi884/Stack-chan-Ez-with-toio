# M5StickC_WebRadio_Radiko_Avatar

kenichi884追加箇所開始 

toio core cubeと組み合わせて使うことで、Avatarの動きに合わせてtoio core cubeが動きます。<br>
toio用のマットが必要です。起動時にマットの位置を読み取ってそこを中心に首振りや前後移動します。<br>
  https://protopedia.net/prototype/5496 <br>
オリジナルと違ってPSRAM搭載のM5StickC Plus2でないと動きません。<br>
また、2024.07.20時点でM5Burner用イメージは公開していません。

kenichi884追加箇所終わり 以下、オリジナルのREADME

---

WebRadio Radikoプレイヤー付きｽﾀｯｸﾁｬﾝです。


M5StickC Plus用アバター表示、レベルメーター付きRadikoプレイヤー

![画像1](images/image1.png)<br><br>


wakwak-kobaさんの M5Stack Radikoプレイヤーをアバターとレベルメーターを同時に表示できるように改造。<br>
さらにM5StickC Plusで動くようにしました。<br>

Radikoプレイヤーは、wakwak-kobaさんのWebRadio_Japanをベースにさせていただきました。<br>
オリジナルはこちら。<br>
WebRadio_Japan <https://github.com/wakwak-koba/WebRadio_Japan><br>


Avatar表示は、meganetaaanさんのm5stack-avatarをベースにさせていただきました。<br>
オリジナルはこちら。<br>
An M5Stack library for rendering avatar faces <https://github.com/meganetaaan/m5stack-avator><br>

---
### このプログラムを動かすのに必要な物 ###
* M5StickC Plus2
* [スピーカーHat](https://www.switch-science.com/catalog/5754/ "Title")
* VSCode
* PlatformIO<br>

使用しているライブラリ等は"platformio.ini"を参照してください。<br>

---
### WiFiの設定 ###
* "M5StickC_WebRadio_Radiko_Avatar.ino"の1行目付近、SSIDとPASSWORDを設定してください。
* SSIDとPASSWORDを設定せずにSmartConfigを使用することもできます。
その場合はiOSかAndroidの「Espressif Esptouch」アプリから設定します。

---
ビルド済みファームウェアをM5Burner v3に公開しました。すぐに使ってみたい方はこちらをどうぞ。<br>

![画像2](images/image2.png)<br><br>

### 使い方 ###
* M5StickC PlusとスピーカーHatが必要です。<br>
* Wi-Fiの設定はSmartConfigを使用、iOSかAndroidの「Espressif Esptouch」アプリから設定します。<br>
* ボタンA：選曲 ボタンA長押し：音量＋ ボタンB：音量<br><br>


