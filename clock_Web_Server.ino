const char* otaServerIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";


String printDigits(int digits)
{
  String f=String(digits);
  if (digits < 10) return ("0"+f); else return f;
}

  
 String genPlot(int *plot_input){
  int max_value = -32000;
  int min_value = 32000;
  String data;
//   String circleData;
//   int point;
  
  for (byte i = 0; i < 15; i++) {
    max_value = max(plot_input[i] , max_value);
    min_value = min(plot_input[i] , min_value);
  }
  if (min_value >= max_value) max_value = min_value + 1;

  data += R"rawliteral( 
  <div class="float">
  <svg viewBox="0 0 460 140" class="chart">
  <line x1="1" x2="500" y1="125" y2="125"  style="stroke:#ccc;stroke-width:1"></line>
  <line x1="35" x2="35" y1="1" y2="140" style="stroke:#ccc;stroke-width:1" ></line>
  )rawliteral";
  data += "<g class=\"labels x-labels\"><text x=\"1\" y=\"10\">" + String(max_value) + "</text>\n<text x=\"1\" y=\"123\">" + String(min_value) + "</text></g>\n";
  
  data += "<polyline fill=\"none\" stroke=\"#0074d9\" stroke-width=\"3\" points=\"";
  
  for (byte i = 0; i < 15; i++) {
    data += " " + String(i*31+35) + "," + String(125-map(plot_input[i], min_value, max_value, 0, 120)) + " ";
//     circleData += "<circle cx=\"" + String(i*31+35) + "\" cy=\"" + String(125-map(plot_input[i], min_value, max_value, 0, 120)) + "\" data-value=\"" + String(plot_input[i]) + "\" r=\"4\"><title>" + String(plot_input[i]) + "</title></circle>\n";
  }
  data += "\"/>\n";
//   data += "<g class=\"plotData\"> \n" + circleData + " </g>\n";
  data += "\"/>\n</svg></div>\n";
  return data;
}


// function to calculete Humidex
float calculate_humidex(float temperature, float humidity) {
  float e;
  e = (6.112 * pow(10, (7.5 * temperature / (237.7 + temperature))) * humidity / 100); //vapor pressure
  float humidex = temperature + 0.55555555 * (e - 10.0); //humidex
  return humidex;
}


void handlePageSelect(){
  
   if( server.arg("page").toInt() <=6 ) {
    
      if( server.arg("big") == "1" ) bigDig = true;
      else if (server.arg("big") == "0" ) bigDig = false;
      
      mode0scr = server.arg("page").toInt();
      
      redrawAllScreen();
   }
   
   server.send(200, "text/html", "<script>\nwindow.location.replace(\"http://" + String(myIP) + "\");\n</script>");
}

void HandleClient() {
  
  unsigned long webpageLoad = millis();
  
  char daysOfTheWeek[9][12] = {"   Sunday", "   Monday", "  Tuesday", "Wednesday", " Thursday", "   Friday", " Saturday"};
  
  DateTime now = rtc.now();
  String webpage;
  
  webpage =  "<html>\n<head><title>"+hostName+"</title><meta charset='UTF-8' http-equiv='refresh' content='60' >\n";
  webpage += R"rawliteral(   <style>
        body {font-family: Sans; Color: #00979d;}
        h2 {line-height: 10%;}
        p {line-height: 20%;}
        .chart { height: 140px; width: 500; }
        .chart .grid { stroke: #ccc; stroke-dasharray: 0; stroke-width: 2;}
        .labels { font-size: 13px;}
        .chart .plotData { fill: red; stroke-width: 1;}
        .float { float: left;}
        .floatclear { clear: both;}
    </style>
    </head>
    <body>

    )rawliteral";
  webpage += "<p><br><b>&nbsp;&nbsp; " + hostName + " </b></p><br>\n";
  
  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; температура </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispTemp) + " °C</h2>\n";
  webpage += "<p> &nbsp; чувствуется как:</p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(calculate_humidex(dispTemp,dispHum)) + " °C</h2>\n"; 
  webpage += genPlot((int*)tempHour);
  webpage += genPlot((int*)tempDay);  
  
#if (EXT_SENS >= 1)
  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; внешняя температура </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;";
  
  if(dispExtTemp != 255) {
    webpage += String(dispExtTemp) + " °C</h2>\n";
    webpage += "<p> &nbsp; чувствуется как:</p>\n";
    webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(calculate_humidex(dispExtTemp,dispExtHum)) + " °C</h2>\n"; 
    
    webpage += genPlot((int*)tempExtHour);
    webpage += genPlot((int*)tempExtDay);  
    
    webpage += "<div class=\"floatclear\"></div><br><p>&nbsp;внешняя влажность </p>\n";
    webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispExtHum) + " %  </h2>\n";
  }
  else  webpage += "-Sensor Error-</h2>\n";
