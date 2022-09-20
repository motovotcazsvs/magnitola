#include <LiquidCrystal.h> //лсд дисплей
#include <iarduino_RTC.h> //часы реального времени
#include <IRremote.h> //ик модуль
#include <OneWire.h> //для термодатчика
#include <SoftwareSerial.h> //библиотека команд плеера
#include <DFPlayer_Mini_Mp3.h> //библиотека ункций плеера
#include <Wire.h> //для дисплея наверное, используется в радио 
#include <radio.h> //библиотека функций радио 
#include <rda5807M.h> //библиотека для модуля радио

iarduino_RTC time(RTC_DS3231); //подключаем чассы к пинам SDA - A4, SCL - A5 (к пинам для I2C)         
LiquidCrystal lcd(8, 9, 4, 5, 6, 7); //подключение дисплея к пинам для I2C
int RECV_PIN = 3; //S подключаем
IRrecv irrecv(RECV_PIN); //пин сигнала
decode_results results; //декодируем нажите кнопки
OneWire ds(2); //ко второму пину данные термодатчика
RDA5807M radio; //обьект для радио

enum REGIM {OB, PL, FM}; //режимы магнитолы обычный, плеер, радио
REGIM regim = OB;

byte customChar[8] = {  //символ градуса
  0b00111,
  0b00101,
  0b00111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000
};

int vol = 1;
int track = 1;
RADIO_FREQ frequency = 10740;
int one = -1;
int two = -1;
int three = -1;
int four = -1;
int five = -1;
bool paus = false;
boolean play_state; 
boolean numberTrack = false;
boolean numberFrequency = false;

//
enum KEYSTATE {KEYSTATE_NONE, KEYSTATE_SELECT, KEYSTATE_LEFT, KEYSTATE_UP, KEYSTATE_DOWN, KEYSTATE_RIGHT} __attribute__((packed));
KEYSTATE getLCDKeypadKey();

KEYSTATE getLCDKeypadKey() {
  static unsigned long lastChange = 0;
  static KEYSTATE lastKey = KEYSTATE_NONE;

  unsigned long now = millis();
  KEYSTATE newKey;

  // Get current key state
  int v = analogRead(A0); // the buttons are read from the analog0 pin
  
  if (v < 100) {
    newKey = KEYSTATE_RIGHT;
  } else if (v < 200) {
    newKey = KEYSTATE_UP;
  } else if (v < 400) {
    newKey = KEYSTATE_DOWN;
  } else if (v < 600) {
    newKey = KEYSTATE_LEFT;
  } else if (v < 800) {
    newKey = KEYSTATE_SELECT;
  } else {
    newKey = KEYSTATE_NONE;
  }

  if (newKey != lastKey) {
    // a new situation - just remember but do not return anything pressed.
    lastChange = now;
    lastKey = newKey;
    return (KEYSTATE_NONE);

  } else if (lastChange == 0) {
    // nothing new.
    return (KEYSTATE_NONE);

  } if (now > lastChange + 50) {
    // now its not a bouncing button any more.
    lastChange = 0; // don't report twice
    return (newKey);

  } else {
    return (KEYSTATE_NONE);

  } // if
} // getLCDKeypadKey()


void DisplayFrequency(RADIO_FREQ f)
{
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  lcd.setCursor(0, 1); 
  lcd.print(s);
} 

void setup() {
  irrecv.enableIRIn(); // Start the receiver
  lcd.begin(16, 2);
  lcd.createChar(0, customChar);
  time.begin();
  Serial.begin (9600);
  mp3_set_serial (Serial);
  delay(800);
  mp3_set_volume (vol);
  lcd.setCursor(3,1);
  lcd.print(time.gettime("d.m.Y"));

  radio.init();
  radio.setMono(true);
  radio.setBassBoost(true);
  radio.setSoftMute(false);
  radio.setMute(false);
  radio.setVolume(15);
}

void ok(){
  if (regim == OB)
  {
    regim = PL;
    Wire.beginTransmission(0x10);
    Wire.write(0b00100000); //2H, отключение радио
    Wire.write(0b00000000); //2L, отключение питания радио
    Wire.endTransmission();
    delay(100);
    mp3_set_device(2);
    delay(100);
    mp3_play (track);
    lcd.setCursor(0, 1); 
    lcd.print("                       ");
    DFplayer();
  }
  else if (regim == PL)
  {
    regim = FM;
    delay(100);
    mp3_set_device(4);
    radio.setFrequency(frequency);
    delay(100);
    lcd.setCursor(0, 1); 
    lcd.print("                       ");
    FMradio();
    frequency = radio.getFrequency();
  }
  else if (regim == FM)
  {
    regim = OB;
    delay(100);
    mp3_set_device(4);
    Wire.beginTransmission(0x10);
    Wire.write(0b00100000); //2H, отключение радио
    Wire.write(0b00000000); //2L, отключение питания радио
    Wire.endTransmission();
    lcd.setCursor(0, 1); 
    lcd.print("                       ");
    lcd.setCursor(3,1);
    lcd.print(time.gettime("d.m.Y"));
  }
}

