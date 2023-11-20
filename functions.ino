String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 3; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void WiFi_Start() {
  delay(10);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
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

void otaCountown() {
  if (otaCount > 0 && otaFlag_ == 1) {
    otaCount--;
    Serial.println(otaCount);
  }
}

void modesTick() {
  button.tick();
  boolean changeFlag = false;

  if (button.isSingle()) {                    // одинарное нажатие на кнопку
    mode0scr++;
    if (CO2_SENSOR == 0 && mode0scr == 4) mode0scr++;
    if (mode0scr > 4) mode0scr = 0;
    changeFlag = true;
  }

  if (button.isHolded()) {                    // удержание кнопки 
    bigDig = !bigDig;
    changeFlag = true;
  }

  if (changeFlag) {
    lcd.clear();
    loadClock();
    drawSensors();
    drawData();
  }
}

void readSensors() {
  int t = -1;
  bme.takeForcedMeasurement();
  dispTemp = bme.readTemperature() + TempCorrection;
  dispHum = bme.readHumidity() + HumCorrection;
  dispPres = (float)bme.readPressure() * 0.00750062;
#if (CO2_SENSOR == 1)
  t = mhz19.getPPM();
  if ( t != -1 ) dispCO2 = t;
#else
  dispCO2 = 0;
#endif
}

void drawSensors() {

  if (mode0scr != 0 && !bigDig) {                      // время  ----------------------------
    lcd.setCursor(0, 3);
    if (hrs / 10 == 0) lcd.print(" ");
    lcd.print(hrs);
    lcd.print(":");
    if (mins / 10 == 0) lcd.print("0");
    lcd.print(mins);
    lcd.print(":");
  } else if (mode0scr == 0) {
      drawClock(hrs, mins, 0, 0);
  }

  if (mode0scr != 1) {                                  // Температура  ----------------------------
    lcd.setCursor(15, 1);
    lcd.print(String(dispTemp, 1));
    lcd.write(223);
  } else {
    drawTemp(dispTemp, 0, 0);
  }

  if (mode0scr != 2) {                                  // Влажность  ----------------------------
    lcd.setCursor(15, 2);
    lcd.print(String(dispHum) + "%");
  } else {
    drawHum(dispHum, 0, 0);
  }

  if (mode0scr != 3) {                                  // Давление  ---------------------------
    lcd.setCursor(15, 3);
    lcd.print(String(dispPres) + "mm");
  } else {
    drawPres(dispPres, 0, 0);
  }

#if (CO2_SENSOR == 1)
  if (mode0scr != 4 && !bigDig) {                       // СО2  ----------------------------
      lcd.setCursor(0, 2);
      lcd.print(String(dispCO2) + "ppm");
  } else if (mode0scr == 4){
      drawPPM(dispCO2, 0, 0);
  }
#endif
}

boolean dotFlag;

void clockTick() {
  dotFlag = !dotFlag;
  if (dotFlag) {            // каждую секунду пересчёт времени
    secs++;
    if (secs > 59) {        // каждую минуту
      secs = 0;
      mins++;
      if (mins <= 59) drawSensors();
    }
    if (mins > 59) {                           // каждый час
      // loadClock();                          // Обновляем знаки, чтобы русские буквы в днях недели тоже обновились. 

      DateTime now = rtc.now();
      secs = now.second();
      mins = now.minute();
      hrs = now.hour();
      drawSensors();
      if (hrs > 23) hrs = 0;
      drawData();
    }
    if (mode0scr == 0) {                       // Если режим часов, то показывать секунды в правом верхмем углу
      lcd.setCursor(16, 0);
      if (secs < 10) lcd.print("0");
      lcd.print(secs);
    }
    if (mode0scr != 0 && !bigDig) {            // Показывать секунды внизу
      lcd.setCursor(6, 3);
      if (secs < 10) lcd.print("0");
      lcd.print(secs);
    }
  }

  // Точки  ---------------------------------------------------
  byte code;
  if (dotFlag) code = 165;
  else code = 32;
  if (mode0scr == 0) {                        // Мигание точками только в нулевом режиме
    if (bigDig) lcd.setCursor(7, 2);
    else lcd.setCursor(7, 0);
    lcd.write(code);
    lcd.setCursor(7, 1);
    lcd.write(code);
    if (code == 165) code = 58;
    lcd.setCursor(15, 0);
    lcd.write(code);
  }
    if (mode0scr != 0 && !bigDig) {           // Мигание точками во всех режимах внизу
      if (code == 165) code = 58;
      lcd.setCursor(2, 3);
      lcd.write(code);
      lcd.setCursor(5, 3);
      lcd.write(code);
    }
}

boolean testTimer(unsigned long & dataTimer, unsigned long setTimer) {   // Проверка таймеров 
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
