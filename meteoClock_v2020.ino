/*
  Время и дата устанавливаются атвоматически при загрузке прошивки (такие как на компьютере)
  График всех величин за час и за сутки (усреднённые за каждый час)
  В модуле реального времени стоит батарейка, которая продолжает отсчёт времени после выключения/сброса питания
  Как настроить время на часах. У нас есть возможность автоматически установить время на время загрузки прошивки, поэтому:
	- Ставим настройку RESET_CLOCK на 1
  - Прошиваемся
  - Сразу ставим RESET_CLOCK 0
  - И прошиваемся ещё раз
  - Всё
*/

/* Версия 1.5
  - Добавлено управление яркостью
  - Яркость дисплея и светодиода СО2 меняется на максимальную и минимальную в зависимости от сигнала с фоторезистора
  Подключите датчик (фоторезистор) по схеме. Теперь на экране отладки справа на второй строчке появится величина сигнала с фоторезистора.
*/

/* Версия 2020
  - Портировал на ESP8266 12E                                       
  - Синхронизация времени через NTP
  - Вэб страница часов с параметрами и страницей настроек
  - Параметры сохраняются в файловую систему LittleFS
  - Обновление прошивки "по воздуху" (через вэб страницу часов, но первый раз придется прошить через COM порт)
  - Подключение к MQTT брокеру и отправка показаний температуры, давления, влажности, содержания CO2
  - После включения подымается WiFi точка 192.168.4.1 (адрес будет на LCD, CLOCK-SSID). 
    Нужно зайти на страницу и прописать параметри подключения к WiFi, часовой пояс, данные для MQTT брокера
    В дальнейшем часы будут получать адрес от вашей сети
*/

// ------------------------- НАСТРОЙКИ --------------------
#define RESET_CLOCK 0     // сброс часов на время загрузки прошивки (для модуля с несъёмной батарейкой). Не забудь поставить 0 и прошить ещё раз!
#define SENS_TIME 70000   // время обновления показаний сенсоров на экране, миллисекунд// при батарее умножается на 2 TODO
// #define LED_MODE 0        // тип RGB светодиода: 0 - главный катод, 1 - главный анод 
#define SEALEVELPRESSURE_HPA (1013.25) // Коэффициент для расчета высоты над уровнем моря

#ifndef STASSID
  #define STASSID "CLOCK-SSID"
  #define STAPSK  "0987654321"
#endif

#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"

#define EXT_SENS 2           //Presence of external station. 1-Active mode (webserver on external), 2-passive mode (webserver on internal)
                             // in mode 2 external server send data every 15(30) minutes
#define EXT_SENS_TIME 600000 // ((long)10 * 60 * 1000);    // 10 min время обновления показаний внешних сенсоров, миллисекунд // don't active on EXT_SENS 2
#define EXT_SENS_HOST "http:\/\/meteo_out/json" 

int unsigned extSensorStatus = 0;

String hostName ="WiFiClock";
char* host; //The DNS hostname
int otaCount=300; //timeout in sec for OTA mode
int otaFlag=0;
int otaFlag_=0;
#include <Ticker.h>
Ticker otaTickLoop;

// управление яркостью
byte LED_BRIGHT = 5;         // при отсутствии сохранения в EEPROM: яркость светодиода СО2 (0 - 10) (коэффициент настраиваемой яркости индикатора по умолчанию, если нет сохранения и не автоматическая регулировка (с)НР)
byte LCD_BRIGHT = 8;         // при отсутствии сохранения в EEPROM: яркость экрана (0 - 10) (коэффициент настраиваемой яркости экрана по умолчанию, если нет сохранения и не автоматическая регулировка (с)НР)
byte powerStatus = 0;         // индикатор вида питания: 255 - не показывать, остальное автоматически (0 - от сети, 1-5 уровень заряда батареи) (с)НР

