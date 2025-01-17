//#define WIFI_SSID "SET YOUR WIFI SSID"
//#define WIFI_PASS "SET YOUR WIFI PASS"
//#define RADIKO_USER "SET YOUR MAIL-ADDRESS"
//#define RADIKO_PASS "SET YOUR PREMIUM PASS"

#include <math.h>
#include <WiFi.h>
#include <SD.h>
#include <M5UnitLCD.h>
#include <M5UnitOLED.h>
#include <M5Unified.h>
#include <nvs.h>

//#define SEPARATE_DOWNLOAD_TASK
#include <WebRadio_Radiko.h>
#include <AudioOutputM5Speaker.h>

#include <Toio.h>
// Toio オブジェクト生成
Toio toio;
ToioCore* toiocore = nullptr;
uint default_angle = 0;
uint default_posx = 0;
uint default_posy = 0;

#include "Avatar.h"
//#include "AtaruFace.h"
#include "RamFace.h"
#include "PaletteColor.h"

//for C Plus
#define USE_SPK_HAT2 

int BatteryLevel = -1;

static constexpr uint8_t select_pref = 0;

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;
static constexpr uint8_t m5spk_task_pinned_core = APP_CPU_NUM;

#define FFT_SIZE 256
class fft_t
{
  float _wr[FFT_SIZE + 1];
  float _wi[FFT_SIZE + 1];
  float _fr[FFT_SIZE + 1];
  float _fi[FFT_SIZE + 1];
  uint16_t _br[FFT_SIZE + 1];
  size_t _ie;

public:
  fft_t(void)
  {
#ifndef M_PI
#define M_PI 3.141592653
#endif
    _ie = logf( (float)FFT_SIZE ) / log(2.0) + 0.5;
    static constexpr float omega = 2.0f * M_PI / FFT_SIZE;
    static constexpr int s4 = FFT_SIZE / 4;
    static constexpr int s2 = FFT_SIZE / 2;
    for ( int i = 1 ; i < s4 ; ++i)
    {
    float f = cosf(omega * i);
      _wi[s4 + i] = f;
      _wi[s4 - i] = f;
      _wr[     i] = f;
      _wr[s2 - i] = -f;
    }
    _wi[s4] = _wr[0] = 1;

    size_t je = 1;
    _br[0] = 0;
    _br[1] = FFT_SIZE / 2;
    for ( size_t i = 0 ; i < _ie - 1 ; ++i )
    {
      _br[ je << 1 ] = _br[ je ] >> 1;
      je = je << 1;
      for ( size_t j = 1 ; j < je ; ++j )
      {
        _br[je + j] = _br[je] + _br[j];
      }
    }
  }

  void exec(const int16_t* in)
  {
    memset(_fi, 0, sizeof(_fi));
    for ( size_t j = 0 ; j < FFT_SIZE / 2 ; ++j )
    {
      float basej = 0.25 * (1.0-_wr[j]);
      size_t r = FFT_SIZE - j - 1;

      /// perform han window and stereo to mono convert.
      _fr[_br[j]] = basej * (in[j * 2] + in[j * 2 + 1]);
      _fr[_br[r]] = basej * (in[r * 2] + in[r * 2 + 1]);
    }

    size_t s = 1;
    size_t i = 0;
    do
    {
      size_t ke = s;
      s <<= 1;
      size_t je = FFT_SIZE / s;
      size_t j = 0;
      do
      {
        size_t k = 0;
        do
        {
          size_t l = s * j + k;
          size_t m = ke * (2 * j + 1) + k;
          size_t p = je * k;
          float Wxmr = _fr[m] * _wr[p] + _fi[m] * _wi[p];
          float Wxmi = _fi[m] * _wr[p] - _fr[m] * _wi[p];
          _fr[m] = _fr[l] - Wxmr;
          _fi[m] = _fi[l] - Wxmi;
          _fr[l] += Wxmr;
          _fi[l] += Wxmi;
        } while ( ++k < ke) ;
      } while ( ++j < je );
    } while ( ++i < _ie );
  }

  uint32_t get(size_t index)
  {
    return (index < FFT_SIZE / 2) ? (uint32_t)sqrtf(_fr[ index ] * _fr[ index ] + _fi[ index ] * _fi[ index ]) : 0u;
  }
};

