//fuzzywobble.com

// This sketch works with std 180MHz and less, but better with 240MHZ, or 144MHz and overclocked F_BUS to the max.
// Must edit Audio library for this to work

#include <SPI.h>
#include <ILI9341_t3DMA.h>
#include <SdFat.h>

//video pins for teensy3.6 w/ ILI9341
#define TFT_TOUCH_CS    38
#define TFT_TOUCH_INT   37
#define TFT_DC          20
#define TFT_CS          21
#define TFT_RST         255
#define TFT_SCLK        14
#define TFT_MOSI        28
#define TFT_MISO        39
ILI9341_t3DMA tft = ILI9341_t3DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

SdFatSdioEX SD;

File file;

const char filename[] = "wake_up_wake_up.bin";

#include <Audio.h>
AudioPlayQueue           queue1;         //xy=183,163
AudioOutputAnalog        dac1;           //xy=376,154
AudioConnection          patchCord1(queue1, dac1);

const float fps = 23.98; //!!! edit this to match framerate !!!
const int rate_us = 1e6 / fps;
const uint32_t framesize = 153600;
uint8_t header[512];
uint16_t audiobuf[16 * 320];
uint32_t audioPos = 0;
int16_t *lb = NULL;

int hour_val,min_val; 
String time_as_string; //turn time into string to draw

#include <font_LiberationMonoBold.h>
#include <font_LiberationMonoBoldItalic.h>
#include <font_LiberationMono.h>
#include <font_LiberationMonoItalic.h>

#include <Encoder.h>
Encoder encoder_hours(1,2); //set encoder for hours
Encoder encoder_mins(3,4); //set encoder for mins
long encoder_read_hours,encoder_read_mins; //readings from encoders
long encoder_read_hours_prev,encoder_read_mins_prev; //readings from encoders previous
int hour_set_val,min_set_val; //time, hours/mins
int hour_alarm_set_val,min_alarm_set_val; //alarm, hours/mins
bool flag_changed;
bool alarm_on;
int pin_set_time,pin_set_alarm;

#include <TimeLib.h>
#include <TimeAlarms.h>



void setup() {

  AudioMemory(10);
  SD.begin();
  SPI.begin();
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.refresh();
  

  pin_set_time = 1;
  pin_set_alarm = 2;
  pinMode(pin_set_time,INPUT_PULLUP);
  pinMode(pin_set_alarm,INPUT_PULLUP);

  flag_changed = false;
  alarm_on = false;

  hour_set_val = 6;
  min_set_val = 16;

  delay(500);

}

void loop() {

  if(digitalRead(pin_set_time)==LOW){mode=1;}
  if(digitalRead(pin_set_alarm)==LOW){mode=2;}
  if(digitalRead(pin_set_alarm)==HIGH && digitalRead(pin_set_time)==HIGH){mode=0;}
  if(digitalRead(pin_alarm_on)==LOW){alarm_on=true;}else{alarm_on=false;}

   //modes
  //0 - clock running, checking for alarm time
  //1 - time set
  //2 - alarm set
  //3 - alarm playing, return to clock running when done

  if(mode==0){

    draw_time(hour_val,min_val,false,false);

  }else if(mode==1){ //time set
    
    encoder_read_hours = encoder_hours.read();
    if(encoder_read_hours>encoder_read_hours_prev){
      hour_set_val++; if(hour_set_val>24){hour_set_val=0;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    if(encoder_read_hours<encoder_read_hours_prev){
      hour_set_val--; if(hour_set_val<0){hour_set_val=24;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    encoder_read_mins = encoder_mins.read();
    if(encoder_read_mins>encoder_read_mins_prev){
      mins_set_val++; if(mins_set_val>24){mins_set_val=0;}  
      encoder_read_mins_prev = encoder_read_mins;
      flag_changed = true;
    }
    if(encoder_read_mins<encoder_read_mins_prev){
      mins_set_val--; if(mins_set_val<0){mins_set_val=24;}  
      encoder_read_mins_prev = encoder_read_mins;
      flag_changed = true;
    }

    if(flag_changed == true){
      setTime(hour_set_val,min_set_val,0,1,1,11);
      flag_changed = false;
      draw_time(hour_set_val,min_set_val,true,false);
    }
     
  }else if(mode==2){ //alarm set

     encoder_read_hours = encoder_hours.read();
    if(encoder_read_hours>encoder_read_hours_prev){
      hour_alarm_set_val++; if(hour_alarm_set_val>24){hour_alarm_set_val=0;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    if(encoder_read_hours<encoder_read_hours_prev){
      hour_alarm_set_val--; if(hour_alarm_set_val<0){hour_alarm_set_val=24;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    
    encoder_read_mins = encoder_mins.read();
    if(encoder_read_mins>encoder_read_mins_prev){
      mins_alarm_set_val++; if(mins_alarm_set_val>24){mins_alarm_set_val=0;}  
      encoder_read_mins_prev = encoder_read_mins;
      flag_changed = true;
    }
    if(encoder_read_mins<encoder_read_mins_prev){
      mins_alarm_set_val--; if(mins_alarm_set_val<0){mins_alarm_set_val=24;}  
      encoder_read_mins_prev = encoder_read_mins;
      flag_changed = true;
    }

    if(flag_changed == true){
      Alarm.alarmRepeat(hour_alarm_set_val,min_alarm_set_val,0, playAlarm);
      flag_changed = false;
      draw_time(hour_alarm_set_val,min_alarm_set_val,false,true);
    }
    
  }else if(mode==3){
    
    playVideo(filename);
    
  }else{
    
  }
 
}



void playAlarm(){
  if(alarm_on==true){
    mode = 3;
  }
}




int16_t  x1, y1;
uint16_t w, h;
void draw_time(int h, int m, bool is_time, bool is_alarm){

    //https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
    time_as_string = (String)h+":"+(String)m;    
    tft.getTextBounds(time_as_string, x, y, &x1, &y1, &w, &h);
    if(is_time=false && is_alarm=false){ //running
      tft.setTextColor(ILI9341_YELLOW);
    }else if(is_time=true){ //set time
      tft.setTextColor(ILI9341_YELLOW);
    }else if(is_alarm=true){ //set alarm
      tft.setTextColor(ILI9341_YELLOW);
    }
    tft.setFont(LiberationMono_24_Bold_Italic);
    tft.setCursor(20,20);
    tft.print();
      
}




//function to play video with audio. written by https://github.com/FrankBoesing/ILI9341_t3DMA
void playVideo(const char * filename) {
  
  int bytesread;
  
  if (!file.open(filename, O_READ)) {
    SD.errorHalt("open failed");
    return;
  }
  
  file.read(header, 512);  //read header - not used yet
  
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

    bytesread = file.read(audiobuf, sizeof(audiobuf));
    if(!bytesread){
      break;
    }

    if(digitalRead(pin_alarm_on)==HIGH){ //alarm is turned off
      alarm_on = false;
      mode = 0;
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

    //Sync with framerate
    while (micros() - m < rate_us) {
      ;
    }
  }

  file.close(); //file done
  mode=0;
  
}