#endif

  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; влажность </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispHum) + " %  </h2>\n";
      
  webpage += genPlot((int*)humHour);
  webpage += genPlot((int*)humDay);  
  
  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; атмосферное давление </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispPres) + " hPa</h2>\n";
      
  webpage += genPlot((int*)pressHour);
  webpage += genPlot((int*)pressDay); 
  
  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; вероятность осадков </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispRain) + " %</h2><br>\n";
  webpage += "<p>&nbsp; содержание CO2 </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispCO2) + " ppm</h2>\n";
  
  for (byte i = 0; i < 15; i++) {                                          //cleaning zero values from empty cells and
    if((co2Hour[i]<CO2_MIN) && (dispCO2>=CO2_MIN)) co2Hour[i] = dispCO2;   //filter out cold sensor readings
    if((co2Day[i]<CO2_MIN) && (dispCO2>=CO2_MIN)) co2Day[i] = dispCO2;
  }
  webpage += genPlot((int*)co2Hour);
  webpage += genPlot((int*)co2Day); 
  
  webpage += "<div class=\"floatclear\"></div>\n<p><br>&nbsp;&nbsp;&nbsp;&nbsp; RTC:" + printDigits(now.hour()) + ":" + printDigits(now.minute()) + ":"  + printDigits(now.second()) + " &nbsp;&nbsp;" + daysOfTheWeek[now.dayOfTheWeek()];
  webpage += " &nbsp;" + printDigits(now.day()) + "/" + printDigits(now.month()) + "/"  + String(now.year()) + "</p>&nbsp;&nbsp;\n";

  webpage += "<p>&nbsp;&nbsp;&nbsp;&nbsp; NTP:" + printDigits(hour()) + ":" + printDigits(minute()) + ":"  + printDigits(second()) + " &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n";
  webpage += " &nbsp;" + printDigits(day()) + "/" + printDigits(month()) + "/"  + String(year()) + "</p>&nbsp;&nbsp;\n";
  webpage += "<p>&nbsp;&nbsp;&nbsp;&nbsp;Uptime: "+Uptime()+",&nbsp;&nbsp;WiFi signal: "+ String( WiFi.RSSI()) + "dBm, </p><p>&nbsp;&nbsp;&nbsp;&nbsp;Battery: "+String((float)batteryVoltCalc()/100)+"V ("+analogRead(BATTERY)+"), &nbsp;&nbsp;ExternalBattery: "+String( (float) dispBattExtVolt/100 )+"V,&nbsp;&nbsp;Page generation: "+String(millis()-webpageLoad)+"msec</p><br><br>\n";

  webpage += R"rawliteral(   
<form  action="/pageselect" method="get">
  <input type="radio"  name="big" value="0">
  <label>Small letters</label>
  <input type="radio"  name="big" value="1">
  <label>Big letters</label>
  <select name="page">
    <option value="0">Clock</option>
    <option value="1">CO2</option>
    <option value="2">Temperature</option>
    <option value="3">Pressure</option>
    <option value="4">Humidity</option>
    <option value="5">Temp External</option>
    <option value="6">Hum External</option>
  </select>
  <input type="submit" value="Submit">
</form>
  )rawliteral";
  
  webpage += "<p>&nbsp;&nbsp; \n<a href='http://"+String(myIP)+"/set_WI_FI'>НАСТРОЙКИ</a></p>&nbsp;&nbsp;<p>\n";
  webpage += "</body>\n";
  webpage += "</html>\n";
  server.send(200, "text/html", webpage);
}


void handleRoot() {
  String webpage;
  webpage =  "<html>"; 
  webpage += "<head><title> Setup </title><meta charset='UTF-8'>";
  webpage += "<style>";
  webpage += "body { font-family: Verdana; Color: #00979d;}";
  webpage += "</style>";
  webpage += "</head>";
  webpage += "<body>";

  String str = "";
  str += webpage;
  str += "<boy>\
   <form method=\"POST\" action=\"ok\">\
     <input type=\"radio\" value=\"1\" name=\"otaflag\"> Загрузить новую прошивку (после перезагрузки зайдите на страницу устройства и запустите процедуру)</br></br>\
     <input type=\"text\" value=\"" + ssid + "\" name=\"ssid\" maxlength=32> WiFi SSID</br></br>\  
     <input type=\"password\" value=\"" + pass + "\" name=\"pswd\" maxlength=64> PASSWORD</br></br>\
     <input type=\"text\" value=\"" + TIMEZONE + "\" name=\"tzn\" maxlength=3> TIMEZONE</br></br>\
     <input type=\"text\" value=\"" + mqtt_ip + "\" name=\"mqtt_ip\" maxlength=15> MQTT IP</br></br>\
     <input type=\"text\" value=\"" + mqtt_port + "\" name=\"mqtt_port\" maxlength=5> MQTT PORT</br></br>\
     <input type=\"text\" value=\"" + mqtt_auth + "\" name=\"mqtt_auth\" maxlength=32> MQTT USER</br></br>\
     <input type=\"password\" value=\"" + mqtt_pass + "\" name=\"mqtt_pass\" maxlength=32> MQTT PWD</br></br>\
     <input type=\"text\" value=\"" + mqtt_Temp + "\" name=\"mqtt_temp\" maxlength=64> MQTT Topic (temperature)</br></br>\
     <input type=\"text\" value=\"" + mqtt_Hum + "\" name=\"mqtt_hum\" maxlength=64> MQTT Topic (humidity)</br></br>\
     <input type=\"text\" value=\"" + mqtt_Press + "\" name=\"mqtt_press\" maxlength=64> MQTT Topic (pressure)</br></br>\
     <input type=\"text\" value=\"" + mqtt_CO2 + "\" name=\"mqtt_co2\" maxlength=64> MQTT Topic (CO2)</br></br>\
     <input type=SUBMIT value=\"Save\">\
   </form>\
 </body>\
</html>";

  server.send ( 200, "text/html", str );
} 


