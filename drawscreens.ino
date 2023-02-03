// mode0scr
// 0 - Часы
// 1 - CO2
// 2 - Температура
// 3 - Давление
// 4 - Влажность
// 5 - Температура Внешная
// 6 - Влажность Внешная


void drawSensors() {
#if (DISPLAY_TYPE == 1)
  // дисплей 2004 ----------------------------------

  
  if (mode0scr != 2) {                        // Температура (с)НР ----------------------------
    if (bigDig) {
      if (mode0scr == 1) lcd.setCursor(15, 2);
      if (mode0scr != 1) lcd.setCursor(15, 0);
    } 
    else lcd.setCursor(0, 2);
    
    lcd.print(String(dispTemp, 1));
    lcd.write(223);
  } else {
    drawTemp(dispTemp, 0, 0);
  }

  
  if (mode0scr != 4) {                        // Влажность (с)НР ----------------------------
    lcd.setCursor(5, 2);
    if (bigDig) lcd.setCursor(15, 1);
    lcd.print(" " + String(dispHum) + "% ");
  } else {
    drawHum(dispHum, 0, 0);
  }

  
#if (BH1750_SENSOR == 1 && DISP_MODE == 0)
  if (!bigDig) {                        // Освещение (с)MH ----------------------------
    lcd.setCursor(15, 1);
    lcd.print(showLux(lux));
  }
#endif


#if (CO2_SENSOR == 1)
  if (mode0scr != 1) {                        // СО2 (с)НР ----------------------------
    if (bigDig) {
      lcd.setCursor(15, 2);
      lcd.print(String(dispCO2) + "p");
    } else {
      lcd.setCursor(11, 2);
      lcd.print(String(dispCO2) + "ppm ");
    }
  } else {
    drawPPM(dispCO2, 0, 0);
  }
#endif


  if (mode0scr != 3) {                      // Давление (с)НР ---------------------------
    lcd.setCursor((shiftExtClock + 0), 3);
    if ( bigDig ) {
      switch ( mode0scr ) {
        case 0:
          lcd.setCursor(15, 3);
        break;
        case 1:
        case 2:
           lcd.setCursor(15, 0);
        break;
        case 4:
          lcd.setCursor(15, 1);
        break;
        lcd.setCursor(15, 1);
      }
    }
#if (EXT_SENS >= 1) 
    lcd.print(String(dispPres) + "P");
#else 
    lcd.print(String(dispPres) + "hPa");
#endif
  } else {
    drawPres(dispPres, 0, 0);
  }

  
#if (EXT_SENS >= 1) 
  if (mode0scr != 5) {                        // Внешная Температура (с)MH -------------------
    if(!bigDig) {
      lcd.setCursor(0, 3);
      if( dispExtTemp != 255 ) {
        lcd.print(String(dispExtTemp, 1));
        lcd.write(223);
      } else {
        lcd.print(String("--.-"));
        lcd.write(223);
      }
    }
  } else {
    drawTemp(dispExtTemp, 0, 0);
  }
  
  
  if (mode0scr == 6) {                      // Внешная Влажность -------------------------
      drawExtHum(dispExtHum, 0, 0);            // мелко не выводим 
  }
#endif


  if (!bigDig) {                            // дождь (с)НР -----------------------------
#if (EXT_SENS >= 1) 
    lcd.setCursor((shiftExtClock + 5), 3); //narrow place, use 5 character
  
    switch ( String(dispRain).length() ){
      case 1:
        lcd.write(0B11000010);
        lcd.print(String(dispRain) + "%  ");
        break;
      case 2:
        lcd.write(0B11000010);
        lcd.print(String(dispRain) + "% ");
        break;
      case 3:
        lcd.print(String(dispRain) + "%");
        break;
      case 4:
        lcd.print(String(dispRain));
        break;
    }
#else                                     //no external clock, lot of space
    lcd.setCursor(9, 3);
    lcd.write(0B11000010);
    lcd.print(String(dispRain) + "%  ");
#endif 
  }
  
  
  if (mode0scr != 0) {                      // время (с)НР ----------------------------
    lcd.setCursor(15, 3);
    if (hrs / 10 == 0) lcd.print(" ");
    lcd.print(hrs);
    lcd.print(":");
    if (mins / 10 == 0) lcd.print("0");
    lcd.print(mins);
  } else {
    drawClock(hrs, mins, 0, 0); //, 1);
  }
  
  
#else


  // дисплей 1602 ----------------------------------
  if (!bigDig) {              // если только мелкими цифрами (с)НР
    lcd.setCursor(0, 0);
    lcd.print(String(dispTemp, 1));
    lcd.write(223);
    lcd.setCursor(6, 0);
    lcd.print(String(dispHum) + "% ");

#if (CO2_SENSOR == 1)
    lcd.print(String(dispCO2) + "ppm");
    if (dispCO2 < 1000) lcd.print(" ");
#endif

    lcd.setCursor(0, 1);
    lcd.print(String(dispPres) + "hP ");
    lcd.write(0B11000010);
    lcd.print(String(dispRain) + "% ");
  } else {                    // для крупных цифр (с)НР
    switch (mode0scr) {
      case 0:
        drawClock(hrs, mins, 0, 0);
        break;
      case 1:
#if (CO2_SENSOR == 1)
        drawPPM(dispCO2, 0, 0);
#endif
        break;
      case 2:
        drawTemp(dispTemp, 2, 0);
        break;
      case 3:
        drawPres(dispPres, 2, 0);
        break;
      case 4:
        drawHum(dispHum, 0, 0);
        break;
#if (EXT_SENS >= 1)
      case 5:
        drawTemp(dispExtTemp, 0, 0);
        break;
      case 6:
        drawHum(dispExtHum, 0, 0);
        break;
#endif
    }
  }
#endif

//   drawData();               //update date too. TODO

}