#define BRIGHT_THRESHOLD 300 // величина сигнала в LUX, выше которой яркость переключится на максимум
#define LED_BRIGHT_MAX 400   // макс яркость светодиода СО2 (0 - 1023)
#define LED_BRIGHT_MIN 3     // мин яркость светодиода СО2 (0 - 1023)
#define LCD_BRIGHT_MAX 1023  // макс яркость подсветки дисплея (0 - 1023)
#define LCD_BRIGHT_MIN 5     // мин яркость подсветки дисплея (0 - 1023)

#define TEMP_COMPENSATION -2   // Компенсация температуры +/-
#define TEMP_COMPENSATION_EXT 0   // Компенсация температуры +/-

#define DISP_MODE 0       // в правом верхнем углу отображать: 0 - LUX-ы, 1 - день недели и секунды
#define SHOW_ALTITUDE 0   // показывать высотy // не работает
// #define WEEK_LANG 0       // язык дня недели: 0 - английский, 1 - русский //удалено, хотел закругленный шрифт
#define PRESSURE 0        // 0 - график давления, 1 - график прогноза дождя (вместо давления). Не забудь поправить пределы графика
#define CO2_SENSOR 1      // включить или выключить поддержку/вывод с датчика СО2 (1 вкл, 0 выкл)
#define BH1750_SENSOR 1      // включить или выключить поддержку/вывод с датчика освещения BH1750 (1 вкл, 0 выкл) 
#define DISPLAY_TYPE 1    // тип дисплея: 1 - 2004 (большой), 0 - 1602 (маленький)
#define DISPLAY_ADDR 0x27 // адрес платы дисплея: 0x27 или 0x3f. Если дисплей не работает - смени адрес! На самом дисплее адрес не указан

// пределы для индикатора (с)НР
#define normCO2 900       // и выше - желтый
#define maxCO2 1500       // и выше - красный
#define blinkLEDCO2 2000  // Значение СО2, при превышении которого будет мигать индикатор
// #define normCO2 800       // и выше - желтый
// #define maxCO2 1200       // и выше - красный
// #define blinkLEDCO2 1600  // Значение СО2, при превышении которого будет мигать индикатор

#define minBlinkLEDTemp 18 // при понижении которой будет мигать синий индикатор
#define minTemp 21        // и ниже для синего индикатора, выше зеленый
#define normTemp 26       // и выше - желтый
#define maxTemp 28        // и выше - красный
#define blinkLEDTemp 35   // Значение температуры, при превышении которой будет мигать индикатор

#define maxHum 90         // и выше - синий
#define normHum 30        // и ниже - желтый
#define minHum 20         // и ниже - красный
#define blinkLEDHum 15    // Значение влажности, при показаниях ниже порога которого будет мигать индикатор

#define normPress 900     // и ниже - желтый
#define minPress 890      // и ниже - красный   // может, переделать на синий?

#define minRain -50       // и ниже - красный   
#define normRain -20      // и ниже - желтый
#define maxRain 50        // и выше - синий
#define maxRainBlink 75   // и выше - синий

// пределы отображения для графиков
#define TEMP_MIN 18
#define TEMP_MAX 35
#define TEMP_EXT_MIN -15
#define TEMP_EXT_MAX 40
#define HUM_MIN 0
#define HUM_MAX 100
#define PRESS_MIN 850
#define PRESS_MAX 950
#define CO2_MIN 420       //set over 410 to filter out cold sensor values for web interface
#define CO2_MAX 2500
#define ALT_MIN 0
#define ALT_MAX 1000


// пины
#define BTN_PIN 12  // пин кнопки притянутый до земли резистором 10 кОм (сенсорный ТТР223 можно без резистора)

#define MHZ_RX 15
#define MHZ_TX 14

#define BACKLIGHT 2
#define LED_R 13
#define LED_G 0
#define LED_B 16

#define BATTERY A0
#define BATTERY_FULL 690      // analogRead(A0) значение батарейки при вольтаже BATTERY_V_FULL
#define BATTERY_EMPTY 124     // и BATTERY_V_EMPTY, можно посмотреть при загрузке на LCD или на веб странице => 3.20V (ЧИСЛО). 
#define BATTERY_V_FULL 430    // 4.3V*100
#define BATTERY_V_EMPTY 200   // 2V*100