void right(){
  if (regim == PL)
  {
    track++;
    if (track > 2999) track = 2999;
    delay(100);
    mp3_play(track);
    lcd.setCursor(0, 1);
    lcd.print("                       ");
    lcd.setCursor(0, 1);
    lcd.print("Next track >>");
    delay(1000);
    DFplayer();
  }
  else if (regim == FM)
  {
    radio.seekUp(true);
    delay(100);
    FMradio();
  }
}

void left(){
  if (regim == PL)
  {
    track--;
    if (track <= 0) track = 1;
    delay(100);
    mp3_play(track);
    lcd.setCursor(0, 1);
    lcd.print("                       ");
    lcd.setCursor(0, 1);
    lcd.print("Previous track<<");
    delay(1000);
    DFplayer();
  }
  else if (regim == FM)
  {
    radio.seekDown(true);
    delay(100);
    FMradio();
  }
}

void volMax(){
  if (regim == PL)
  {
    if(vol >= 30) vol = 29; 
    delay(100);
    mp3_set_volume(++vol);
    DFplayer();
  }
  else if (regim == FM)
  {
    int v = radio.getVolume();
    if (v < 15) radio.setVolume(++v);
    FMradio();
  }
}

void volMin(){
  if (regim == PL)
  {
    if(vol <= 0)vol = 1;
    delay(100);
    mp3_set_volume(--vol);
    DFplayer();
  }
  else if (regim == FM)
  {
    int v = radio.getVolume();
    if (v > 0) radio.setVolume(--v);
    FMradio();
  }
}

void zvezda(){
  if (regim == PL)
  {
    paus = !paus;
    if (paus)
    {
      delay(100);
      mp3_pause();
      lcd.setCursor(0, 1); 
      lcd.print("                       ");
      lcd.setCursor(0, 1);
      lcd.print("Pause");
      delay(1000);
    }
    else if (!paus)
    {
      delay(100);
      mp3_play();
      lcd.setCursor(0, 1); 
      lcd.print("                       ");
      DFplayer(); 
    }
  }
  else if (regim == FM)
  {
    static boolean fmStereo = false;
    fmStereo = !fmStereo;
    radio.setMono(fmStereo);
  }
}

void reshotka(){
  lcd.setCursor(0, 1);
  lcd.print("#                      ");
  lcd.setCursor(0, 1);
  if (regim == PL)
  {
    numberTrack = !numberTrack;
    if(!numberTrack)
    {
      one = -1;
      two = -1;
      three = -1;
      four = -1;
      delay(100);
      mp3_play(track);
      DFplayer();
    }
  }
  else if (regim == FM)
  {
    numberFrequency = !numberFrequency;
    if(!numberFrequency)
    {
      one = -1;
      two = -1;
      three = -1;
      four = -1;
      five = -1;
      radio.setFrequency(frequency);
      delay(100);
      FMradio();
    }
  }
}

void number(int num){
  if (regim == PL)
  {
    if(numberTrack){
      if(one == -1){
        one = num;
        track = one;
        lcd.setCursor(1,1);
      }
      else if(two == -1){
        two = num;
        track = (one * 10) + two;
        lcd.setCursor(2,1);
      }
      else if(three == -1){
        three = num;
        track = (one * 100) + (two * 10) + three;
        lcd.setCursor(3,1);
      }
      else if(four == -1){
        four = num;
        track = (one * 1000) + (two * 100) + (three * 10) + four;
        lcd.setCursor(4,1);
      }
      else {
        lcd.setCursor(0, 1); 
        lcd.print("                       ");
        lcd.print("error!");
        delay(1000);
        DFplayer();
      }
    }
  }
  else if (regim == FM)
  {
     if(numberFrequency){
      if(one == -1){
        one = num;
        frequency = one;
        lcd.setCursor(1,1);
      }
      else if(two == -1){
        two = num;
        frequency = (one * 10) + two;
        lcd.setCursor(2,1);
      }
      else if(three == -1){
        three = num;
        frequency = (one * 100) + (two * 10) + three;
        lcd.setCursor(3,1);
      }
      else if(four == -1){
        four = num;
        frequency = (one * 1000) + (two * 100) + (three * 10) + four;
        lcd.setCursor(4,1);
      }
      else if(five == -1){
        five = num;
        frequency = (one * 10000) + (two * 1000) + (three * 100) + (four * 10) + five;
        lcd.setCursor(5,1);
      }
      else {
        lcd.setCursor(0, 1); 
        lcd.print("                       ");
        lcd.print("error!");
        delay(1000);
        FMradio();
      }
    }
    else if(!numberFrequency){
      switch(num){
      case 1:
          frequency = 10740;
          radio.setFrequency(frequency);
          delay(100);
          FMradio();       
          break;
      case 2:
          frequency = 10700;
          radio.setFrequency(frequency);
          delay(100);
          FMradio();    
          break;
      case 3:
          frequency = 10240;
          radio.setFrequency(frequency);
          delay(100);
          FMradio();    
          break;
      case 4:
          frequency = 8930;
          radio.setFrequency(frequency);
          delay(100);
          FMradio();    
          break;
      }
    }  
  }
}