void drawPPM(int dispCO2, byte x, byte y) {     // Уровень СО2 крупно на главном экране (с)НР ----------------------------
  if (dispCO2 / 1000 == 0) drawDig(10, x, y);
  else drawDig(dispCO2 / 1000, x, y);
  drawDig((dispCO2 % 1000) / 100, x + 4, y);
  drawDig((dispCO2 % 100) / 10, x + 8, y);
  drawDig(dispCO2 % 10 , x + 12, y);
  lcd.setCursor(15, 0);
#if (DISPLAY_TYPE == 1)
  lcd.print("ppm");
#else
  lcd.print("p");
#endif
}


void drawPres(int dispPres, byte x, byte y) {   // Давление крупно на главном экране (с)НР ----------------------------
  drawDig((dispPres % 1000) / 100, x , y);
  drawDig((dispPres % 100) / 10, x + 4, y);
  drawDig(dispPres % 10 , x + 8, y);
  lcd.setCursor(x + 11, 1);
  if (bigDig) lcd.setCursor(x + 11, 3);
  lcd.print("hPa");
}


// void drawAlt(float dispAlt, byte x, byte y) {   // Высота крупно на главном экране (с)НР -----------------------------
//   if (dispAlt >= 1000) {
//     drawDig((int(dispAlt) % 10000) / 1000, x , y);
//     x += 4;
//   }
//   drawDig((int(dispAlt) % 1000) / 100, x , y);
//   drawDig((int(dispAlt) % 100) / 10, x + 4, y);
//   drawDig(int(dispAlt) % 10 , x + 8, y);
//   if (dispAlt < 1000) {       // десятые доли метра, если высота ниже 1000 м. (с)НР
//     //   drawDig((int(dispAlt * 10.0)) % 10 , x + 12, y);         // десятые крупными цифрами (тогда буква m наезжает на последнюю цифру)
//     lcd.setCursor(x + 12, y + 1 + (bigDig && DISPLAY_TYPE) * 2);  // десятые мелкими цифрами
//     lcd.print((int(dispAlt * 10.0)) % 10);
//     if (bigDig && DISPLAY_TYPE == 1) lcd.setCursor(x + 11, y + 3);
//     else lcd.setCursor(x + 11, y + 1);
//     lcd.print(".");
//     x -= 1; // сдвинуть букву m левее
//   }  else {
//     x -= 4;
//   }
//   if (bigDig && DISPLAY_TYPE == 1) lcd.setCursor(x + 14, 3);
//   else lcd.setCursor(x + 14, 1);
//   lcd.print("m");
// }