byte LEDType = 0;         //  при отсутствии сохранения в EEPROM: привязка индикатора к датчикам: 0 - СО2, 1 - Влажность, 2 - Температура, 3 - Осадки

// библиотеки
#include <EEPROM.h>
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

#if (EXT_SENS == 1)
  #include <ESP8266HTTPClient.h> //to get json fron external station
#endif

#if (BH1750_SENSOR == 1)
  #include <BH1750.h>           // BH1750 (Light Sensor) Library //orl
#endif

extern "C" {
  #include "user_interface.h" //Needed for the reset command
}

#if (CO2_SENSOR == 1)
  #include <MHZ19_uart.h>
  MHZ19_uart mhz19;
#endif

#include "GyverButton.h"

int MAX_ONDATA = 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512 + 1024 + 2048; // при отсутствии сохранения в EEPROM: максимальные показания графиков исходя из накопленных фактических (но в пределах лимитов) данных вместо указанных пределов, 0 - использовать фиксированные пределы (с)НР
int VIS_ONDATA = 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512 + 1024 + 2048; // при отсутствии сохранения в EEPROM: отображение показания графиков, 0 - Не отображать (с)НР

/* 1 - для графика СО2 часового, 2 - для графика СО2 суточного (с)НР
   4 - для графика влажности часовой, 8 - для графика влажности суточной (с)НР
   16 - для графика температуры часовой, 32 - для графика температуры суточной (с)НР
   64 - для прогноза дождя часового, 128 - для прогноза дождя суточного (с)НР
   256 - для графика давления часового, 512 - для графика давления суточного (с)НР
   1024 - для графика высоты часового, 2048 - для графика высоты суточного (с)НР
   для выборочных графиков значения нужно сложить (с)НР
   например: для изменения пределов у графиков суточной температуры и суточного СО2 складываем 2 + 32 и устанавливаем значение 34 (можно ставить сумму) (с)НР
*/
const char* clock_ssid = STASSID;
const char* clock_pass = STAPSK;

// NTP Servers:
//static const char ntpServerName[] = "us.pool.ntp.org";
static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

String ssid;       //  your network SSID (name)
String pass;       // your network password
int TIMEZONE = 4; // Asia/Yerevan Time
String mqtt_ip;    // MQTT ip address
unsigned int ip_addr[]={0,0,0,0};
String mqtt_port;  // MQTT port
String mqtt_auth;  // MQTT user name
String mqtt_pass;  // MQTT user password
String mqtt_CO2;   // MQTT топик датчика CO2
String mqtt_Hum;   // MQTT топик датчика влажности
String mqtt_Temp;  // MQTT топик датчика температуры
String mqtt_Press; // MQTT топик датчика давления

WiFiClient espClient;
IPAddress mqtt_server(0, 0, 0, 0);
PubSubClient client(espClient, mqtt_server);

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();
void sendNTPpacket(IPAddress &address);

ESP8266WebServer server(80);

#if (DISPLAY_TYPE == 1)
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 20, 4);
#else
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 16, 2);
#endif

RTC_DS3231 rtc;

Adafruit_BME280 bme;

#if (BH1750_SENSOR == 1)
  BH1750 lightMeter;          // Initialize the light Sensor
#endif
  
unsigned long sensorsTimer = SENS_TIME;
unsigned long drawSensorsTimer = SENS_TIME;
unsigned long clockTimer = 500;

#if (DISPLAY_TYPE == 1)           // для дисплея 2004 график "длиннее", поэтому интервалы времени на сегмент короче (с)НР
  unsigned long hourPlotTimer = ((long)4 * 60 * 1000);        // 4 минуты
  unsigned long dayPlotTimer = ((long)1.6 * 60 * 60 * 1000);  // 1.6 часа
#else
  unsigned long hourPlotTimer = ((long)5 * 60 * 1000);        // 5 минуты
  unsigned long dayPlotTimer = ((long)2 * 60 * 60 * 1000);    // 2 часа
#endif

unsigned long predictTimer = ((long)10 * 60 * 1000);        // 10 минут
unsigned long predictTimerD = 0;