void handleOk(){
  String webpage;
  webpage =  "<html>";
  webpage += "<head><title>settings save </title><meta charset='UTF-8'>";
  webpage += "<style>";
  webpage += "body { font-family: Verdana; Color: #00979d;}";
  webpage += "</style>";
  webpage += "</head>"; 
  webpage += "<body>";
  String ssid_ap       = server.arg("ssid");
  String pass_ap       = server.arg("pswd");
  String TZN_ap        = server.arg("tzn");
  String mqtt_ip_ap    = server.arg("mqtt_ip");
  String mqtt_port_ap  = server.arg("mqtt_port");
  String mqtt_auth_ap  = server.arg("mqtt_auth");
  String mqtt_pass_ap  = server.arg("mqtt_pass");
  String mqtt_temp_ap  = server.arg("mqtt_temp");
  String mqtt_hum_ap   = server.arg("mqtt_hum");
  String mqtt_press_ap = server.arg("mqtt_press");
  String mqtt_CO2_ap   = server.arg("mqtt_co2");
  String otaFlag_ap    = server.arg("otaflag");
  int tz;
  String str = "";
 
  str += webpage;
  str += "<body>";
  tz = TZN_ap.toInt();

  (otaFlag_ap == "0") ? otaFlag = 0 : otaFlag = 1;
  
  if( (tz > -12) && (tz < 12) ) TIMEZONE = tz;
  mqtt_ip    = mqtt_ip_ap;
  mqtt_port  = mqtt_port_ap;
  
  mqtt_auth_ap.replace("%2F","/");
  mqtt_auth  = mqtt_auth_ap;
  
  mqtt_pass_ap.replace("%2F","/");
  mqtt_pass  = mqtt_pass_ap;
  
  mqtt_temp_ap.replace("%2F","/");
  mqtt_Temp  = mqtt_temp_ap;
  
  mqtt_hum_ap.replace("%2F","/");
  mqtt_Hum   = mqtt_hum_ap;
  
  mqtt_press_ap.replace("%2F","/");
  mqtt_Press = mqtt_press_ap;
  
  mqtt_CO2_ap.replace("%2F","/");
  mqtt_CO2   = mqtt_CO2_ap;

  ssid_ap.replace("%2F","/");
  pass_ap.replace("%2F","/");

  str +="Configuration saved in FS</br>\   
  <a href=\"/\">Return</a> to settings page</br>";
  str += "</body></html>";
  server.send ( 200, "text/html", str );
  
  saveConfig();

  if( (ssid_ap != String(ssid)) || (pass_ap != String(pass)) ){
      ssid = ssid_ap;
      pass = pass_ap;
      saveConfig();
      delay(1000);
      ESP.restart();
      delay(100);
  }
  if( otaFlag ) {
    lcd.clear();
    lcd.print("Rebooting...");
    delay(1000);
    ESP.restart();
    delay(100);
  }
}


void handleOTA() {
  Serial.println("Starting OTA mode.");    
  Serial.printf("Sketch size: %u\n", ESP.getSketchSize());
  Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());
  MDNS.begin(host);
  server.on("/", HTTP_GET, [](){
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/html", otaServerIndex);
  });
  server.on("/update", HTTP_POST, [](){
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    setOtaFlag(0); 
    lcd.clear();
    delay(100);
    ESP.restart();
  },[](){
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
      //Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\n", upload.filename.c_str());
      otaCount=300;
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if(!Update.begin(maxSketchSpace)){//start with max available size
        Update.printError(Serial);
      }
    } else if(upload.status == UPLOAD_FILE_WRITE){
      if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
        Update.printError(Serial);
      }
    } else if(upload.status == UPLOAD_FILE_END){
      if(Update.end(true)){ //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });
  server.begin();
  Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  MDNS.addService("http", "tcp", 80);
  otaTickLoop.attach(1, otaCountown);
}