static constexpr size_t WAVE_SIZE = 320;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
static Radiko radio(&out, 1-m5spk_task_pinned_core);

static fft_t fft;
static bool fft_enabled = false;
static bool wave_enabled = false;
static uint16_t prev_y[(FFT_SIZE / 2)+1];
static uint16_t peak_y[(FFT_SIZE / 2)+1];
static int16_t wave_y[WAVE_SIZE];
static int16_t wave_h[WAVE_SIZE];
static int16_t raw_data[WAVE_SIZE * 2];
static int header_height = 0;
static char stream_title[128] = { 0 };
static const char* meta_text[2] = { nullptr, stream_title };
static const size_t meta_text_num = sizeof(meta_text) / sizeof(meta_text[0]);
static uint8_t meta_mod_bits = 0;

static uint32_t bgcolor(LGFX_Device* gfx, int y)
{
  auto h = gfx->height()/4;
  auto dh = h - header_height;
  int v = ((h - y)<<5) / dh;
  if (dh > 44)
  {
    int v2 = ((h - y - 1)<<5) / dh;
    if ((v >> 2) != (v2 >> 2))
    {
      return 0x666666u;
    }
  }
  return gfx->color888(v + 2, v, v + 6);
}

static void gfxSetup(LGFX_Device* gfx)
{
  if (gfx == nullptr) { return; }
//  if (gfx->width() < gfx->height())
//  {
//    gfx->setRotation(gfx->getRotation()^1);
//  }
  gfx->setFont(&fonts::lgfxJapanGothic_12);
  gfx->setEpdMode(epd_mode_t::epd_fastest);
  gfx->setTextWrap(false);
  gfx->setCursor(0, 8);
  gfx->println("WebRadio player");
  gfx->fillRect(0, 6, gfx->width(), 2, TFT_BLACK);

  header_height = (gfx->height() > 80) ? 33 : 21;
  fft_enabled = !gfx->isEPD();
  if (fft_enabled)
  {
    wave_enabled = (gfx->getBoard() != m5gfx::board_M5UnitLCD);

    for (int y = header_height; y < gfx->height(); ++y)
    {
      gfx->drawFastHLine(0, y, gfx->width(), bgcolor(gfx, y));
    }
  }

  for (int x = 0; x < (FFT_SIZE/2)+1; ++x)
  {
//    prev_y[x] = INT16_MAX;
    prev_y[x] = gfx->height()/4;
    peak_y[x] = INT16_MAX;
  }
  for (int x = 0; x < WAVE_SIZE; ++x)
  {
    wave_y[x] = gfx->height()/4;
    wave_h[x] = 0;
  }
}

