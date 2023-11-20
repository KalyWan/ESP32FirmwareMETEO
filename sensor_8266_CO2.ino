
#define RESET_CLOCK 0     // сброс часов на время загрузки прошивки (для модуля с несъёмной батарейкой). Не забудь поставить 0 и прошить ещё раз!
#define SENS_TIME 10000   // время обновления показаний сенсоров на экране, миллисекунд

#ifndef STASSID
#define STASSID "CLOCK-SSID"
#define STAPSK  "admin123456"
#endif
String hostName ="WiFiClock";
char* host; //The DNS hostname
uint8_t mac[6];
int otaCount=300; //timeout in sec for OTA mode
int otaFlag=0;
int otaFlag_=0;
#include <Ticker.h>
Ticker otaTickLoop;

#define WEEK_LANG 1       // язык дня недели: 0 - английский, 1 - русский
#define CO2_SENSOR 1      // включить или выключить поддержку/вывод с датчика СО2 (1 вкл, 0 выкл)
#define DISPLAY_ADDR 0x27 // адрес платы дисплея: 0x27 или 0x3f. Если дисплей не работает - смени адрес! На самом дисплее адрес не указан

// пины
#define BTN_PIN 12 //D8  // пин кнопки притянутый до земли резистором 10 кОм (сенсорный ТТР223 можно без резистора)

#define MHZ_RX 15 //D3
#define MHZ_TX 14 //D4

// библиотеки
//#include <EEPROM.h>
//#include <ESP8266FtpServer.h>
#include <LiquidCrystal_I2C.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
extern "C" {
  #include "user_interface.h" //Needed for the reset command
}

#if (CO2_SENSOR == 1)
  #include <MHZ19_uart.h>
  MHZ19_uart mhz19;
#endif

#include "GyverButton.h"

const char* clock_ssid = STASSID;
const char* clock_pass = STAPSK;

// NTP Servers:
static const char ntpServerName[] = "ru.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";

String ssid = "Kharonix1";       //  your network SSID (name)
String pass = "trapeciya10a";       // your network password
int TIMEZONE = 03;
String mqtt_ip = "192.168.3.2";    // MQTT ip address
unsigned int ip_addr[]={0,0,0,0};
String mqtt_port = "1883";  // MQTT port
String mqtt_auth = "mes";  // MQTT user name
String mqtt_pass = "admin";  // MQTT user password
String mqtt_CO2 = "sensor_8266_CO2/CO2";   // MQTT топик датчика CO2
String mqtt_Hum = "sensor_8266_CO2/humid";   // MQTT топик датчика влажности
String mqtt_Temp = "sensor_8266_CO2/temp";  // MQTT топик датчика температуры
String mqtt_Press = "sensor_8266_CO2/press"; // MQTT топик датчика давления

WiFiClient espClient;
IPAddress mqtt_server(0, 0, 0, 0);
PubSubClient client(espClient, mqtt_server);

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);
ESP8266WebServer server(80);

LiquidCrystal_I2C lcd(DISPLAY_ADDR, 20, 4);

RTC_DS3231 rtc;

Adafruit_BME280 bme;

unsigned long sensorsTimer = SENS_TIME;
unsigned long drawSensorsTimer = SENS_TIME;
unsigned long clockTimer = 500;

unsigned long RTCsyncTimer = ((long)60 * 60 * 1000);    // 1 час
unsigned long RTCsyncTimerD = 0;

unsigned long WEBsyncTimer = ((long)3 * 1000);    // 3 сек
unsigned long WEBsyncTimerD = 0;

unsigned long MQTTsyncTimer = ((long)60 * 1000);    // 60 сек
unsigned long MQTTsyncTimerD = 0;

unsigned long sensorsTimerD = 0;
unsigned long drawSensorsTimerD = 0;
unsigned long clockTimerD = 0;

GButton button(BTN_PIN, LOW_PULL, NORM_OPEN);

int8_t hrs, mins, secs;

byte mode0scr = 4;
/* Режимы экрана
  0 - Время
  1 - Температура
  2 - Влажность
  3 - Давление
  4 - Содержание СО2
*/
boolean bigDig = false;   // true - цифры на главном экране на все 4 строки (для LCD 2004) 

