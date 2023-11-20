const char* otaServerIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

String printDigits(int digits)
{
  String f=String(digits);
  if (digits < 10) return ("0"+f); else return f;
}

void HandleClient() 
{
  char daysOfTheWeek[9][12] = {"   Sunday", "   Monday", "  Tuesday", "Wednesday", " Thursday", "   Friday", " Saturday"};
  DateTime now = rtc.now();
  String webpage;
  webpage =  "<html>";
  webpage += "<head><title>"+hostName+"</title><meta charset='UTF-8' http-equiv='refresh' content='5' >";
  webpage += "<style>";
  webpage += "body {font-family: Verdana; Color: #00979d;}";
  webpage += "h2 {line-height: 10%;} ";
  webpage += "p {line-height: 20%;} ";
  webpage += "</style>";
  webpage += "</head>";
  webpage += "<body>";
  webpage += "<p><b>&nbsp;&nbsp; " + hostName + " </b></p><br>";
  webpage += "<p>&nbsp; температура </p>";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispTemp) + " °C</h2><br>";
  webpage += "<p>&nbsp; влажность </p>";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispHum) + " %  </h2><br>";
  webpage += "<p>&nbsp; атмосферное давление </p>";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispPres) + " мм р.ст.</h2><br>";
  webpage += "<p>&nbsp; содержание CO2 </p>";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispCO2) + " ppm</h2><br>";
   
  webpage += "<p>&nbsp;&nbsp;&nbsp;&nbsp; RTC:" + printDigits(now.hour()) + ":" + printDigits(now.minute()) + ":"  + printDigits(now.second()) + " &nbsp;&nbsp;" + daysOfTheWeek[now.dayOfTheWeek()];
  webpage += " &nbsp;" + printDigits(now.day()) + "/" + printDigits(now.month()) + "/"  + String(now.year()) + "</p>&nbsp;&nbsp;";

  webpage += "<p>&nbsp;&nbsp;&nbsp;&nbsp; NTP:" + printDigits(hour()) + ":" + printDigits(minute()) + ":"  + printDigits(second()) + " &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;";
  webpage += " &nbsp;" + printDigits(day()) + "/" + printDigits(month()) + "/"  + String(year()) + "</p>&nbsp;&nbsp;";

  webpage += "<p>&nbsp;&nbsp; <a href='http://"+String(myIP)+"/set_WI_FI'>НАСТРОЙКИ</a></p>";
  webpage += "</body>";
  webpage += "</html>";
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
  str += "<body>\
   <form method=\"POST\" action=\"ok\">\
     <input type=\"text\" value=\"" + ssid + "\" name=\"ssid\" maxlength=32> WiFi SSID</br></br>\  
     <input type=\"password\" value=\"" + pass + "\" name=\"pswd\" maxlength=64> PASSWORD</br></br><hr>\
     <input type=\"text\" value=\"" + TIMEZONE + "\" name=\"tzn\" maxlength=3> TIMEZONE</br></br><hr>\
     <input type=\"text\" value=\"" + mqtt_ip + "\" name=\"mqtt_ip\" maxlength=15> MQTT IP</br></br>\
     <input type=\"text\" value=\"" + mqtt_port + "\" name=\"mqtt_port\" maxlength=5> MQTT PORT</br></br>\
     <input type=\"text\" value=\"" + mqtt_auth + "\" name=\"mqtt_auth\" maxlength=32> MQTT USER</br></br>\
     <input type=\"password\" value=\"" + mqtt_pass + "\" name=\"mqtt_pass\" maxlength=32> MQTT PWD</br></br>\
     <input type=\"text\" value=\"" + mqtt_Temp + "\" name=\"mqtt_temp\" maxlength=64> MQTT Topic (temperature)</br></br>\
     <input type=\"text\" value=\"" + mqtt_Hum + "\" name=\"mqtt_hum\" maxlength=64> MQTT Topic (humidity)</br></br>\
     <input type=\"text\" value=\"" + mqtt_Press + "\" name=\"mqtt_press\" maxlength=64> MQTT Topic (pressure)</br></br>\
     <input type=\"text\" value=\"" + mqtt_CO2 + "\" name=\"mqtt_co2\" maxlength=64> MQTT Topic (CO2)</br></br><hr>\
     <input type=\"text\" value=\"" + TempCorrection + "\" name=\"tmpcorr\" maxlength=3> Temperature correction</br></br>\
     <input type=\"text\" value=\"" + HumCorrection + "\" name=\"humcorr\" maxlength=3> Humidity correction</br></br><hr>\
     <input type=\"radio\" value=\"0\" name=\"otaflg\"> Сохранить настройки</br></br>\
     <input type=\"radio\" value=\"1\" name=\"otaflg\"> Загрузить новую прошивку</br></br><hr>\
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
  String TMPCORR_ap    = server.arg("tmpcorr");
  String HUMCORR_ap    = server.arg("humcorr");
  String otaFlag_ap    = server.arg("otaflg");
  int tz;
  int tmpc;
  int humc;
  String str = "";
 
  str += webpage;
  str += "<body>";
  tz = TZN_ap.toInt();

  otaFlag = (otaFlag_ap == "0") ? 0 : 1;
  
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

  tmpc = TMPCORR_ap.toInt();
  TempCorrection = tmpc;

  humc = HUMCORR_ap.toInt();
  HumCorrection = humc;

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