void gfxLoop(LGFX_Device* gfx)
{
  if (gfx == nullptr) { return; }
  if (header_height > 32)
  {
    if (meta_mod_bits)
    {
      gfx->startWrite();
      for (int id = 0; id < meta_text_num; ++id)
      {
        if (0 == (meta_mod_bits & (1<<id))) { continue; }
        meta_mod_bits &= ~(1<<id);
        size_t y = id * 12;
        if (y+12 >= header_height) { continue; }
        gfx->setCursor(4, 8 + y);
        gfx->fillRect(0, 8 + y, gfx->width(), 12, gfx->getBaseColor());
        gfx->print(meta_text[id]);
        gfx->print(" "); // Garbage data removal when UTF8 characters are broken in the middle.
      }
      gfx->display();
      gfx->endWrite();
    }
  }
  else
  {
    static int title_x;
    static int title_id;
    static int wait = INT16_MAX;

    if (meta_mod_bits)
    {
      if (meta_mod_bits & 1)
      {
        title_x = 4;
        title_id = 0;
        gfx->fillRect(0, 8, gfx->width(), 12, gfx->getBaseColor());
      }
      meta_mod_bits = 0;
      wait = 0;
    }

    if (--wait < 0)
    {
      int tx = title_x;
      int tid = title_id;
      wait = 3;
      gfx->startWrite();
      uint_fast8_t no_data_bits = 0;
      do
      {
        if (tx == 4) { wait = 255; }
        gfx->setCursor(tx, 8);
        const char* meta = meta_text[tid];
        if (meta[0] != 0)
        {
          gfx->print(meta);
          gfx->print("  /  ");
          tx = gfx->getCursorX();
          if (++tid == meta_text_num) { tid = 0; }
          if (tx <= 4)
          {
            title_x = tx;
            title_id = tid;
          }
        }
        else
        {
          if ((no_data_bits |= 1 << tid) == ((1 << meta_text_num) - 1))
          {
            break;
          }
          if (++tid == meta_text_num) { tid = 0; }
        }
      } while (tx < gfx->width());
      --title_x;
      gfx->display();
      gfx->endWrite();
    }
  }

  if (fft_enabled)
  {
    static int prev_x[2];
    static int peak_x[2];

    auto buf = out.getBuffer();
    if (buf)
    {
      memcpy(raw_data, buf, WAVE_SIZE * 2 * sizeof(int16_t)); // stereo data copy
      gfx->startWrite();

      // draw stereo level meter
      for (size_t i = 0; i < 2; ++i)
      {
        int32_t level = 0;
        for (size_t j = i; j < 640; j += 32)
        {
          uint32_t lv = abs(raw_data[j]);
          if (level < lv) { level = lv; }
        }

        int32_t x = (level * gfx->width()) / INT16_MAX;
        int32_t px = prev_x[i];
        if (px != x)
        {
          gfx->fillRect(x, i * 3, px - x, 2, px < x ? 0xFF9900u : 0x330000u);
          prev_x[i] = x;
        }
        px = peak_x[i];
        if (px > x)
        {
          gfx->writeFastVLine(px, i * 3, 2, TFT_BLACK);
          px--;
        }
        else
        {
          px = x;
        }
        if (peak_x[i] != px)
        {
          peak_x[i] = px;
          gfx->writeFastVLine(px, i * 3, 2, TFT_WHITE);
        }
      }
      gfx->display();

      // draw FFT level meter
      fft.exec(raw_data);
      size_t bw = gfx->width() / 60;
      if (bw < 3) { bw = 3; }
      int32_t dsp_height = gfx->height()/4;
      int32_t fft_height = dsp_height - header_height - 1;
      size_t xe = gfx->width() / bw;
      if (xe > (FFT_SIZE/2)) { xe = (FFT_SIZE/2); }
      int32_t wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[0] + raw_data[1])) * fft_height) >> 17);

      uint32_t bar_color[2] = { 0x000033u, 0x99AAFFu };

      for (size_t bx = 0; bx <= xe; ++bx)
      {
        size_t x = bx * bw;
        if ((x & 7) == 0) { gfx->display(); taskYIELD(); }
        int32_t f = fft.get(bx);
        int32_t y = (f * fft_height) >> 18;
        if (y > fft_height) { y = fft_height; }
        y = dsp_height - y;
        int32_t py = prev_y[bx];
        if (y != py)
        {
          gfx->fillRect(x, y, bw - 1, py - y, bar_color[(y < py)]);
          prev_y[bx] = y;
        }
        py = peak_y[bx] + 1;
        if (py < y)
        {
          gfx->writeFastHLine(x, py - 1, bw - 1, bgcolor(gfx, py - 1));
        }
        else
        {
          py = y - 1;
        }
        if (peak_y[bx] != py)
        {
          peak_y[bx] = py;
          gfx->writeFastHLine(x, py, bw - 1, TFT_WHITE);
        }


        if (wave_enabled)
        {
          for (size_t bi = 0; bi < bw; ++bi)
          {
            size_t i = x + bi;
            if (i >= gfx->width() || i >= WAVE_SIZE) { break; }
            y = wave_y[i];
            int32_t h = wave_h[i];
            bool use_bg = (bi+1 == bw);
            if (h>0)
            { /// erase previous wave.
              gfx->setAddrWindow(i, y, 1, h);
              h += y;
              do
              {
                uint32_t bg = (use_bg || y < peak_y[bx]) ? bgcolor(gfx, y)
                            : (y == peak_y[bx]) ? 0xFFFFFFu
                            : bar_color[(y >= prev_y[bx])];
                gfx->writeColor(bg, 1);
              } while (++y < h);
            }
            size_t i2 = i << 1;
            int32_t y1 = wave_next;
            wave_next = ((header_height + dsp_height) >> 1) + (((256 - (raw_data[i2] + raw_data[i2 + 1])) * fft_height) >> 17);
            int32_t y2 = wave_next;
            if (y1 > y2)
            {
              int32_t tmp = y1;
              y1 = y2;
              y2 = tmp;
            }
            y = y1;
            h = y2 + 1 - y;
            wave_y[i] = y;
            wave_h[i] = h;
            if (h>0)
            { /// draw new wave.
              gfx->setAddrWindow(i, y, 1, h);
              h += y;
              do
              {
                uint32_t bg = (y < prev_y[bx]) ? 0xFFCC33u : 0xFFFFFFu;
                gfx->writeColor(bg, 1);
              } while (++y < h);
            }
          }
        }
      }
      gfx->display();
      gfx->endWrite();
    }
  }

  if (!gfx->displayBusy())
  { // draw volume bar
    static int px;
    uint8_t v = M5.Speaker.getChannelVolume(m5spk_virtual_channel);
    int x = v * (gfx->width()) >> 8;
    if (px != x)
    {
      gfx->fillRect(x, 6, px - x, 2, px < x ? 0xAAFFAAu : 0u);
      gfx->display();
      px = x;
    }
  }
}

