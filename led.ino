void checkBrightness() {
#if (BH1750_SENSOR == 1)
  lux = lightMeter.readLightLevel();
#endif
  
  if (LCD_BRIGHT == 11) {                         // если установлен автоматический режим для экрана (с)НР
    if (lux >= BRIGHT_THRESHOLD) {                 // если светло
      analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);
    } else {                                      // если темно - adaptiv
      analogWrite(BACKLIGHT, map(lux, 0, BRIGHT_THRESHOLD, LCD_BRIGHT_MIN, LCD_BRIGHT_MAX));
    }
  } else {
    analogWrite(BACKLIGHT, LCD_BRIGHT * LCD_BRIGHT * 10.22); //2.5 for 256 levels
  }

  if (LED_BRIGHT == 11) {                         // если установлен автоматический режим для индикатора (с)НР
    if (lux >= BRIGHT_THRESHOLD) {                 // если светло
      LED_ON = (LED_BRIGHT_MAX);
    } else {                                      // если темно - adaptiv
      LED_ON = (map (lux, 0, BRIGHT_THRESHOLD, LED_BRIGHT_MIN, LED_BRIGHT_MAX));
//       if(LED_ON > 1023) LED_ON = LED_BRIGHT_MIN;  //maping error
    }
  } else {
    LED_ON = ( LED_BRIGHT * LED_BRIGHT * 10.22 ); //2.5 for 256 levels
  }
}


void setLEDcolor(byte color) {                    // цвет индикатора задается двумя битами на каждый цвет (с)НР
  
  byte LED_OFF = (0);
  
  if(color == 0){                    // off
    analogWrite(LED_R, LED_OFF);
    analogWrite(LED_G, LED_OFF);
    analogWrite(LED_B, LED_OFF);
  }
  if(color == 3){                    // red
    analogWrite(LED_R, LED_ON);
    analogWrite(LED_G, LED_OFF);
    analogWrite(LED_B, LED_OFF);
  }
  if(color == 12){                   // green
    analogWrite(LED_R, LED_OFF);
    analogWrite(LED_G, LED_ON);
    analogWrite(LED_B, LED_OFF);
  }
  if(color == 11){                   // yellow      //can be canfigured brightness of eash LED
//     analogWrite(LED_R, (LED_ON * .9)); 
    analogWrite(LED_R, (LED_ON*0.8)); 
    analogWrite(LED_G, (LED_ON));
    analogWrite(LED_B, LED_OFF);
  }
  if(color == 48){                    // blue
    analogWrite(LED_R, LED_OFF);
    analogWrite(LED_G, LED_OFF);
    analogWrite(LED_B, LED_ON);
  }
}


void setLED() {

  // ниже задается цвет индикатора в зависимости от назначенного сенсора: красный, желтый, зеленый, синий (с)НР
  if ((dispCO2 >= maxCO2) && LEDType == 0 || (dispHum <= minHum) && LEDType == 1 || (dispTemp >= maxTemp) && LEDType == 2 || (dispRain <= minRain) && LEDType == 3 || (dispPres <= minPress) && LEDType == 4) setLEDcolor(3);   // красный
  else if ((dispCO2 >= normCO2) && LEDType == 0 || (dispHum <= normHum) && LEDType == 1 || (dispTemp >= normTemp) && LEDType == 2 || (dispRain <= normRain) && LEDType == 3 || (dispPres <= normPress) && LEDType == 4) setLEDcolor(3 + 8);   // желтый
  else if (LEDType == 0 || (dispHum <= maxHum) && LEDType == 1 || (dispTemp >= minTemp) && LEDType == 2 || (dispRain <= maxRain) && LEDType == 3 || LEDType == 4) setLEDcolor(12);    // зеленый
  
  else setLEDcolor(48);   // синий (если влажность превышает заданный максимум, температура ниже минимума, вероятность осадков выше maxRain)
  
}


String showLux(uint16_t value) {           //MH convert LUX to string
  String luxvalue = String(value);
  switch (luxvalue.length()) {
    case 1:
      luxvalue = " " + luxvalue + "Lux";
    break;
    case 2:
      luxvalue = luxvalue + "Lux";
    break;
    case 3:
      luxvalue = luxvalue + "Lx";
    break;
    case 4:
      luxvalue = luxvalue + "L";
    break;
    case 5:
    break;
    case 6:
    case 7:
      luxvalue = " over";
    break;
  }
  return(luxvalue);
}
