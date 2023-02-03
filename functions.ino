void redrawAllScreen(){
  lcd.clear();
  loadClock();
  drawSensors();
  if (DISPLAY_TYPE == 1) drawData();
}



String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 3; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}


void otaCountown(){
    if(otaCount>0 && otaFlag_==1) {
      otaCount--;
      Serial.println(otaCount); 
    }
}


void readSensors() {
  int t=-1;
  bme.takeForcedMeasurement();
  dispTemp = bme.readTemperature() + TEMP_COMPENSATION;
  dispHum = bme.readHumidity();
//   dispAlt = ((float)dispAlt * 1 + bme.readAltitude(SEALEVELPRESSURE_HPA)) / 2;  // усреднение, чтобы не было резких скачков (с)НР //oriol
//   dispPres = (float)bme.readPressure() * 0.00750062;          //mmHg 
  dispPres = (float)bme.readPressure() / 100;                  //hPa
#if (CO2_SENSOR == 1)
  t = mhz19.getPPM();
  if( t != -1 ) dispCO2 = t;
#else
  dispCO2 = 0;
#endif
}


// boolean dotFlag;


boolean testTimer(unsigned long & dataTimer, unsigned long setTimer) {   // Проверка таймеров (с)НР
  if (millis() - dataTimer >= setTimer) {
    dataTimer = millis();
    return true;
  } else {
    return false;
  }
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + TIMEZONE * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}


// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


long batteryVoltCalc(){
  long voltage;
  if(analogRead(BATTERY) <= BATTERY_EMPTY) voltage = 0; 
  else if(analogRead(BATTERY) >= BATTERY_FULL) voltage = 500; 
  else voltage = map(analogRead(BATTERY), BATTERY_EMPTY, BATTERY_FULL, BATTERY_V_EMPTY, BATTERY_V_FULL);
  return voltage;
}


String Uptime(){
  
  uint32_t sec = millis() / 1000ul;
  int timeHours = (sec / 3600ul);
  int timeMins = (sec % 3600ul) / 60ul;
  int timeSecs = (sec % 3600ul) % 60ul;
  int timeDays = timeHours/24;   
  timeHours = timeHours-(timeDays*24);   
  
  String uptime;
  
  if (timeDays>0) {                        
    uptime += timeDays+" days:";
  }
  if(String(timeHours).length()==1) uptime += "0"+String(timeHours)+"h:";
  else uptime += String(timeHours)+"h:";
  if(String(timeMins).length()==1) uptime += "0"+String(timeMins)+"m:";
  else uptime += String(timeMins)+"m:";
  uptime += String(timeSecs)+"s";
  
  return uptime;
}


// void wifimanstart() { // Волшебная процедура начального подключения к Wifi.
//                       // Если не знает к чему подцепить - создает точку доступа ESP8266 и настроечную таблицу http://192.168.4.1
//                       // Подробнее: https://github.com/tzapu/WiFiManager
//   WiFiManager wifiManager;
//   wifiManager.setDebugOutput(debug);
//   wifiManager.setMinimumSignalQuality();
//   if (!wifiManager.autoConnect("ESP8266")) {
//   if (debug) Serial.println("failed to connect and hit timeout");
//     delay(3000);
//     //reset and try again, or maybe put it to deep sleep
//     ESP.reset();
//     delay(5000); }
//   if (debug) Serial.println("connected...");
// }
