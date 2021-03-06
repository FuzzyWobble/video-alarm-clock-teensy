
//stuff from frank below
//Demovideo see :
//https://drive.google.com/drive/folders/0Bx2Jw84lqebkelF3MHg2eWVzb0U
// This sketch works with std 180MHz and less, but better with 240MHZ, or 144MHz and overclocked F_BUS to the max.
//For Audio, edit the Audio-library and remove all files which #include "SD.h" - sorry, no other way.

#define USE_AUDIO 1
#include <SPI.h>
#include <ILI9341_t3DMA.h>
#include <SdFat.h>

//for C64 v0.3 Board:
#define SCK       14
#define MISO      39
#define MOSI      28
#define TFT_TOUCH_CS    38
#define TFT_TOUCH_INT   37
#define TFT_DC          20
#define TFT_CS          21
#define TFT_RST         255
#define TFT_SCLK        SCK
#define TFT_MOSI        MOSI
#define TFT_MISO        MISO


ILI9341_t3DMA tft = ILI9341_t3DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

SdFatSdioEX SD;
File file;

#if USE_AUDIO
const char filename[] = "wake_up_wake_up.bin";
#else
const char filename[] = "wake_up_wake_up.bin";
#endif

#if USE_AUDIO
#include <Audio.h>
AudioPlayQueue           queue1;         //xy=183,163
AudioOutputAnalog        dac1;           //xy=376,154
AudioConnection          patchCord1(queue1, dac1);
#endif

const float fps = 23.976;
const int rate_us = 1e6 / fps;
const uint32_t framesize = 153600;

#if USE_AUDIO
uint8_t header[512];
uint16_t audiobuf[16 * 320];
uint32_t audioPos = 0;
int16_t *lb = NULL;
//int16_t *rb = NULL;
#endif


void playVideo(const char * filename) {
  int bytesread;
  
  Serial.print("Begin playing ");
  Serial.println(filename);

  if (!file.open(filename, O_READ)) {
    SD.errorHalt("open failed");
    return;
  }

#if USE_AUDIO
  file.read(header, 512);  //read header - not used yet
#endif

  while (1) {
    uint32_t m;
    m = micros();
    uint32_t * p = screen32;
    int rd = 0;

    rd = framesize;
    do {

      bytesread = file.read(p, rd);
      if (bytesread <= 0) {
        break;
      }
      p += bytesread / sizeof(uint32_t);
      rd -= bytesread;
    } while (bytesread > 0 && rd > 0);

    if (bytesread <= 0) {
      break;
    }

#if USE_AUDIO
    bytesread = file.read(audiobuf, sizeof(audiobuf));
    if (!bytesread) {
      break;
    }

    //Audio is 1839 samples*2 channels
    int16_t l = 0;
    for (int i = 0; i < 1839; i++) {
      if (audioPos == 0) {
        lb = queue1.getBuffer();
      }
      if (lb) {
        *lb++ = audiobuf[i * 2];
        audioPos += 1;
        if (audioPos >= 128) {
          audioPos = 0;
          queue1.playBuffer();
        }
      }
    }
#endif


    //Sync with framerate
    while (micros() - m < rate_us) {
      ;
    }
  }

  file.close();
  Serial.println("EOF");
}


void setup() {

#if USE_AUDIO
  AudioMemory(10);
#endif

  SD.begin();
  SPI.begin();

  tft.begin();
  tft.fillScreen(ILI9341_BLUE);
  tft.refresh();

  delay(500);

}

void loop() {
  playVideo(filename);
}