using namespace m5avatar;
Avatar* avatar;

void lipSync(void *args)
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
   for (;;)
  {
    level = abs(*out.getBuffer());
    if(level<100) level = 0;
    if(level > 15000)
    {
      level = 15000;
    }
    float open = (float)level/15000.0;
    toiocore->turnOnLed(int(0xff * open), 0x00, 0x00);
    avatar->setMouthOpenRatio(open);
    avatar->getGaze(&gazeY, &gazeX);
    avatar->setRotation(gazeX * 5);
    
    int degx = default_angle + (int) 20.0 * gazeX;
    if (degx > 360) 
      degx = degx - 360;
    else if(degx < 0 )
      degx = degx + 360;
    int tmp = 0;
    if(gazeY < 0) {
      tmp = (int)(15.0 * gazeY + open * 15.0);
      if(tmp > 15) tmp = 15;
    } else {
      tmp = (int)(10.0 * gazeY - open * 15.0);
    }    
    toiocore->controlMotorWithTarget(0, 5, 0, 30, 0x03, default_posx, default_posy + tmp, degx);
     delay(50);
  }
}

void Wifi_setup() {
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  M5.Display.println("WiFi begin");
  Serial.println("WiFi begin");
#if defined ( WIFI_SSID ) && defined ( WIFI_PASS )
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#else
  // 前回接続時情報で接続する
  WiFi.begin();
#endif
  // 前回接続時情報で接続する
  //M5.Display.println("WiFi begin");
  //Serial.println("WiFi begin");
  //WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    Serial.print(".");
    delay(500);
    // 10秒以上接続できなかったら抜ける
    if ( 10000 < millis() ) {
      break;
    }
  }
  M5.Display.println("");
  Serial.println("");
  // 未接続の場合にはSmartConfig待受
  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
    M5.Display.println("Waiting for SmartConfig");
    Serial.println("Waiting for SmartConfig");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      M5.Display.print("#");
      Serial.print("#");
      // 30秒以上接続できなかったら抜ける
      if ( 30000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    // Wi-fi接続
    M5.Display.println("");
    Serial.println("");
    M5.Display.println("Waiting for WiFi");
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      M5.Display.print(".");
      Serial.print(".");
      // 60秒以上接続できなかったら抜ける
      if ( 60000 < millis() ) {
        Serial.println("");
        Serial.println("Reset");
        ESP.restart();
      }
    }
    M5.Display.println("");
    Serial.println("");
    M5.Display.println("WiFi Connected.");
    Serial.println("WiFi Connected.");
  }
  M5.Display.print("IP Address: ");
  Serial.print("IP Address: ");
  M5.Display.println(WiFi.localIP());
  Serial.println(WiFi.localIP());
}