unsigned long plotTimer = hourPlotTimer;
unsigned long plotTimerD = 0;

unsigned long RTCsyncTimer = ((long)1 * 60 * 60 * 1000);    // 1 час
unsigned long RTCsyncTimerD = 0;

unsigned long WEBsyncTimer = ((long)3 * 1000);    // 3 сек
unsigned long WEBsyncTimerD = 0;

unsigned long MQTTsyncTimer = ((long)15 * 1000);    // 15 сек
unsigned long MQTTsyncTimerD = 0;

// boolean extTempBlinkTimer = false;

#if (EXT_SENS == 2)
  unsigned long dispExtSyncTime;    // 20 min
#else if (EXT_SENS == 1)
  // unsigned long extSensTimer = ((long)40 * 1000);    // 15 сек
  unsigned long extSensTimer = EXT_SENS_TIME;   //((long)10 * 60 * 1000);    // 10 min 
  unsigned long extSensTimerD = 0;
#endif

unsigned long sensorsTimerD = 0;
unsigned long drawSensorsTimerD = 0;
unsigned long clockTimerD = 0;
unsigned long hourPlotTimerD = 0;
unsigned long dayPlotTimerD = 0;

unsigned long brightTimer = (3000);                   // Таймер подсветки
unsigned long brightTimerD = 0;

uint16_t LED_ON = (LED_BRIGHT_MAX);

#if (EXT_SENS >= 1) 
  #define shiftExtClock  5
#else if
  #define shiftExtClock  0
#endif


GButton button(BTN_PIN, LOW_PULL, NORM_OPEN);

int8_t hrs, mins, secs;
byte mode = 0;
/*
  0 часы и данные - главный экран
  1 график углекислого за час
  2 график углекислого за сутки
  3 график влажности за час
  4 график влажности за сутки
  5 график температуры за час
  6 график температуры за сутки
  7 график дождя/давления за час
  8 график дождя/давления за сутки
  9 график ext температуры за час
  10 график ext температуры за сутки
  11 график ext влажности за час
  12 график ext влажности за сутки
*/

byte podMode = 1; // подрежим меню(с)НР
byte mode0scr = 0;
/* (с)НР
  0 - Крупно время
  1 - Крупно содержание СО2
  2 - Крупно температура
  3 - Крупно давление
  4 - Крупно влажность
  5 - Крупно ext температура
  6 - Крупно ext влажност
*/
boolean bigDig = false;   // true - цифры на главном экране на все 4 строки (для LCD 2004) (с)НР

// переменные для вывода
float dispTemp;
float dispExtTemp = 255;   //empty value is 255, not 0
byte dispHum;
byte dispExtHum = 255;
int dispPres;
int dispExtPres;
int dispCO2 = -1;
int dispRain;
int dispBattVolt;
int dispBattExtVolt;
int dispExtWiFi;
String dispExtUptime;

// float dispAlt;  //int
char myIP[] = "000.000.000.000";
uint16_t lux;                     // MH

// массивы графиков
int tempHour[15], tempDay[15];
int tempExtHour[15], tempExtDay[15];
//#define tempK 40                //поправочный поэффициент, чтобы показания влезли в байт
int humHour[15], humDay[15];
int humExtHour[15], humExtDay[15];
int pressHour[15], pressDay[15];
//#define pressK -600             //поправочный поэффициент, чтобы показания влезли в байт
// int rainHour[15], rainDay[15]; //no rain in plots
//#define rainK 100               //поправочный поэффициент, чтобы показания влезли в байт
int co2Hour[15], co2Day[15];
// int altHour[15], altDay[15];      // высота
int delta;
uint32_t pressure_array[6];
uint32_t sumX, sumY, sumX2, sumXY;
float a, b;
//byte time_array[6];

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
// график
byte rowS[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b10001,  0b01010,  0b00100,  0b00000};   // стрелка вниз (с)НР
byte row7[8] = {0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row6[8] = {0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};   // в т.ч. для четырехстрочных цифр 2, 3, 4, 5, 6, 8, 9, 0 (с)НР
byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 0, для четырехстрочных цифр 2, 3, 4, 5, 6, 8, 9 (с)НР
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 4 (с)НР
byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};


