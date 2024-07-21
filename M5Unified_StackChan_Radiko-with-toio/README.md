# M5Unified_StackChan_Radiko-with-toio

kenichi884追加箇所開始 

toio core cubeと組み合わせて使うことで、Avatarの動きに合わせてtoio core cubeが動きます。<br>
toio用のマットが必要です。起動時にマットの位置を読み取ってそこを中心に首振りや前後移動します。<br>
  https://protopedia.net/prototype/5496 <br>
オリジナルと違ってPSRAM搭載の機種でないと動きません。(Core2、CoreS3で動作確認しています)<br>
また、2024.07.20時点でM5Burner用イメージは公開していません。

 kenichi884追加箇所終わり 以下、オリジナルのREADME

---

WebRadio Radikoプレイヤー付きｽﾀｯｸﾁｬﾝです。


![画像1](images/image1.png)<br><br>


@mongonta555 さんの[ｽﾀｯｸﾁｬﾝ M5GoBottom版組み立てキット](https://raspberrypi.mongonta.com/about-products-stackchan-m5gobottom-version/ "Title")に対応しています。<br>

wakwak-kobaさんの M5Stack Radikoプレイヤーをアバターとレベルメーターを同時に表示できるように改造。<br>
さらにｽﾀｯｸﾁｬﾝM5GoBottom版組み立てキットで動くようにしました。<br>

Radikoプレイヤーは、wakwak-kobaさんのWebRadio_Japanをベースにさせていただきました。<br>
オリジナルはこちら。<br>
WebRadio_Japan <https://github.com/wakwak-koba/WebRadio_Japan><br>


Avatar表示は、meganetaaanさんのm5stack-avatarをベースにさせていただきました。<br>
オリジナルはこちら。<br>
An M5Stack library for rendering avatar faces <https://github.com/meganetaaan/m5stack-avator><br>

---
### このプログラムを動かすのに必要な物 ###
* [M5Stack Core2](http://www.m5stack.com/ "Title") (M5Stack Core2 for AWSで動作確認をしました。)<br>
* [ｽﾀｯｸﾁｬﾝ M5GoBottom版組み立てキット](https://raspberrypi.mongonta.com/about-products-stackchan-m5gobottom-version/ "Title")
* VSCode
* PlatformIO<br>

使用しているライブラリ等は"platformio.ini"を参照してください。<br>


---
### ｽﾀｯｸﾁｬﾝを組み立てる時の注意点 ###
ｽﾀｯｸﾁｬﾝ を組み立てる時は事前にサーボの位置を90度にしておいて、その時ｽﾀｯｸﾁｬﾝが正面を向くように取り付けてください。<br>
それを忘れるとバラしてもう一度組み立て直しになる可能性があります。<br>

---
### サーボモーターで使用するピンの設定 ###
* "M5Unified_StackChan_Radiko.ino"の10行目付近、SERVO_PIN_XとSERVO_PIN_Yを設定してください。

---
### WiFiの設定 ###
* "M5Unified_StackChan_Radiko.ino"の1行目付近、SSIDとPASSWORDを設定してください。
* SSIDとPASSWORDを設定せずにSmartConfigを使用することもできます。
その場合はiOSかAndroidの「Espressif Esptouch」アプリから設定します。

---
### 使い方 ###
* SSIDとPASSWORDを設定していない場合Wi-Fiの設定はSmartConfigを使用し、iOSかAndroidの「Espressif Esptouch」アプリから設定します。<br>
* ボタンA：選曲 ボタンB：音量ー ボタンC：音量＋<br>
* レベルメーター表示部にタッチすると、レベルメーター表示をON/OFFできます。<br>
* 画面中央にタッチすると首振りを止めます。<br>
* レベルメーター表示OFFの時、画面下部にタッチするとバルーンでラジオ局名を表示します。<br>
* 画面右端の中央付近にタッチするとFaceを切り替えられます。<br><br>

---
### 参考リンク ###
* [ｽﾀｯｸﾁｬﾝ M5GoBottom版 組み立て方法【その１ 部品キット編】](https://raspberrypi.mongonta.com/how-to-make-stackchan-m5gobottom/ "Title")　<br>
* [ｽﾀｯｸﾁｬﾝ M5GoBottom版 組み立て方法【その2 ケースセットの組み立てと完成まで】](https://raspberrypi.mongonta.com/how-to-make-stackchan-m5gobottom-2/ "Title")　<br>
* [分解作業なしでｽﾀｯｸﾁｬﾝを作る。【ｽﾀｯｸﾁｬﾝ M5GoBottom版 組み立てキット】](https://raspberrypi.mongonta.com/how-to-build-easy-stackchan-m5gobottom/ "Title")<br>

<br><br><br>

