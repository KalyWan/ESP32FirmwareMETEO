

void drawData() {                     // выводим дату -------------------------------------------------------------
  int Y = 0;
  DateTime now =rtc.now();
  if (DISPLAY_TYPE == 1 && mode0scr == 1) Y = 2;
  if (!bigDig) {              // если 4-х строчные цифры, то дату, день недели (и секунды) не пишем - некуда (с)НР
    lcd.setCursor(15, 0 + Y);
    
//     if(dispExtHum != 255 && extTempBlinkTimer) { //show external temperature every 5 second //orl
//       lcd.print(String(dispExtHum, 1));
//       lcd.print("%");
//     } else {
      if (now.day() < 10) lcd.print(0);
      lcd.print(now.day());
      lcd.print("/");
      if (now.month() < 10) lcd.print(0);
      lcd.print(now.month());
//     }
//     extTempBlinkTimer = !extTempBlinkTimer;

    loadClock();              // принудительно обновляем знаки, т.к. есть жалобы на необновление знаков в днях недели (с)НР
    lcd.setCursor(18, 1);
    int dayofweek = now.dayOfTheWeek();
    lcd.print(dayNames[dayofweek]);
    // if (hrs == 0 && mins == 0 && secs <= 1) loadClock();   // Обновляем знаки, чтобы русские буквы в днях недели тоже обновились. (с)НР
    
  }
}


boolean dotFlag;

void clockTick() {
  dotFlag = !dotFlag;
  if (dotFlag) {            // каждую секунду пересчёт времени
    secs++;
    if (secs > 59) {        // каждую минуту
      secs = 0;
      mins++;
      if (mins <= 59 && mode == 0) {
        drawSensors();      // (с)НР
      }
    }
    
    if (mins > 59) {        // каждый час
      // loadClock();        // Обновляем знаки, чтобы русские буквы в днях недели тоже обновились. (с)НР

      DateTime now = rtc.now();
      secs = now.second();
      mins = now.minute();
      hrs = now.hour();
      if (mode == 0) drawSensors();
      if (hrs > 23) hrs = 0;
      if (mode == 0 && DISPLAY_TYPE) drawData();
    }
    
    if ( DISP_MODE != 0 && mode == 0 && DISPLAY_TYPE == 1 && !bigDig ) {   // Если режим секунд или дни недели по-русски, и 2-х строчные цифры то показывать секунды (с)НР
      lcd.setCursor(15, 1);
      if (secs < 10) lcd.print(" ");
      lcd.print(secs);
    } else if ( DISP_MODE == 0 && mode == 0 && BH1750_SENSOR == 1  && !bigDig )  {           // LUX oriol       
      lcd.setCursor(15, 1);
      lcd.print(showLux(lux));
    } else {
      lcd.setCursor(15, 1);              // year
//       lcd.print(now.year());
    }
  }

  if (mode == 0) {                                     // Точки и статус питания (с)НР ---------------------------------------------------
    
    if (!bigDig && powerStatus != 255 && DISPLAY_TYPE == 1) {          // отображаем статус питания (с)НР
      unsigned int batteryVolt = batteryVoltCalc();
      if (batteryVolt == 0 || (batteryVolt >= 410)) powerStatus = 0;
      else powerStatus = map (batteryVolt, 300, 410, 2, 6);      

      if (powerStatus) {
        for (byte i = 2; i <= 6; i++) {         // рисуем уровень заряда батареи (с)НР
          if ((7 - powerStatus) < i) DC[i] = 0b11111;
          else DC[i] = 0b10001;
        }
        lcd.createChar(7, DC);
      } else lcd.createChar(7, AC);

      if (mode0scr != 1) lcd.setCursor(19, 2);
      else lcd.setCursor(19, 0);
      if (!dotFlag && powerStatus == 1) lcd.write(32);
      else lcd.write(7);
    }
    
    //Serial.print("Значение: " + String(analogRead(BATTERY))); Serial.print(" Напряжение0: " + String(analogRead(BATTERY) * 5.2 / 1023.0)); Serial.print(" Напряжение1: " + String(analogRead(A1) * 5.2 / 1023.0)); Serial.print(" Статус: " + String(powerStatus));  Serial.println(" Статус2: " + String((constrain((int)analogRead(BATTERY) * 5.0 / 1023.0, 3.0, 4.2) - 3.0) / ((4.2 - 3.0) / 6.0) + 1)); //отладка (с)НР

    byte code;
    if (dotFlag) code = 165;
    else code = 32;
    if (mode0scr == 0 && (bigDig && DISPLAY_TYPE == 0 || DISPLAY_TYPE == 1)) {          // мигание большими точками только в нулевом режиме главного экрана (с)НР
      if (bigDig && DISPLAY_TYPE == 1) lcd.setCursor(7, 2);
      else lcd.setCursor(7, 0);
      lcd.write(code);
      lcd.setCursor(7, 1);
      lcd.write(code);
    }
    else {
#if (DISPLAY_TYPE == 1)
      if (code == 165) code = 58;
      lcd.setCursor(17, 3);
      lcd.write(code);
#endif
    }
  }
  
  if ((dispCO2 >= blinkLEDCO2 && LEDType == 0 || dispHum <= blinkLEDHum && LEDType == 1 || dispTemp >= blinkLEDTemp && LEDType == 2 || dispTemp < minBlinkLEDTemp && LEDType == 2 || dispRain >= maxRainBlink && LEDType == 3 ) && !dotFlag) setLEDcolor(0);     // мигание индикатора в зависимости от значения и привязанного сенсора (с)НР
  else setLED();
}
