#if (EXT_SENS == 1)
void readExtWeather(){

//   Serial.println(EXT_SENS_HOST);
  
  WiFiClient client;

  HTTPClient http;

//   Serial.print("[HTTP] begin...\n");
  if (http.begin(client, "http://meteo_out/json")) {  // HTTP


//     Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        extSensorStatus = 200;
        String payload = http.getString();
        jsonToData(payload);
      }
      
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      extSensorStatus = 404;
      dispExtTemp = 255;
      dispExtHum = 255;
    }

    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
    extSensorStatus = 503;
    dispExtTemp = 255;
    dispExtHum = 255;
  }
}
#endif


String jsonToData(String inputJson){
  const size_t capacity = JSON_OBJECT_SIZE(5) + 40;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, inputJson);

  dispExtTemp = doc["temp"]; 
  dispExtHum = doc["hum"]; 
  dispExtPres = doc["pressure"]; 
  dispBattExtVolt = doc["battery"]; 
}


#if (EXT_SENS == 2)
void handleExtSensor(){
  if( (server.arg("status") == "1") || (server.arg("status") == "600") ){
    
    dispExtTemp = server.arg("temp").toFloat() + TEMP_COMPENSATION_EXT ;
    dispExtHum = server.arg("hum").toInt();
    dispExtPres = server.arg("pres").toInt();
    dispBattExtVolt = server.arg("volt").toInt();
    dispExtWiFi = (int) server.arg("wifi").toInt();
    dispExtSyncTime = ((unsigned long) server.arg("timeout").toInt())+300000+millis();
    
    Serial.println("Got temp: " + server.arg("temp"));
    
    server.send(200, "text/html", "OK");
    
  } else if (server.arg("status") == "418") {
    dispExtTemp = 255;
    dispExtHum = 255;
    server.send(418, "text/html", "Sensor error");
    
  } else server.send(503, "text/html", "Error on data");
  
//   if (server.arg("status") == "600") //low power
}
#endif