// цифры /AlexGyver
uint8_t LT[8] = {0b00111,  0b01111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
uint8_t UB[8] = {0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};
uint8_t RT[8] = {0b11100,  0b11110,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
uint8_t LL[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b01111,  0b00111};
uint8_t LB[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};
uint8_t LR[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11110,  0b11100};
uint8_t UMB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};
uint8_t LMB[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};


// цифры //  (с)НР
// uint8_t UB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};   // для двустрочных 7, 0   // для четырехстрочных 2, 3, 4, 5, 6, 8, 9
// uint8_t UMB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
// uint8_t LMB[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
uint8_t LM2[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};  // для двустрочной 4
uint8_t UT[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000};   // для четырехстрочных 2, 3, 4, 5, 6, 7, 8, 9, 0
// 
uint8_t KU[8] = {0b00000,  0b00000,  0b00000,  0b00001,  0b00010,  0b00100,  0b01000,  0b10000};   // для верхней части %
uint8_t KD[8] = {0b00001,  0b00010,  0b00100,  0b01000,  0b10000,  0b00000,  0b00000,  0b00000};   // для нижней части %

// индикатор питания (с)НР
// сеть
uint8_t AC[8] = {0b01010,  0b01010,  0b11111,  0b11111,  0b01110,  0b00100,  0b00100,  0b00011};
// батарея
uint8_t DC[8] = {0b01110,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};    // уровень батареи - изменяется в коде скетча (с)НР


void digSeg(byte x, byte y, byte z1, byte z2, byte z3, byte z4, byte z5, byte z6) {   // отображение двух строк по три символа с указанием кодов символов (с)НР
  lcd.setCursor(x, y);
  lcd.write(z1); lcd.write(z2); lcd.write(z3);
  if (x <= 11) lcd.print(" ");
  lcd.setCursor(x, y + 1);
  lcd.write(z4); lcd.write(z5); lcd.write(z6);
  if (x <= 11) lcd.print(" ");
}


void drawDig(byte dig, byte x, byte y) {        // рисуем цифры (с)НР ---------------------------------------
  if (bigDig && DISPLAY_TYPE == 1) {
    switch (dig) {                              // четырехстрочные цифры (с)НР
      case 0:
        digSeg(x, y, 255, 0, 255, 255, 32, 255);
        digSeg(x, y + 2, 255, 32, 255, 255, 3, 255);
        break;
      case 1:
        digSeg(x, y, 2, 255, 32, 32, 255, 32);
        digSeg(x, y + 2, 32, 255, 32, 3, 255, 3);
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
        digSeg(x, y, 0, 1, 2, 3, 4, 5);
        break;
      case 1:
        digSeg(x, y, 32, 2, 32, 32, 255, 32);
        break;
      case 2:
//         digSeg(x, y, 6, 6, 2, 255, 7, 7);
        digSeg(x, y, 6, 6, 2, 255, 4, 4);
        break;
      case 3:
//         digSeg(x, y, 6, 6, 2, 7, 7, 5);
        digSeg(x, y, 6, 6, 2, 4, 4, 5);
        break;
      case 4:
        digSeg(x, y, 3, 4, 2, 32, 32, 5);
        break;
      case 5:
//         digSeg(x, y, 255, 6, 6, 7, 7, 5);
        digSeg(x, y, 255, 6, 6, 4, 4, 5);
        break;
      case 6:
//         digSeg(x, y, 0, 6, 6, 3, 7, 5);
        digSeg(x, y, 0, 6, 6, 3, 4, 5);
        break;
      case 7:
        digSeg(x, y, 1, 1, 5, 32, 0, 32);
        break;
      case 8:
//         digSeg(x, y, 0, 6, 2, 3, 7, 5);
        digSeg(x, y, 0, 6, 2, 3, 4, 5);
        break;
      case 9:
        digSeg(x, y, 0, 6, 2, 32, 4, 5);
        break;
      case 10:
        digSeg(x, y, 32, 32, 32, 32, 32, 32);
        break;
    }
  }
}


// #if (WEEK_LANG == 0)
static const char *dayNames[]  = {
  "Su",
  "Mo",
  "Tu",
  "We",
  "Th",
  "Fr",
  "Sa",
};



void loadClock() {                                  //AlexGyver
  if (bigDig && (DISPLAY_TYPE == 1)) {              // для четырехстрочных цифр (с)НР
    lcd.createChar(0, UT);
    lcd.createChar(1, row3);
    lcd.createChar(2, UB);
    lcd.createChar(3, row5);
    lcd.createChar(4, KU);
    lcd.createChar(5, KD);
  } else {                                             // для двустрочных цифр (с)НР
    lcd.createChar(0, LT);
    lcd.createChar(1, UB);
    lcd.createChar(2, RT);
    lcd.createChar(3, LL);
    lcd.createChar(4, LB);
    lcd.createChar(5, LR);
    lcd.createChar(6, UMB);
//     lcd.createChar(7, LMB);
      
  }
}


void loadPlot() {
  lcd.createChar(0, rowS);      // Стрелка вниз для индикатора пределов (с)НР
  lcd.createChar(1, row1);
  lcd.createChar(2, row2);
  lcd.createChar(3, row3);
  lcd.createChar(4, row4);
  lcd.createChar(5, row5);
  lcd.createChar(6, row6);
  lcd.createChar(7, row7);
}


//==============================================================================
void setup() {
  EEPROM.begin(16);
//  Serial.begin(9600);
  Serial.begin(115200);

  pinMode(BACKLIGHT, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLEDcolor(3);

  analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  
  Serial.println("Mounting FS...");
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
  }

  if (EEPROM.read(0) & 0b1000) {      // если было сохранение настроек, то восстанавливаем их
    Serial.println("Restore settings..");
    lcd.print("Restore settings..");
    MAX_ONDATA = EEPROM.read(2);
    MAX_ONDATA += (long)(EEPROM.read(3) << 8);
    VIS_ONDATA = EEPROM.read(4);
    VIS_ONDATA += (long)(EEPROM.read(5) << 8);
    mode0scr = EEPROM.read(6);
    bigDig = EEPROM.read(7);
    LED_BRIGHT = EEPROM.read(8);
    LCD_BRIGHT = EEPROM.read(9);
    LEDType = EEPROM.read(10);
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

//   if( ssid.length() == 0 ) ssid = "*";
//   if( pass.length() == 0 ) pass = "*";
//   if( ssid.length() == 0 ) 
      ssid = WIFI_SSID;
//   if( pass.length() == 0 ) 
      pass = WIFI_PASS;
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

    setLEDcolor(11);
    
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
    lcd.print(F("BME280.."));
    Serial.print(F("BME280... "));
    delay(50);
    if (bme.begin(0x76,&Wire)) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERR"));
      Serial.println(F("ERROR"));
      status = false;
    }
    
  //oriol
    lcd.print(F(" BH17.."));
    Serial.print(F("BH1750... "));
    delay(50);
    
#if (BH1750_SENSOR == 1)
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERR"));
      Serial.println(F("ERROR"));
      status = false;
    }
#endif

    lcd.setCursor(0, 3);
    if (status) {
      lcd.print(F("All good"));
      Serial.println(F("All good"));
    } else {
      lcd.print(F("Check wires!"));
      Serial.println(F("Check wires!"));
    }
  
    for (byte i = 1; i < 10; i++) { //ожидание
      lcd.setCursor(11, 1);
      lcd.print("Batt:    ");
      lcd.setCursor(16, 1);
      lcd.print(analogRead(BATTERY), 1);
      Serial.println(analogRead(BATTERY));
      delay(200);
    }
  } //!otaFlag
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Connecting to WiFi"));
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  setLEDcolor(48);
  
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
    Serial.println("Not connected to WiFi ");
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
    server.on("/extsensor", handleExtSensor);
    server.on("/pageselect", handlePageSelect);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
    
    setLEDcolor(12);
      
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


    bme.takeForcedMeasurement();

