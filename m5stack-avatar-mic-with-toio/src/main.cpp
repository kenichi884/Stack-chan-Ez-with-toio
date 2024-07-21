// Copyright(c) 2023 Takao Akaki


#include <M5Unified.h>
#include <Avatar.h>
#include "fft.hpp"
#include <cinttypes>

#include <Toio.h>
// Toio オブジェクト生成
Toio toio;
ToioCore* toiocore = nullptr;
uint default_angle = 0;
uint default_posx = 0;
uint default_posy = 0;

#if defined(ARDUINO_M5STACK_CORES3)
  #include <gob_unifiedButton.hpp>
  goblib::UnifiedButton unifiedButton;
#endif
#define USE_MIC

#ifdef USE_MIC
  // ---------- Mic sampling ----------

  #define READ_LEN    (2 * 256)
  #define LIPSYNC_LEVEL_MAX 10.0f

  int16_t *adcBuffer = NULL;
  static fft_t fft;
  static constexpr size_t WAVE_SIZE = 256 * 2;

  static constexpr const size_t record_samplerate = 16000; // M5StickCPlus2だと48KHzじゃないと動かなかったが、M5Unified0.1.12で修正されたのとM5AtomS2+PDFUnitで不具合が出たので戻した。。
  static int16_t *rec_data;
  
  // setupの最初の方の機種判別で書き換えている場合があります。そちらもチェックしてください。（マイクの性能が異なるため）
  uint8_t lipsync_shift_level = 11; // リップシンクのデータをどのくらい小さくするか設定。口の開き方が変わります。
  float lipsync_max =LIPSYNC_LEVEL_MAX;  // リップシンクの単位ここを増減すると口の開き方が変わります。

#endif




using namespace m5avatar;

Avatar avatar;
ColorPalette *cps[6];
uint8_t palette_index = 0;

uint32_t last_rotation_msec = 0;
uint32_t last_lipsync_max_msec = 0;

void lipsync() {
  
  size_t bytesread;
  uint64_t level = 0;
  float gazeX, gazeY;
#ifndef SDL_h_
  if ( M5.Mic.record(rec_data, WAVE_SIZE, record_samplerate)) {
    fft.exec(rec_data);
    for (size_t bx=5;bx<=60;++bx) {
      int32_t f = fft.get(bx);
      level += abs(f);
    }
  }
  uint32_t temp_level = level >> lipsync_shift_level;
  //M5_LOGI("level:%" PRId64 "\n", level) ;         // lipsync_maxを調整するときはこの行をコメントアウトしてください。
  //M5_LOGI("temp_level:%d\n", temp_level) ;         // lipsync_maxを調整するときはこの行をコメントアウトしてください。
  float ratio = (float)(temp_level / lipsync_max);
  //M5_LOGI("ratio:%f\n", ratio);
  if (ratio <= 0.01f) {
    ratio = 0.0f;
    if ((lgfx::v1::millis() - last_lipsync_max_msec) > 500) {
      // 0.5秒以上無音の場合リップシンク上限をリセット
      last_lipsync_max_msec = lgfx::v1::millis();
      lipsync_max = LIPSYNC_LEVEL_MAX;
    }
  } else {
    if (ratio > 1.3f) {
      if (ratio > 1.5f) {
        // リップシンク上限を大幅に超えた場合、上限を上げていく。
        lipsync_max += 10.0f;
      }
      ratio = 1.3f;
    }
    last_lipsync_max_msec = lgfx::v1::millis(); // 無音でない場合は更新
  }

  if ((lgfx::v1::millis() - last_rotation_msec) > 350) {
    int direction = random(-2, 2);
    avatar.setRotation(direction * 10 * ratio);
    last_rotation_msec = lgfx::v1::millis();
  }
#else
  float ratio = 0.0f;
#endif
  avatar.setMouthOpenRatio(ratio);
  toiocore->turnOnLed(int(0xff * ratio), 0x00, 0x00);
  avatar.getGaze(&gazeY, &gazeX);
  int degx = default_angle + (int) 20.0 * gazeX;
  if (degx > 360) 
    degx = degx - 360;
  else if(degx < 0 )
    degx = degx + 360;
  int tmp = 0;
  if(gazeY < 0) {
    tmp = (int)(15.0 * gazeY + ratio * 15.0);
    if(tmp > 15) tmp = 15;
  } else {
    tmp = (int)(10.0 * gazeY - ratio * 15.0);
  }    
  toiocore->controlMotorWithTarget(0, 5, 0, 30, 0x03, default_posx, default_posy + tmp, degx);
  
}