void Avatar_setup() {
//    avatar = new Avatar(new AtaruFace());
/*
  avatar = new Avatar(new RamFace());
  ColorPalette cp;
  cp.set(COLOR_PRIMARY, PC_BLACK);  //
  cp.set(COLOR_BACKGROUND, PC_WHITE);
  cp.set(COLOR_SECONDARY, PC_WHITE);
  */
 avatar = new Avatar();
  ColorPalette cp;
  cp.set(COLOR_PRIMARY, PC_WHITE);  //
  cp.set(COLOR_BACKGROUND, PC_BLACK);
  cp.set(COLOR_SECONDARY, PC_BLACK);
   avatar->setColorPalette(cp);

  
  
  switch (M5.getBoard())
  {
    case m5::board_t::board_M5StickCPlus:
      avatar->setScale(0.45);
      avatar->setOffset(-90, 30);
      break;
    case m5::board_t::board_M5StickC:
      avatar->setScale(0.25);
      avatar->setOffset(-120, 0);
      break;
    case m5::board_t::board_M5Stack:
    case m5::board_t::board_M5StackCore2:
    case m5::board_t::board_M5Tough:
      avatar->setScale(0.80);
      avatar->setOffset(0, 52);
      break;
    default:
      avatar->setScale(0.45);
      avatar->setOffset(-90, 30);
      break;
  }
  avatar->init(); // start drawing
  avatar->addTask(lipSync, "lipSync");
}

void setup(void)
{
  audioLogger = &Serial;
  
  auto cfg = M5.config();

  cfg.external_spk = true;    /// use external speaker (SPK HAT / ATOMIC SPK)
  // If you want to play sound from HAT Speaker, write this
  //cfg.external_speaker.hat_spk        = true;
  // If you want to play sound from HAT Speaker2, write this
#ifdef USE_SPK_HAT2
  cfg.external_speaker.hat_spk2 = true;
#endif
//cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
//cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT

  M5.begin(cfg);
  M5.update();
  if(M5.BtnA.isPressed() && M5.BtnB.isPressed() && M5.BtnC.isPressed()) {
    uint32_t nvs_handle;
    if(ESP_OK == nvs_open("WebRadio", NVS_READWRITE, &nvs_handle)) {
      M5.Display.println("nvs_flash_ersce");
      nvs_erase_all(nvs_handle);
      delay(3000);
    }
  }

  { /// custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    //spk_cfg.sample_rate = 144000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.sample_rate =  32000;// 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = m5spk_task_pinned_core;
    M5.Speaker.config(spk_cfg);
  }

  M5.Speaker.begin();
/*
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

#if defined ( WIFI_SSID ) && defined ( WIFI_PASS )
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#endif
*/
#if defined( RADIKO_USER ) && defined( RADIKO_PASS )
  radio.setAuthorization(RADIKO_USER, RADIKO_PASS);
#endif

  Wifi_setup();
/*
  /// settings
  if (SD.begin(GPIO_NUM_4, SPI, 25000000)) {
    /// wifi
    auto fs = SD.open("/wifi.txt", FILE_READ);
    if(fs) {
      size_t sz = fs.size();
      char buf[sz + 1];
      fs.read((uint8_t*)buf, sz);
      buf[sz] = 0;
      fs.close();

      int y = 0;
      for(int x = 0; x < sz; x++) {
        if(buf[x] == 0x0a || buf[x] == 0x0d)
          buf[x] = 0;
        else if (!y && x > 0 && !buf[x - 1] && buf[x])
          y = x;
      }
      WiFi.begin(buf, &buf[y]);
    }

    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("WebRadio", NVS_READWRITE, &nvs_handle)) {
      /// radiko-premium
      fs = SD.open("/radiko.txt", FILE_READ);
      if(fs) {
        size_t sz = fs.size();
        char buf[sz + 1];
        fs.read((uint8_t*)buf, sz);
        buf[sz] = 0;
        fs.close();
  
        int y = 0;
        for(int x = 0; x < sz; x++) {
          if(buf[x] == 0x0a || buf[x] == 0x0d)
            buf[x] = 0;
          else if (!y && x > 0 && !buf[x - 1] && buf[x])
            y = x;
        }

        nvs_set_str(nvs_handle, "radiko_user", buf);
        nvs_set_str(nvs_handle, "radiko_pass", &buf[y]);
      }
      
      nvs_close(nvs_handle);
    }
    SD.end();
  }
*/
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("WebRadio", NVS_READONLY, &nvs_handle)) {
      size_t volume;
      nvs_get_u32(nvs_handle, "volume", &volume);
      if(volume>255) volume = 255;
      M5.Speaker.setVolume(volume);
      M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);

      size_t length1;
      size_t length2;
      if(ESP_OK == nvs_get_str(nvs_handle, "radiko_user", nullptr, &length1) && ESP_OK == nvs_get_str(nvs_handle, "radiko_pass", nullptr, &length2) && length1 && length2) {
        char user[length1 + 1];
        char pass[length2 + 1];
        if(ESP_OK == nvs_get_str(nvs_handle, "radiko_user", user, &length1) && ESP_OK == nvs_get_str(nvs_handle, "radiko_pass", pass, &length2)) {
          M5.Display.print("premium member: ");
          M5.Display.println(user);
          radio.setAuthorization(user, pass);
        }
      }
      nvs_close(nvs_handle);
    }
  }
