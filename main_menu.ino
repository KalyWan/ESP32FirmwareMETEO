
/*
  mode:
  0 - Главный экран
  1-8(12) - Графики: СО2 (час, день), Влажность (час, день), Температура (час, день), Осадки (час, день)
  252 - выбор режима индикатора (podMode от 0 до 3: индикация СО2, влажности, температуры, осадков)
  253 - настройка яркости экрана (podMode от 0 до 11: выбор от 0% до 100% или автоматическое регулирование)
  254 - настройка яркости индикатора (podMode от 0 до 11: выбор от 0% до 100% или автоматическое регулирование)
  255 - главное меню (podMode от 0 до 13: 1 - Сохранить, 2 - Выход, 3 - Ярк.индикатора, 4 - Ярк.экрана, 5 - Режим индикатора,
                                        6-13 вкл/выкл графики: СО2 (час, день), Влажность (час, день), Температура (час, день), Осадки (час, день))
*/
void modesTick() {
  button.tick();
  boolean changeFlag = false;
  
  if (button.isSingle()) {    // одинарное нажатие на кнопку

    if (mode >= 240) {
      podMode++;
      switch (mode) {
        case 252:             // Перебираем все варианты режимов LED индикатора (с)НР
          //         podMode++;
          if (podMode > 4) podMode = 0;
          LEDType = podMode;
          changeFlag = true;
          break;

        case 253:             // Перебираем все варианты яркости LCD экрана (с)НР
          //         podMode++;
          if (podMode > 11) podMode = 0;
          LCD_BRIGHT = podMode;
          checkBrightness();
          changeFlag = true;
          break;

        case 254:             // Перебираем все варианты яркости LED индикатора (с)НР
          //         podMode++;
          if (podMode > 11) podMode = 0;
          LED_BRIGHT = podMode;
          changeFlag = true;
          break;

        case 255:             // Перебираем все варианты основных настроек (с)НР
          //         podMode++;
          if (podMode > 15) podMode = 1;
          changeFlag = true;
          break;
      }
      
      
    } 
    else if(mode==0) {      // Переключение рехима работы главного экрана ОДНИМ нажатием (с)MH
      

// mode0scr
// 0 - Часы
// 1 - CO2
// 2 - Температура
// 3 - Давление
// 4 - Влажность
// 5 - Температура Внешная
// 6 - Влажность Внешная

      
      mode0scr++;
      if (CO2_SENSOR == 0 && mode0scr == 1) mode0scr++;     
//       if (SHOW_ALTITUDE  == 0 && mode0scr == 5) mode0scr = 0;  
#if (EXT_SENS >= 1)
      if (mode0scr > 6) mode0scr = 0;         
#else
      if (mode0scr > 4) mode0scr = 0;         //
#endif

      changeFlag = true;      //??????
    } 
    
    else {
//       do {
                      
        mode++;
        
#if (EXT_SENS >= 1)
        if (mode > 12) mode = 1;
#else
        if (mode > 8) mode = 1;
#endif
      
        if (CO2_SENSOR == 0 && mode == 1){
          mode = 3;
        }      
        
//       } while (((VIS_ONDATA & (1 << (mode - 1))) == 0) && (mode > 0));   // проверка на отображение графиков (с)НР
      changeFlag = true;
    }
  }
  
  
  if (button.isDouble()) {                    // двойное нажатие (с)НР ----------------------------
  
    if (mode > 0 && mode < 13) {              // Меняет пределы графика на установленные/фактические максимумы (с)НР
      MAX_ONDATA = (int)MAX_ONDATA ^ (1 << (mode - 1));
    } 
    else if (mode == 0)  mode = 1;           //вход в графики DABLE кликом
                           
      
    else if (mode > 240) podMode = 1;       // Переключение на меню сохранения (с)НР
    changeFlag = true;
  }

  
  if ((button.isTriple()) && (mode == 0)) {  // тройное нажатие в режиме главного экрана - переход в меню (с)НР
    mode = 255;
    podMode = 3;
    changeFlag = true;
  }

  
  if (button.isHolded()) {    // удержание кнопки (с)НР
    //    if ((mode >=252) && (mode <= 254)) {
    //      mode = 255;
    //      podMode = 1;
    //    }
    switch (mode) {
      case 0:
        bigDig = !bigDig;
        break;
      case 252:       // реж. индикатора
        mode = 255;
        podMode = 1;
        break;
      case 253:       // ярк. экрана
        mode = 255;
        podMode = 1;
        break;
      case 254:       // ярк. индикатора
        mode = 255;
        podMode = 1;
        break;
      case 255:       // главное меню
        if (podMode == 2 || podMode == 1) mode = 0;                   // если Выход или Сохранить
        if (podMode >= 3 && podMode <= 5) mode = 255 - podMode + 2;   // если настройки яркостей, то переключаемся в настройки пункта меню
        if (podMode >= 6 && podMode <= 17) VIS_ONDATA = VIS_ONDATA ^ (1 << (podMode - 6));  // вкл/выкл отображения графиков
        if (podMode == 1) {                                           // если Сохранить
          if (EEPROM.read(2) != (MAX_ONDATA & 255)) EEPROM.write(2, (MAX_ONDATA & 255));
          if (EEPROM.read(3) != (MAX_ONDATA >> 8)) EEPROM.write(3, (MAX_ONDATA >> 8));
          if (EEPROM.read(4) != (VIS_ONDATA & 255)) EEPROM.write(4, (VIS_ONDATA & 255));
          if (EEPROM.read(5) != (VIS_ONDATA >> 8)) EEPROM.write(5, (VIS_ONDATA >> 8));
          if (EEPROM.read(6) != mode0scr) EEPROM.write(6, mode0scr);
          if (EEPROM.read(7) != bigDig) EEPROM.write(7, bigDig);
          if (EEPROM.read(8) != LED_BRIGHT) EEPROM.write(8, LED_BRIGHT);
          if (EEPROM.read(9) != LCD_BRIGHT) EEPROM.write(9, LCD_BRIGHT);
          if (EEPROM.read(10) != LEDType) EEPROM.write(10, LEDType);
          byte t = EEPROM.read(0);
          EEPROM.write(0, 0b1000 | t);
          EEPROM.commit();
        }
        if (podMode < 6) podMode = 1;
        if (mode == 252) podMode = LEDType;     // если выбран режим LED - устанавливаем текущее значение (с)НР
        if (mode == 254) podMode = LED_BRIGHT;  // если выбрана яркость LED - устанавливаем текущее показание (с)НР
        if (mode == 253) podMode = LCD_BRIGHT;  // если выбрана яркость LCD - устанавливаем текущее показание (с)НР
        break;
      default:
        mode = 0;
    }
    changeFlag = true;
  }

  if (changeFlag) {
    if (mode >= 240) {
      lcd.clear();
      lcd.setCursor(0, 0);
    }
    if (mode == 255) {          // Перебираем варианты в главном меню (с)НР

      lcd.print("Setup:");         // ---Настройки
      lcd.setCursor(0, 1);
      switch (podMode) {
        case 1:
          lcd.print("Save");
          break;
        case 2:
          lcd.print("Exit");
          break;
        case 5:
          lcd.print("indicator mode");
          break;
        case 3:
          lcd.print("indicator brt.");
          break;
        case 4:
          lcd.print("Bright LCD");
          break;
      }
      if (podMode >= 6 && podMode <= 17) {
        lcd.setCursor(10, 0);
        lcd.print("Charts  ");
        lcd.setCursor(0, 1);
        if ((3 & (1 << (podMode - 6))) != 0) lcd.print("CO2 ");
        if ((12 & (1 << (podMode - 6))) != 0) {
          lcd.print("Hum,%");
        }
        if ((48 & (1 << (podMode - 6))) != 0) lcd.print("t\337 ");
        if ((192 & (1 << (podMode - 6))) != 0) {
          if (PRESSURE) lcd.print("p, rain ");
          else lcd.print("p,hPa ");
        }
        if ((768 & (1 << (podMode - 6))) != 0) {
          lcd.print("hgt,m  ");
        }

        if ((1365 & (1 << (podMode - 6))) != 0) {
          lcd.setCursor(8, 1);
          lcd.print("Hour:");
        } else {
          lcd.setCursor(7, 1);
          lcd.print("Day: ");
        }
        if ((VIS_ONDATA & (1 << (podMode - 6))) != 0) {
          lcd.print("On  ");
        }
        else {
          lcd.print("Off ");
        }
      }
    }
    if (mode == 252) {                        // --------------------- показать  "Реж.индикатора"
      LEDType = podMode;
      lcd.setCursor(0, 0);
      lcd.print("indicator mode:");
      lcd.setCursor(0, 1);
      switch (podMode) {
        case 0:
          lcd.print("CO2   ");
          break;
        case 1:
          lcd.print("Humid.");
          break;
        case 2:
          lcd.print("t\337     ");
          break;
        case 3:
          lcd.print("rain  ");
          break;
        case 4:
          lcd.print("pressure");
          break;
      }

    }
    if (mode == 253) {                        // --------------------- показать  "Ярк.экрана"
      lcd.print("Bright LCD:");
      //lcd.setCursor(11, 0);
      if (LCD_BRIGHT == 11) {
        lcd.print("Auto ");
      }
      else lcd.print(String(LCD_BRIGHT * 10) + "%");
    }
    if (mode == 254) {                        // --------------------- показать  "Ярк.индикатора"
      lcd.print("indic.brt.:");
      //lcd.setCursor(15, 0);
      if (LED_BRIGHT == 11) {
        lcd.print("Auto ");
      }
      else lcd.print(String(LED_BRIGHT * 10) + "%");
    }

    if (mode == 0) {
      redrawAllScreen();
    } else if (mode <= 12) {
      //lcd.clear();
      loadPlot();
      redrawPlot();
    }
  }
}
