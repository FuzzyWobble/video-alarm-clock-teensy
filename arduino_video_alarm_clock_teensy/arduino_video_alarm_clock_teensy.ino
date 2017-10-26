//fuzzywobble.com
//instructables://

// This sketch works with std 180MHz and less
// ! must edit Audio library for this to work !

#include <SPI.h>
#include <ILI9341_t3DMA.h>
#include <SdFat.h>

int mode;
int prev_mode;
//modes
//0 - clock running, waiting for alarm if set
//1 - time set
//2 - alarm set
//3 - alarm playing, return to clock running when turned off


//video pins for teensy3.6 w/ ILI9341
#define TFT_TOUCH_CS    38
#define TFT_TOUCH_INT   37
#define TFT_DC          20
#define TFT_CS          21
#define TFT_RST         255 //send to 5V
#define TFT_SCLK        14
#define TFT_MOSI        28
#define TFT_MISO        39
ILI9341_t3DMA tft = ILI9341_t3DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

SdFatSdioEX SD;

File file;

const char filename[] = "wake_up_wake_up_5.bin";

#include <Audio.h>
AudioPlayQueue           queue1;         //xy=183,163
AudioOutputAnalog        dac1;           //xy=376,154 //connect to dac0 on teensy (A21)
AudioConnection          patchCord1(queue1, dac1);

const float fps = 23.976; //!!! edit this to match framerate !!!
const int rate_us = 1e6 / fps;
const uint32_t framesize = 153600;
uint8_t header[512];
uint16_t audiobuf[16 * 320];
uint32_t audioPos = 0;
int16_t *lb = NULL;

#include <font_LiberationMonoBold.h>

#include <Encoder.h>
Encoder encoder_hours(3,2); //set encoder for hours (if they are spinning the wrong way just switch the order of the numbers)
Encoder encoder_min(5,4); //set encoder for min (if they are spinning the wrong way just switch the order of the numbers)
long encoder_read_hours,encoder_read_min; //readings from encoders
long encoder_read_hours_prev,encoder_read_min_prev; //readings from encoders previous

bool flag_changed;
bool alarm_on;
int pin_set_time,pin_set_alarm,pin_alarm_on;

int hour_val,min_val,sec_val; //time, hours/min
int hour_set_val,min_set_val; //time set, hours/min
int hour_alarm_set_val,min_alarm_set_val; //alarm set, hours/min

int min_val_prev;
float dist_draw;

String time_as_string; //turn time into string to draw

#include <TimeLib.h>
#include <TimeAlarms.h>

int px,py; //point for pixel draw







//================================================================
//                           SETUP
//================================================================

void setup() {

  mode = 0;
  alarm_on = false;
  
  prev_mode = 0;

  Serial.begin(9600);

  AudioMemory(10);
  
  SD.begin();
  
  SPI.begin();
  
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.refresh();
  
  tft.setFont(LiberationMono_96_Bold);
  
  pin_set_time = 7;
  pin_set_alarm = 8;
  pin_alarm_on = 6;
  pinMode(pin_set_time,INPUT_PULLUP);
  pinMode(pin_set_alarm,INPUT_PULLUP);
  pinMode(pin_alarm_on,INPUT_PULLUP);

  flag_changed = false;
  

  //default times on power up
  hour_val = 4;
  min_val = 20; 
  sec_val = 0;
  min_val_prev = 20;
  hour_set_val = 4;
  min_set_val = 20; 
  hour_alarm_set_val = 8;
  min_alarm_set_val = 20; 
  setTime(hour_set_val,min_set_val,0,1,1,11);

  dist_draw = 1.0;

  delay(500);

}







//================================================================
//                              LOOP 
//================================================================