// переменные для вывода
float dispTemp;
byte dispHum;
int dispPres;
int dispCO2 = -1;
char myIP[] = "000.000.000.000";
int TempCorrection = -2; // корректировка температуры в градусах
int HumCorrection = 10; // корректировка температуры в градусах

/*
  Характеристики датчика BME:
  Температура: от-40 до + 85 °C
  Влажность: 0-100%
  Давление: 300-1100 hPa (225-825 ммРтСт)
  Разрешение:
  Температура: 0,01 °C
  Влажность: 0.008%
  Давление: 0,18 Pa
  Точность:
  Температура: +-1 °C
  Влажность: +-3%
  Давление: +-1 Па
*/

// символы
//byte rowS[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b10001,  0b01010,  0b00100,  0b00000};   // стрелка вниз 
//byte row7[8] = {0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
//byte row6[8] = {0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};   // в т.ч. для четырехстрочных цифр 2, 3, 4, 5, 6, 8, 9, 0 
//byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 0, для четырехстрочных цифр 2, 3, 4, 5, 6, 8, 9 
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 4 
//byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};

// цифры //  
uint8_t UB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};   // для двустрочных 7, 0   // для четырехстрочных 2, 3, 4, 5, 6, 8, 9
uint8_t UMB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
uint8_t LMB[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
uint8_t LM2[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};  // для двустрочных 4
uint8_t UT[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000};   // для четырехстрочных 2, 3, 4, 5, 6, 7, 8, 9, 0

uint8_t KU[8] = {0b00000,  0b00000,  0b00000,  0b00001,  0b00010,  0b00100,  0b01000,  0b10000};   // для верхней части %
uint8_t KD[8] = {0b00001,  0b00010,  0b00100,  0b01000,  0b10000,  0b00000,  0b00000,  0b00000};   // для нижней части %

// русские буквы 
uint8_t PP[8] = {0b11111,  0b10001,  0b10001,  0b10001,  0b10001,  0b10001,  0b10001,  0b00000};  // П
uint8_t BB[8] = {0b11111,  0b10000,  0b10000,  0b11111,  0b10001,  0b10001,  0b11111,  0b00000};  // Б
uint8_t CH[8] = {0b10001,  0b10001,  0b10001,  0b01111,  0b00001,  0b00001,  0b00001,  0b00000};  // Ч
//uint8_t II[8] = {0b10001,  0b10001,  0b10011,  0b10101,  0b11001,  0b10001,  0b10001,  0b00000};  // И
//uint8_t BM[8] = {0b10000,  0b10000,  0b10000,  0b11110,  0b10001,  0b10001,  0b11110,  0b00000};  // Ь
//uint8_t IY[8] = {0b01100,  0b00001,  0b10011,  0b10101,  0b11001,  0b10001,  0b10001,  0b00000};  // Й
//uint8_t DD[8] = {0b01110,  0b01010,  0b01010,  0b01010,  0b01010,  0b01010,  0b11111,  0b10001};  // Д
//uint8_t AA[8] = {0b11100,  0b00010,  0b00001,  0b00111,  0b00001,  0b00010,  0b11100,  0b00000};  // Э
//uint8_t IA[8] = {0b01111,  0b10001,  0b10001,  0b01111,  0b00101,  0b01001,  0b10001,  0b00000};  // Я
//uint8_t YY[8] = {0b10001,  0b10001,  0b10001,  0b11101,  0b10011,  0b10011,  0b11101,  0b00000};  // Ы
//uint8_t GG[8] = {0b11110,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000,  0b00000};  // Г
//uint8_t FF[8] = {0b00100,  0b01110,  0b10101,  0b10101,  0b10101,  0b01110,  0b00100,  0b00000};  // Ф
//uint8_t LL[8] = {0b01111,  0b01001,  0b01001,  0b01001,  0b01001,  0b01001,  0b10001,  0b00000};  // Л
//uint8_t ZZ[8] = {0b10101,  0b10101,  0b10101,  0b01110,  0b10101,  0b10101,  0b10101,  0b00000};  // Ж

void digSeg(byte x, byte y, byte z1, byte z2, byte z3, byte z4, byte z5, byte z6) {   // отображение двух строк по три символа с указанием кодов символов 
  lcd.setCursor(x, y);
  lcd.write(z1); lcd.write(z2); lcd.write(z3);
  if (x <= 11) lcd.print(" ");
  lcd.setCursor(x, y + 1);
  lcd.write(z4); lcd.write(z5); lcd.write(z6);
  if (x <= 11) lcd.print(" ");
}

void drawDig(byte dig, byte x, byte y) {        // рисуем цифры  ---------------------------------------
  if (bigDig) {
    switch (dig) {            // четырехстрочные цифры 
      case 0:
        digSeg(x, y, 255, 0, 255, 255, 32, 255);
        digSeg(x, y + 2, 255, 32, 255, 255, 3, 255);
        break;
      case 1:
        digSeg(x, y, 32, 255, 32, 32, 255, 32);
        digSeg(x, y + 2, 32, 255, 32, 32, 255, 32);
        break;
      case 2:
        digSeg(x, y, 0, 0, 255, 1, 1, 255);
        digSeg(x, y + 2, 255, 2, 2, 255, 3, 3);
        break;
      case 3:
        digSeg(x, y, 0, 0, 255, 1, 1, 255);
        digSeg(x, y + 2, 2, 2, 255, 3, 3, 255);
        break;
      case 4:
        digSeg(x, y, 255, 32, 255, 255, 1, 255);
        digSeg(x, y + 2, 2, 2, 255, 32, 32, 255);
        break;
      case 5:
        digSeg(x, y, 255, 0, 0, 255, 1, 1);
        digSeg(x, y + 2, 2, 2, 255, 3, 3, 255);
        break;
      case 6:
        digSeg(x, y, 255, 0, 0, 255, 1, 1);
        digSeg(x, y + 2, 255, 2, 255, 255, 3, 255);
        break;
      case 7:
        digSeg(x, y, 0, 0, 255, 32, 32, 255);
        digSeg(x, y + 2, 32, 255, 32, 32, 255, 32);
        break;
      case 8:
        digSeg(x, y, 255, 0, 255, 255, 1, 255);
        digSeg(x, y + 2, 255, 2, 255, 255, 3, 255);
        break;
      case 9:
        digSeg(x, y, 255, 0, 255, 255, 1, 255);
        digSeg(x, y + 2, 2, 2, 255, 3, 3, 255);
        break;
      case 10:
        digSeg(x, y, 32, 32, 32, 32, 32, 32);
        digSeg(x, y + 2, 32, 32, 32, 32, 32, 32);
        break;
    }
  }
  else {
    switch (dig) {            // двухстрочные цифры
      case 0:
        digSeg(x, y, 255, 1, 255, 255, 2, 255);
        break;
      case 1:
        digSeg(x, y, 32, 255, 32, 32, 255, 32);
        break;
      case 2:
        digSeg(x, y, 3, 3, 255, 255, 4, 4);
        break;
      case 3:
        digSeg(x, y, 3, 3, 255, 4, 4, 255);
        break;
      case 4:
        digSeg(x, y, 255, 0, 255, 5, 5, 255);
        break;
      case 5:
        digSeg(x, y, 255, 3, 3, 4, 4, 255);
        break;
      case 6:
        digSeg(x, y, 255, 3, 3, 255, 4, 255);
        break;
      case 7:
        digSeg(x, y, 1, 1, 255, 32, 255, 32);
        break;
      case 8:
        digSeg(x, y, 255, 3, 255, 255, 4, 255);
        break;
      case 9:
        digSeg(x, y, 255, 3, 255, 4, 4, 255);
        break;
      case 10:
        digSeg(x, y, 32, 32, 32, 32, 32, 32);
        break;
    }
  }
}

void drawPPM(int dispCO2, byte x, byte y) {     // Уровень СО2 крупно на главном экране  ----------------------------
  if (dispCO2 / 1000 == 0) drawDig(10, x, y);
  else drawDig(dispCO2 / 1000, x, y);
  drawDig((dispCO2 % 1000) / 100, x + 4, y);
  drawDig((dispCO2 % 100) / 10, x + 8, y);
  drawDig(dispCO2 % 10 , x + 12, y);
  lcd.setCursor(15, 0);
  lcd.print("ppm");
}

void drawPres(int dispPres, byte x, byte y) {   // Давление крупно на главном экране  ----------------------------
  drawDig((dispPres % 1000) / 100, x , y);
  drawDig((dispPres % 100) / 10, x + 4, y);
  drawDig(dispPres % 10 , x + 8, y);
  lcd.setCursor(15, 0);
  lcd.print("mm");
}

void drawTemp(float dispTemp, byte x, byte y) { // Температура крупно на главном экране  ----------------------------
  if (dispTemp / 10 == 0) drawDig(10, x, y);
  else drawDig(dispTemp / 10, x, y);
  drawDig(int(dispTemp) % 10, x + 4, y);
  drawDig(int(dispTemp * 10.0) % 10, x + 9, y);

  if (bigDig) {
    lcd.setCursor(x + 7, y + 3);
    lcd.write(1);             // десятичная точка
  }
  else {
    lcd.setCursor(x + 7, y + 1);
    lcd.write(0B10100001);    // десятичная точка
  }
  lcd.setCursor(15, 0);
  lcd.write(223);             // градусы
}

void drawHum(int dispHum, byte x, byte y) {   // Влажность крупно на главном экране  ----------------------------
  if (dispHum / 100 == 0) drawDig(10, x, y);
  else drawDig(dispHum / 100, x, y);
  if ((dispHum % 100) / 10 == 0) drawDig(0, x + 4, y);
  else drawDig(dispHum / 10, x + 4, y);
  drawDig(int(dispHum) % 10, x + 8, y);
  lcd.setCursor(15, 0);
  lcd.print("%");
}

void drawClock(byte hours, byte minutes, byte x, byte y) {    // рисуем время крупными цифрами -------------------------------------------
  if (hours > 23 || minutes > 59) return;
  if (hours / 10 == 0) drawDig(10, x, y);
  else drawDig(hours / 10, x, y);
  drawDig(hours % 10, x + 4, y);
  // тут должны быть точки. Отдельной функцией
  drawDig(minutes / 10, x + 8, y);
  drawDig(minutes % 10, x + 12, y);
}

#if (WEEK_LANG == 0)
static const char *dayNames[]  = {
  "Su",
  "Mo",
  "Tu",
  "We",
  "Th",
  "Fr",
  "Sa",
};
#else
static const char *dayNames[]  = {  // доработал дни недели на двухсимвольные русские (ПН, ВТ, СР....) 
  "BC",
  "\7H",
  "BT",
  "CP",
  "\7T",
  "\7T",
  "C\7",
};
#endif

void drawData() {                     // выводим дату -------------------------------------------------------------
  int Y = 0;
  DateTime now =rtc.now();
  if (mode0scr == 1) Y = 2;
  if (!bigDig) {              // если 4-х строчные цифры, то дату, день недели (и секунды) не пишем - некуда 
    lcd.setCursor(9, 3);
//    lcd.setCursor(15, 0 + Y);
    if (now.day() < 10) lcd.print(0);
    lcd.print(now.day());
    lcd.print(".");
    if (now.month() < 10) lcd.print(0);
    lcd.print(now.month());

    if (!bigDig) {
      loadClock();              // принудительно обновляем знаки, т.к. есть жалобы на необновление знаков в днях недели 
      lcd.setCursor(12, 2);
      int dayofweek = now.dayOfTheWeek();
      lcd.print(dayNames[dayofweek]);
      // if (hrs == 0 && mins == 0 && secs <= 1) loadClock();   // Обновляем знаки, чтобы русские буквы в днях недели тоже обновились. 
    }
  }
}

void loadClock() {
  DateTime now=rtc.now();
  if (bigDig) {                                     // для четырехстрочных цифр 
    lcd.createChar(0, UT);
    lcd.createChar(1, row3);
    lcd.createChar(2, UB);
    lcd.createChar(3, row5);
    lcd.createChar(4, KU);
    lcd.createChar(5, KD);
  }
  else {                                            // для двустрочных цифр 
    lcd.createChar(0, row2);
    lcd.createChar(1, UB);
    lcd.createChar(2, row3);
    lcd.createChar(3, UMB);
    lcd.createChar(4, LMB);
    lcd.createChar(5, LM2);
  }

  if (now.dayOfTheWeek() == 4)  {          // Для четверга в ячейку запоминаем "Ч", для субботы "Б", иначе "П" 
    lcd.createChar(7, CH);  // Ч 
  } else if (now.dayOfTheWeek() == 6) {
    lcd.createChar(7, BB);  // Б 
  } else {
    lcd.createChar(7, PP);  // П 
  }
}

void setup() {
//EEPROM.begin(16);
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  Serial.println("Mounting FS...");
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
  }

  Serial.println("");
  loadConfig();
  Serial.print("TIMEZONE: ");
  Serial.println(String(TIMEZONE));
  Serial.print("mqtt_ip: ");
  Serial.println(mqtt_ip);
  Serial.print("mqtt_port: ");
  Serial.println(mqtt_port);
  Serial.print("mqtt_auth: ");
  Serial.println(mqtt_auth);
  Serial.print("mqtt_pass: ");
  Serial.println(mqtt_pass);

  if( ssid.length() == 0 ) ssid = "*";
  if( pass.length() == 0 ) pass = "*";
  Serial.print("ssid: ");
  Serial.println(ssid);
  Serial.print("pass: ");
  Serial.println(pass);
  Serial.print("otaFlag: ");
  Serial.println(otaFlag);
  if (!otaFlag) {
    if (mqtt_server.fromString(mqtt_ip)) { //Указан корректный ip адрес
      String ip = mqtt_ip;
      unsigned int pos = ip.indexOf(".");
      unsigned int pos_last = ip.lastIndexOf(".");
      ip_addr[0] = ip.substring(0,pos).toInt();
      ip_addr[3] = ip.substring(pos_last+1).toInt();
      ip    = ip.substring(pos+1,pos_last);
      pos = ip.indexOf(".");
      ip_addr[1] = ip.substring(0,pos).toInt();
      ip_addr[2] = ip.substring(pos+1).toInt();
      IPAddress mqtt_server(ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3]);
      client.set_server(mqtt_server,mqtt_port.toInt()); 
      Serial.print("MQTT IP: "); Serial.println(mqtt_server);
      Serial.print("MQTT PORT: "); Serial.println(mqtt_port);
    } 
    else {client.set_server(mqtt_ip,mqtt_port.toInt()); Serial.print("MQTT name: "); Serial.println(mqtt_ip); Serial.print("MQTT PORT: "); Serial.println(mqtt_port);}
  
    delay(1000);
    lcd.clear();
    boolean status = true;

#if (CO2_SENSOR == 1)
    lcd.setCursor(0, 0);
    lcd.print(F("MHZ-19... "));
    Serial.print(F("MHZ-19... "));
    mhz19.begin(MHZ_TX, MHZ_RX);
    mhz19.setAutoCalibration(false);
    mhz19.getStatus();    // первый запрос, в любом случае возвращает -1
    delay(500);
    if (mhz19.getStatus() == 0) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERROR"));
      Serial.println(F("ERROR"));
      status = false;
    }