#if (EXT_SENS == 1)
    readExtWeather();
#endif
    
    uint32_t Pressure = bme.readPressure();
    for (byte i = 0; i < 6; i++) {          // счётчик от 0 до 5
      pressure_array[i] = Pressure; // забить весь массив текущим давлением
      //time_array[i] = i;                    // забить массив времени числами 0 - 5
    }

//     dispAlt = (float)bme.readAltitude(SEALEVELPRESSURE_HPA);

    // заполняем графики текущим значением
    readSensors();
    
    for (byte i = 0; i < 15; i++) {   // счётчик от 0 до 14
      tempHour[i] = dispTemp;
      tempDay[i] = dispTemp;
      humHour[i] = dispHum;
      humDay[i] = dispHum;
      if(dispExtTemp!=255){
        tempExtHour[i] = dispExtTemp;
        tempExtDay[i] = dispExtTemp;
        humExtHour[i] = dispExtHum;
        humExtDay[i] = dispExtHum;
      }
      //    rainHour[i] = 0;
      //    rainDay[i] = 0;
      if (PRESSURE) {
        pressHour[i] = 0;
        pressDay[i] = 0;
      } else {
        pressHour[i] = dispPres;
        pressDay[i] = dispPres;
      }
    }
    
    
    delay(1500);
    lcd.clear();
    if (DISPLAY_TYPE == 1) drawData();
    loadClock();
    // readSensors();
    drawSensors();
  }
}