void loop() {

  if(mode!=3){
  if(digitalRead(pin_set_alarm)==HIGH && digitalRead(pin_set_time)==HIGH){mode=0;}
  if(digitalRead(pin_set_time)==LOW){mode=1;}
  if(digitalRead(pin_set_alarm)==LOW){mode=2;}
  }
  if(digitalRead(pin_alarm_on)==LOW){alarm_on=true;}else{alarm_on=false;}

  if(mode!=prev_mode){
    Serial.println("mode change");
    tft.fillScreen(ILI9341_BLACK);
    prev_mode = mode;
    if(mode==1 || mode==2){flag_changed = true;}
  }
   //modes
  //0 - clock running, checking for alarm time
  //1 - time set
  //2 - alarm set
  //3 - alarm playing, return to clock running when done

  if(mode==0){

    hour_val = hour();
    min_val = minute();
    sec_val = second();

    if(min_val!=min_val_prev){
      min_val_prev = min_val;
      dist_draw = 1.0;
      tft.fillScreen(ILI9341_BLACK);
    }
    
    tft.setCursor(0, 0);
    px = 177 + cos(millis()/500.0)*dist_draw;
    py = 120 + sin(millis()/500.0)*dist_draw;
    tft.drawPixel(px,py,0x4208);
    dist_draw += 0.005;

    draw_time(hour_val,min_val,false,false);
    
    Serial.println("mode 0, "+(String)hour_val+":"+(String)min_val);
    if(alarm_on==true){
      Serial.println("alarm on, "+(String)hour_alarm_set_val+":"+(String)min_alarm_set_val);
    }

    //check if alarm goes off
    //called using Alarm.alarmRepeat instead
    if(alarm_on==true){
      if((int)hour_val==(int)hour_alarm_set_val){
        if((int)min_val==(int)min_alarm_set_val){
          mode = 3;
        }
      }
      delay(500);
    }

  }else if(mode==1){ //time set
    
    encoder_read_hours = encoder_hours.read();
    if(encoder_read_hours>encoder_read_hours_prev+3){
      hour_set_val++; if(hour_set_val>23){hour_set_val=0;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    if(encoder_read_hours<encoder_read_hours_prev-3){
      hour_set_val--; if(hour_set_val<0){hour_set_val=23;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    encoder_read_min = encoder_min.read();
    if(encoder_read_min>encoder_read_min_prev+3){
      min_set_val++; if(min_set_val>59){min_set_val=0;}  
      encoder_read_min_prev = encoder_read_min;
      flag_changed = true;
    }
    if(encoder_read_min<encoder_read_min_prev-3){
      min_set_val--; if(min_set_val<0){min_set_val=59;}  
      encoder_read_min_prev = encoder_read_min;
      flag_changed = true;
    }

    if(flag_changed == true){
      Serial.println("mode 1, flag");
      hour_val = hour_set_val;
      min_val = min_set_val;
      setTime(hour_set_val,min_set_val,0,1,1,11);
      flag_changed = false;
      tft.fillScreen(ILI9341_BLACK);
      draw_time(hour_set_val,min_set_val,true,false);
    }else{
      Serial.println("mode 1");
    }
     
  }else if(mode==2){ //alarm set

     encoder_read_hours = encoder_hours.read();
    if(encoder_read_hours>encoder_read_hours_prev+3){
      hour_alarm_set_val++; if(hour_alarm_set_val>23){hour_alarm_set_val=0;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    if(encoder_read_hours<encoder_read_hours_prev-3){
      hour_alarm_set_val--; if(hour_alarm_set_val<0){hour_alarm_set_val=23;}  
      encoder_read_hours_prev = encoder_read_hours;
      flag_changed = true;
    }
    
    encoder_read_min = encoder_min.read();
    if(encoder_read_min>encoder_read_min_prev+3){
      min_alarm_set_val++; if(min_alarm_set_val>59){min_alarm_set_val=0;}  
      encoder_read_min_prev = encoder_read_min;
      flag_changed = true;
    }
    if(encoder_read_min<encoder_read_min_prev-3){
      min_alarm_set_val--; if(min_alarm_set_val<0){min_alarm_set_val=59;}  
      encoder_read_min_prev = encoder_read_min;
      flag_changed = true;
    }

    if(flag_changed == true){
      Serial.println("mode 2, flag");
      //Alarm.alarmRepeat(hour_alarm_set_val,min_alarm_set_val,0,playAlarm);
      flag_changed = false;
      tft.fillScreen(ILI9341_BLACK);delay(15);
      draw_time(hour_alarm_set_val,min_alarm_set_val,false,true);
    }else{
      Serial.println("mode 2");
    }
    
  }else if(mode==3){

    //SPI.begin();
//    tft.fillScreen(ILI9341_BLUE);
//    tft.refresh();
//    delay(500);
    playVideo(filename);
    //SPI.end();
    
  }else{

    //uhoh
    
  }
 
}
void playAlarm(){
  if(alarm_on==true){
    mode = 3;
  }
}






//================================================================
//                    DRAW TIME TO ILI9341 
//================================================================

int nx,ny;
void draw_time(int hr, int mn, bool setting_time, bool setting_alarm){

    if(setting_time==true){
      tft.setTextColor(ILI9341_WHITE);
    }else if(setting_alarm==true){
      tft.setTextColor(ILI9341_RED);
    }else if(alarm_on==true){
      tft.setTextColor(ILI9341_PINK);
    }else{
      tft.setTextColor(ILI9341_PURPLE);
    }

    time_as_string = "";
    if(hr<10){
      time_as_string = "0"+(String)hr;
    }else{
      time_as_string = (String)hr;
    }
    tft.setCursor(93, 25);
    tft.print(time_as_string);

    time_as_string = "";
    if(mn<10){
      time_as_string = "0"+(String)mn;
    }else{
      time_as_string = (String)mn;
    }
    tft.setCursor(93, 130);
    tft.print(time_as_string);
   
}








//================================================================
//                      PLAY VIDEO ON ILI9341 
//================================================================

//function to play video with audio. written by https://github.com/FrankBoesing/ILI9341_t3DMA

void playVideo(const char * filename) {

  Serial.println("playing video");
  
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
  Serial.println("Video file done");
  mode=0;
  
}