#endif

    lcd.setCursor(0, 1);
    lcd.print(F("RTC... "));
    Serial.print(F("RTC... "));
    delay(50);
    if (rtc.begin()) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERROR"));
      Serial.println(F("ERROR"));
      status = false;
    }

    lcd.setCursor(0, 2);
    lcd.print(F("BME280... "));
    Serial.print(F("BME280... "));
    delay(50);
    if (bme.begin(0x76,&Wire)) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERROR"));
      Serial.println(F("ERROR"));
      status = false;
    }
    lcd.setCursor(0, 3);
    if (status) {
      lcd.print(F("All good"));
      Serial.println(F("All good"));
    } else {
      lcd.print(F("Check wires!"));
      Serial.println(F("Check wires!"));
    }
    delay(1000);

  } //!otaFlag
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Connecting to WiFi"));
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.persistent(false);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  uint8_t mac[6];
  for (byte i = 1; i < 100; i++) {
    delay(100);
    Serial.print(".");
    if ( WiFi.status() == WL_CONNECTED ) break;
  }
  if ( WiFi.status() == WL_CONNECTED ) {
    lcd.print(F(" OK"));
    Serial.print("IP number assigned by DHCP is ");
    Serial.println(WiFi.localIP());
    Serial.println("Starting UDP");
    Udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);
    setSyncInterval(300);
    WiFi.macAddress(mac);
  }
  else {
    lcd.clear();
    lcd.print(F("Not connected to WiFi"));
    Serial.println("Not connected to WiFi");
  }

  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(clock_ssid, clock_pass);
    sprintf(myIP, "%s", WiFi.softAPIP().toString().c_str());
    Serial.print("WiFi SSID: ");
    Serial.println(clock_ssid);
    Serial.print("WiFi password: ");
    Serial.println(clock_pass);
    lcd.setCursor(0, 3);
    lcd.print(clock_ssid);
    WiFi.softAPmacAddress(mac);
  }                      
  else {
    sprintf(myIP, "%s", WiFi.localIP().toString().c_str());
  }
  lcd.setCursor(0, 2);
  lcd.print(myIP);

  hostName += "-";
  hostName += macToStr(mac);
  hostName.replace(":","-");
  host = (char*) hostName.c_str();
  Serial.print("host: "); Serial.println(hostName.c_str());
  otaFlag_ = otaFlag;
  if( otaFlag ) {
    lcd.setCursor(0, 3);
    lcd.print("Updating...");
    setOtaFlag(0);
    handleOTA();
  }
  else {
    MDNS.begin(host);
    Serial.print("Open http://");
    Serial.print(myIP);
    Serial.println("/ in your browser");
    server.on("/", HandleClient);
    server.on("/set_WI_FI", handleRoot);
    server.on("/ok", handleOk);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                   Adafruit_BME280::SAMPLING_X1, // temperature
                   Adafruit_BME280::SAMPLING_X1, // pressure
                   Adafruit_BME280::SAMPLING_X1, // humidity
                   Adafruit_BME280::FILTER_OFF);
  
    if (RESET_CLOCK || rtc.lostPower()) {
      // When time needs to be set on a new device, or after a power loss, the
      // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

    DateTime now = rtc.now();
    secs = now.second();
    mins = now.minute();
    hrs = now.hour();

    if (WiFi.status() == WL_CONNECTED) {
      rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
          DateTime now = rtc.now();
          secs = now.second();
          mins = now.minute();
          hrs = now.hour();
    }

    bme.takeForcedMeasurement();
    uint32_t Pressure = bme.readPressure();

    readSensors();

    delay(2500);
    lcd.clear();
    drawData();
    loadClock();
    // readSensors();
    drawSensors();

//    ftpSrv.begin("esp8266", "esp8266");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && otaFlag_){
    if(otaCount<=1) {
      Serial.println("OTA mode time out. Reset!"); 
      setOtaFlag(0);
      delay(1000);
      ESP.reset();
      delay(100);
    }
    server.handleClient();
    delay(1);
  }
  else {
    if (testTimer(sensorsTimerD, sensorsTimer)) readSensors();    // читаем показания датчиков с периодом SENS_TIME
    if (testTimer(clockTimerD, clockTimer)) clockTick();          // два раза в секунду пересчитываем время и мигаем точками
//    plotSensorsTick();                                            // тут внутри несколько таймеров для пересчёта графиков (за час, за день и прогноз)
    modesTick();                                                  // тут ловим нажатия на кнопку и переключаем режимы
    if (testTimer(drawSensorsTimerD, drawSensorsTimer)) drawSensors();  // обновляем показания датчиков на дисплее с периодом SENS_TIME

    if (WiFi.status() == WL_CONNECTED) {
//      ftpSrv.handleFTP();
      if (testTimer(RTCsyncTimerD, RTCsyncTimer)) if (timeStatus() == timeSet) rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      if (testTimer(WEBsyncTimerD, WEBsyncTimer)) server.handleClient();
      if (testTimer(MQTTsyncTimerD, MQTTsyncTimer)) {
        if (!client.connected()) {
          Serial.println("Connecting to MQTT server");
          if (client.connect(MQTT::Connect("espClient").set_auth(mqtt_auth, mqtt_pass))) Serial.println("Connected to MQTT server");
          else Serial.println("Could not connect to MQTT server");
        }   
        if (client.connected()) {
          client.loop();
          client.publish(mqtt_Temp, String (dispTemp,1));
          client.publish(mqtt_Hum, String (dispHum));
          client.publish(mqtt_Press, String (dispPres));
          client.publish(mqtt_CO2, String (dispCO2));   
          Serial.println("Publish to MQTT server");
        }
      }
    }
  }
}