//==============================================================================
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
    if (testTimer(brightTimerD, brightTimer)) checkBrightness();  // яркость
    if (testTimer(sensorsTimerD, sensorsTimer)) readSensors();    // читаем показания датчиков с периодом SENS_TIME
    if (testTimer(clockTimerD, clockTimer)) clockTick();          // два раза в секунду пересчитываем время и мигаем точками
    plotSensorsTick();                                            // тут внутри несколько таймеров для пересчёта графиков (за час, за день и прогноз)
    modesTick();                                                  // тут ловим нажатия на кнопку и переключаем режимы
    if (mode == 0) {  // в режиме "главного экрана"
      if (testTimer(drawSensorsTimerD, drawSensorsTimer)) drawSensors();  // обновляем показания датчиков на дисплее с периодом SENS_TIME
    } else {                                               // в любом из графиков
      if (testTimer(plotTimerD, plotTimer)) redrawPlot();  // перерисовываем график
    } 
    if (WiFi.status() == WL_CONNECTED) {
      
      if (testTimer(RTCsyncTimerD, RTCsyncTimer)) if (timeStatus() == timeSet) rtc.adjust(DateTime(year(), month(), day(), hour(), minute(), second()));
      
      if (testTimer(WEBsyncTimerD, WEBsyncTimer)) server.handleClient();
      
#if (EXT_SENS == 1)
      if (testTimer(extSensTimerD, extSensTimer)) readExtWeather(); 
#else if(EXT_SENS == 2)
      if ( dispExtSyncTime < millis() ) {
        dispExtTemp = 255;
        dispExtHum = 255;
      } 
#endif
//       if (testTimer(MQTTsyncTimerD, MQTTsyncTimer)) {                          // удаление за ненадобностю
//         if (!client.connected()) {
//           Serial.println("Connecting to MQTT server");
//           if (client.connect(MQTT::Connect("espClient").set_auth(mqtt_auth, mqtt_pass))) Serial.println("Connected to MQTT server");
//           else Serial.println("Could not connect to MQTT server");
//         }   
//         if (client.connected()) {
//           client.loop();
//           client.publish(mqtt_Temp, String (dispTemp,1));
//           client.publish(mqtt_Hum, String (dispHum));
//           client.publish(mqtt_Press, String (dispPres));
//           client.publish(mqtt_CO2, String (dispCO2));   
//           Serial.println("Publish to MQTT server");
//         }
//       }
    }
  }yield(); // что за команда - фиг знает, но ESP работает с ней стабильно и не глючит.
}