/*
  // Try forever
  M5.Display.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    M5.Display.print(".");
    delay(100);
  }
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());  
*/

  M5.Display.setTextSize(2); 
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
  Serial.printf("default pos (%u, %u) angle %u\n", default_posx, default_posy, default_angle);

  M5.Display.clear();

  gfxSetup(&M5.Display);
//  M5.Display.fillRect(0, 61, M5.Display.width(), M5.Display.height(), TFT_WHITE);
  //M5.Display.fillRect(0, M5.Display.height()/4+1, M5.Display.width(), M5.Display.height(), TFT_WHITE);
  M5.Display.fillRect(0, M5.Display.height()/4+1, M5.Display.width(), M5.Display.height(), TFT_BLACK);

// radiko
  radio.onPlay = [](const char * station_name, const size_t station_idx) {
    Serial.printf("onPlay:%d %s", station_idx, station_name);
    Serial.println();
    meta_text[0] = station_name;
    stream_title[0] = 0;
    meta_mod_bits = 3;
  };
  radio.onInfo = [](const char *text) {
    Serial.println(text);
  };
  radio.onError = [](const char * text) {
    Serial.println(text);
  };
  radio.onProgram = [](const char * program_title) {
    strcpy(stream_title, program_title);
    meta_mod_bits |= 2;
  };
#ifdef SYSLOG_TO
  radio.setSyslog(SYSLOG_TO);
#endif
  if(!radio.begin()) {
    Serial.println("failed: radio.begin()");
    for(;;);
  } 
  radio.play();

  Avatar_setup();

}

void loop(void)
{
  static unsigned long long saveSettings = 0;
  radio.handle();
  gfxLoop(&M5.Display);
  avatar->draw();

  {
    static int prev_frame;
    int frame;
    do
    {
      delay(1);
    } while (prev_frame == (frame = millis() >> 3)); /// 8 msec cycle wait
    prev_frame = frame;
  }
 
  static int lastms = 0;
  if (millis()-lastms > 1000) {
    lastms = millis();
    BatteryLevel = M5.Power.getBatteryLevel();
//    printf("%d\n\r",BatVoltage);
   }

  M5.update();
  if (M5.BtnA.wasPressed())
  {
    M5.Speaker.tone(440, 50);
  }
  if (M5.BtnA.wasDeciedClickCount())
  {
    switch (M5.BtnA.getClickCount())
    {
    case 1:
      M5.Speaker.tone(1000, 100);
      radio.play(true);
      break;

    case 2:
      M5.Speaker.tone(800, 100);
      radio.play(false);
      break;
    }
  }
//  if (M5.BtnA.isHolding() || M5.BtnB.isPressed() || M5.BtnC.isPressed())
  if (M5.BtnA.isHolding() || M5.BtnB.isPressed())
  {
    size_t v = M5.Speaker.getChannelVolume(m5spk_virtual_channel);
    int add = (M5.BtnB.isPressed()) ? -1 : 1;
    if (M5.BtnA.isHolding())
    {
      add = M5.BtnA.getClickCount() ? -1 : 1;
    }
    v += add;
    if (v <= 255)
    {
      M5.Speaker.setVolume(v);
      M5.Speaker.setChannelVolume(m5spk_virtual_channel, v);
      saveSettings = millis() + 5000;
    }
  }

  if (saveSettings > 0 && millis() > saveSettings)
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("WebRadio", NVS_READWRITE, &nvs_handle)) {
      size_t volume = M5.Speaker.getChannelVolume(m5spk_virtual_channel);
      nvs_set_u32(nvs_handle, "volume", volume);
      nvs_close(nvs_handle);
    }
    saveSettings = 0;
  }
}