void drawTemp(float dispTemp, byte x, byte y) { // Температура крупно на главном экране (с)НР ----------------------------
  if( mode0scr>4 && dispExtTemp == 255) {  // error from external server
    lcd.setCursor(x, y);
    lcd.print("  -NO SENSOR-");
  } else {
    if (dispTemp / 10 == 0) drawDig(10, x, y);
    else drawDig(dispTemp / 10, x, y);
    drawDig(int(dispTemp) % 10, x + 4, y);
    drawDig(int(dispTemp * 10.0) % 10, x + 8, y);
  
    if (bigDig && DISPLAY_TYPE == 1) {
      lcd.setCursor(x + 7, y + 3);
      lcd.write(1);             // десятичная точка
    
      if(mode0scr>4){
        lcd.setCursor(x + 11, y + 3);
        lcd.print("ext");    // external
      }
    } else {
      lcd.setCursor(x + 7, y + 1);
      lcd.write(0B10100001);    // десятичная точка

      if(mode0scr>4){
        lcd.setCursor(x + 11, y + 1);
        lcd.print("ext");    // external
      }
    }
    lcd.setCursor(x + 12, y);
    lcd.write(223);             // градусы
  }
}


void drawHum(int dispHum, byte x, byte y) {   // Влажность крупно на главном экране (с)НР ----------------------------
  if (dispHum / 100 == 0) drawDig(10, x, y);
  else drawDig(dispHum / 100, x, y);
  if ((dispHum % 100) / 10 == 0) drawDig(0, x + 4, y);
  else drawDig(dispHum / 10, x + 4, y);
  drawDig(int(dispHum) % 10, x + 8, y);
  
  if (bigDig && DISPLAY_TYPE == 1) {
    lcd.setCursor(x + 12, y + 1);
    lcd.print("\245\4");
    lcd.setCursor(x + 12, y + 2);
    lcd.print("\5\245");
    
  } else {
    lcd.setCursor(x + 12, y + 1);
    lcd.print("%");
  }
}


void drawExtHum(int dispHum, byte x, byte y) {   // Влажность крупно на главном экране (с)НР ----------------------------
  if( mode0scr > 4 && dispExtHum == 255 ) {  // error from external server
    lcd.setCursor(x, y);
    lcd.print("  -NO SENSOR-");
  } else {
    if (!bigDig && DISPLAY_TYPE == 1) {
      lcd.setCursor(x + 0, y + 0);
      lcd.print("Hum:");
      lcd.print(dispExtHum);
      lcd.print("%");
      lcd.print("     ");
      lcd.setCursor(x + 0, y + 1);
      lcd.write(7);
//       lcd.print(":");
      lcd.print((float)dispBattExtVolt/100);
      lcd.print("V WF");
      lcd.print(dispExtWiFi); 
//       lcd.print("dBm");
    } else {
      lcd.setCursor(x + 1, y + 0);
      lcd.print("Temp:");
      lcd.print(String(dispExtTemp, 1));
      lcd.write(223);    // десятичная точка 
      lcd.print("    ");
      lcd.setCursor(x + 1, y + 1);
      lcd.print(" Hum:");
      lcd.print(dispExtHum);
      lcd.print("%");
      lcd.setCursor(x + 1, y + 2);
      lcd.print("   ");
      lcd.write(7);
      lcd.print(":");
      lcd.print((float)dispBattExtVolt/100);
      lcd.print("V");
      lcd.setCursor(x + 1, y + 3);
      lcd.print("WiFi:");
      lcd.print(dispExtWiFi); 
      lcd.print("dBm");
    }
  }
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