void DFplayer(){
  lcd.setCursor(0, 1); 
  lcd.print("                       ");
  lcd.setCursor(0, 1);
  lcd.print("Track ");
  lcd.print(track);
  lcd.setCursor(10, 1);
  lcd.print("Vol ");
  lcd.print(vol);
}

void FMradio(){
  RADIO_FREQ f = radio.getFrequency();
  DisplayFrequency(f);

  lcd.setCursor(10, 1);
  lcd.print("      ");
  lcd.setCursor(10, 1);
  lcd.print("Vol ");
  lcd.print(radio.getVolume());
}

void ds18b20(){
  byte data[2];
  ds.reset(); 
  ds.write(0xCC);
  ds.write(0x44);
  //delay(750);
  
  ds.reset();
  ds.write(0xCC);
  ds.write(0xBE);
  data[0] = ds.read(); 
  data[1] = ds.read();
  int Temp = (data[1]<< 8)+data[0];
  Temp = Temp>>4;
  if(Temp < 0){
    lcd.setCursor(11,0);
    lcd.print("-");
  }
  else if(Temp > 0){
    lcd.setCursor(11,0);
    lcd.print("+");
  }
  else
    lcd.setCursor(12,0);
  
  lcd.print(Temp);
  lcd.write((uint8_t)0);
  lcd.print("C");
}

void loop() {
  lcd.setCursor(0,0);
  lcd.print(time.gettime("H:i")); //выводим время
  ds18b20();
  
  KEYSTATE k = getLCDKeypadKey();

  if (k == KEYSTATE_RIGHT) {
    right();
  } 
  else if (k == KEYSTATE_UP) { 
    volMax();
  } 
  else if (k == KEYSTATE_DOWN) {  
    volMin(); 
  } 
  else if (k == KEYSTATE_LEFT) {
    left();
  } 
  else if (k == KEYSTATE_SELECT) {
    ok();
  } 
  
  if (irrecv.decode(&results)) {
    switch(results.value){
    case 0xFF02FD:
      ok();  
      break;
    case 0xFF629D:
      volMax();
      break;
    case 0xFFA857:
      volMin();
      break;
    case 0xFFC23D:
      right();
      break;
    case 0xFF22DD:
      left();
      break;
    case 0xFF6897:
      number(1);
      lcd.print("1");
      break;
    case 0xFF9867:
      number(2);
      lcd.print("2");
      break;
    case 0xFFB04F:
      number(3);
      lcd.print("3");
      break;
    case 0xFF30CF:  
      number(4);
      lcd.print("4");
      break;
    case 0xFF18E7:   
      number(5);
      lcd.print("5");
      break;
    case 0xFF7A85:   
      number(6);
      lcd.print("6");
      break;
    case 0xFF10EF:
      number(7);
      lcd.print("7");
      break;
    case 0xFF38C7:
      number(8);
      lcd.print("8");
      break;
    case 0xFF5AA5:
      number(9);
      lcd.print("9");
      break;
    case 0xFF4AB5:
      number(0);
      lcd.print("0");
      break;
    case 0xFF42BD:
      zvezda();
      break;
    case 0xFF52AD:
      reshotka();
      break;
    }
    irrecv.resume(); // Receive the next value
    delay(100);
  }
  
  if(regim == PL && paus == false){
    play_state = digitalRead(11);
    if(play_state == HIGH){
      delay(100);
      mp3_play(++track);
      DFplayer();
    }
    delay(100);
  }
}