void setup()
{
  auto cfg = M5.config();
  cfg.internal_mic = true;
  M5.begin(cfg);
#if defined( ARDUINO_M5STACK_CORES3 )
  unifiedButton.begin(&M5.Display, goblib::UnifiedButton::appearance_t::transparent_all);
#endif
  M5.Log.setLogLevel(m5::log_target_display, ESP_LOG_NONE);
  M5.Log.setLogLevel(m5::log_target_serial, ESP_LOG_INFO);
  M5.Log.setEnableColor(m5::log_target_serial, false);
  M5_LOGI("Avatar Start");
  M5.Log.printf("M5.Log avatar Start\n");
  float scale = 0.0f;
  int8_t position_top = 0;
  int8_t position_left = 0;
  uint8_t display_rotation = 1; // ディスプレイの向き(0〜3)
  uint8_t first_cps = 0;
  auto mic_cfg = M5.Mic.config();
  switch (M5.getBoard()) {
    case m5::board_t::board_M5AtomS3:
      first_cps = 4;
      scale = 0.55f;
      position_top =  -60;
      position_left = -95;
      display_rotation = 2;
      // M5AtomS3は外部マイク(PDMUnit)なので設定を行う。
      mic_cfg.sample_rate = 16000;
      //mic_cfg.dma_buf_len = 256;
      //mic_cfg.dma_buf_count = 3;
      mic_cfg.pin_ws = 1;
      mic_cfg.pin_data_in = 2;
      M5.Mic.config(mic_cfg);
      break;

    case m5::board_t::board_M5StickC:
      first_cps = 1;
      scale = 0.6f;
      position_top = -80;
      position_left = -80;
      display_rotation = 3;
      break;

    case m5::board_t::board_M5StickCPlus:
      first_cps = 1;
      scale = 0.85f;
      position_top = -55;
      position_left = -35;
      display_rotation = 3;
      break;

    case m5::board_t::board_M5StickCPlus2:
      first_cps = 2;
      scale = 0.85f;
      position_top = -55;
      position_left = -35;
      display_rotation = 3;
      break;
   
     case m5::board_t::board_M5StackCore2:
      scale = 1.0f;
      position_top = 0;
      position_left = 0;
      display_rotation = 1;
      break;

    case m5::board_t::board_M5StackCoreS3:
    case m5::board_t::board_M5StackCoreS3SE:
      scale = 1.0f;
      position_top = 0;
      position_left = 0;
      display_rotation = 1;
      break;

    case m5::board_t::board_M5Stack:
      scale = 1.0f;
      position_top = 0;
      position_left = 0;
      display_rotation = 1;
      break;

    case m5::board_t::board_M5Dial:
      first_cps = 1;
      scale = 0.8f;
      position_top =  0;
      position_left = -40;
      display_rotation = 2;
      // M5ADial(StampS3)は外部マイク(PDMUnit)なので設定を行う。(Port.A)
      mic_cfg.pin_ws = 15;
      mic_cfg.pin_data_in = 13;
      M5.Mic.config(mic_cfg);
      break;

      
    defalut:
      M5.Log.println("Invalid board.");
      break;
  }
#ifndef SDL_h_
  rec_data = (typeof(rec_data))heap_caps_malloc(WAVE_SIZE * sizeof(int16_t), MALLOC_CAP_8BIT);
  memset(rec_data, 0 , WAVE_SIZE * sizeof(int16_t));
  M5.Mic.begin();
#endif
  M5.Speaker.end();
  // 3 秒間 Toio Core Cube をスキャン
  M5_LOGI("Scanning your toio core...");
  M5.Display.setCursor(0, 0);
  M5.Display.println("Scanning your toio core...");
  std::vector<ToioCore*> toiocore_list = toio.scan(3);
  size_t n = toiocore_list.size();
  if (n == 0) {
    M5_LOGI("No toio Core Cube was found. Turn on your Toio Core Cube, then press the reset button of your Toio Core Cube.");
    M5.Display.println("No toio Core Cube was found.");
    return;
  }

  // 最初に見つかった Toio Core Cube の ToioCore オブジェクト
  toiocore = toiocore_list.at(0);
  M5_LOGI("Your toio core was found:      ");
  M5.Display.println("No toio Core Cube was found.");

  // Toio Core のデバイス名と MAC アドレスを表示
  M5_LOGI("%s %s", toiocore->getName(), toiocore->getAddress());
  M5.Display.printf("%s %s\n", toiocore->getName().c_str(), toiocore->getAddress().c_str());

  // BLE 接続開始
  M5_LOGI("Connecting...");
  M5.Display.println("Connecting...");

  if (!toiocore->connect()) {
    M5_LOGI("Failed to establish a BLE connection.");
    M5.Display.println("Connection failed");
    return;
  }
  M5_LOGI("toio Connected.");
  M5.Display.println("toio Connected.");
  ToioCoreIDData pos = toiocore->getIDReaderData();
  default_angle = pos.position.cubeAngleDegree;
  default_posx = pos.position.cubePosX;
  default_posy = pos.position.cubePosY;
  M5_LOGI("default pos (%u, %u) angle %u\n", default_posx, default_posy, default_angle);

  M5.Display.setRotation(display_rotation);
  avatar.setScale(scale);
  avatar.setPosition(position_top, position_left);
  avatar.init(1); // start drawing
  cps[0] = new ColorPalette();
  cps[0]->set(COLOR_PRIMARY, TFT_BLACK);
  cps[0]->set(COLOR_BACKGROUND, TFT_YELLOW);
  cps[1] = new ColorPalette();
  cps[1]->set(COLOR_PRIMARY, TFT_BLACK);
  cps[1]->set(COLOR_BACKGROUND, TFT_ORANGE);
  cps[2] = new ColorPalette();
  cps[2]->set(COLOR_PRIMARY, (uint16_t)0x00ff00);
  cps[2]->set(COLOR_BACKGROUND, (uint16_t)0x303303);
  cps[3] = new ColorPalette();
  cps[3]->set(COLOR_PRIMARY, TFT_WHITE);
  cps[3]->set(COLOR_BACKGROUND, TFT_BLACK);
  cps[4] = new ColorPalette();
  cps[4]->set(COLOR_PRIMARY, TFT_BLACK);
  cps[4]->set(COLOR_BACKGROUND, TFT_WHITE);
  cps[5] = new ColorPalette();
  cps[5]->set(COLOR_PRIMARY, (uint16_t)0x303303);
  cps[5]->set(COLOR_BACKGROUND, (uint16_t)0x00ff00);
  avatar.setColorPalette(*cps[first_cps]);
  //avatar.addTask(lipsync, "lipsync");
  last_rotation_msec = lgfx::v1::millis();
  M5_LOGI("setup end");
}

uint32_t count = 0;

void loop()
{
  M5.update();

#if defined( ARDUINO_M5STACK_CORES3 )
  unifiedButton.update();
#endif
  if (M5.BtnA.wasPressed()) {
    M5_LOGI("Push BtnA");
    palette_index++;
    if (palette_index > 5) {
      palette_index = 0;
    }
    avatar.setColorPalette(*cps[palette_index]);
  }
  if (M5.BtnA.wasDoubleClicked()) {
    M5.Display.setRotation(3);
  }
  if (M5.BtnPWR.wasClicked()) {
#ifdef ARDUINO
    esp_restart();
#endif
  } 
//  if ((millis() - last_rotation_msec) > 100) {
    //float angle = 10 * sin(count);
    //avatar.setRotation(angle);
    //last_rotation_msec = millis();
    //count++;
  //}

  // avatar's face updates in another thread
  // so no need to loop-by-loop rendering
  lipsync();
  lgfx::v1::delay(1);
}
