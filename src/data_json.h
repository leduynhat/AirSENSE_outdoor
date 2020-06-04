void createAndPrintJSON(data _send_data,char* names)
 {
    float co    = _send_data.co + _send_data.dot_co*0.01;
    float hum   = _send_data.hum + _send_data.dot_hum*0.01;
    float tem   = _send_data.temp + _send_data.dot_temp*0.01;
    float pm1   = _send_data.pm1 + _send_data.dot_pm1*0.01;
    float pm2p5 = _send_data.pm25 + _send_data.dot_pm25*0.01;
    float pm10  = _send_data.pm10 + _send_data.dot_pm10*0.01;
    uint32_t realtimes = _send_data.epoch_time;
     DynamicJsonDocument doc(256);
    char mes[256] = {0};
    //JsonObject obj = doc.to<JsonObject>();
    JsonObject DATA = doc.createNestedObject("DATA"); 
    DATA["CO"]        = co; 
    DATA["Hum"]       = hum;
    DATA["Pm1"]       = pm1;
    DATA["Pm10"]      = pm10;
    DATA["Pm2p5"]     = pm2p5;
    DATA["Time"]      = realtimes - 7*3600;
    DATA["Tem"]       = tem;
    //JsonObject NAME = doc.createNestedObject("NAME"); 
    doc["NodeId"]     = names;
    //Serial.println();
    serializeJson(doc, mes);
    if(mqttClient.publish(topic, mes, true))
    {
//      Serial.println(F("***Published successfully!***"));
      for(uint8_t i=0; i<10; i++){
        digitalWrite(PIN_LED, LOW);
        delay(500);
        digitalWrite(PIN_LED, HIGH);
        delay(500);
      }
    }
}
 void queueData(float temp, float humi, float pm1, float pm25, float pm10, float co, uint32_t unix)
{
    data queuing;
    
    queuing.temp = temp;
    queuing.dot_temp = (temp - queuing.temp) * 100 ;
    queuing.hum = humi;
    queuing.dot_hum = (humi - queuing.hum) * 100;
  
    queuing.pm1 = pm1;
    queuing.dot_pm1 = (pm1 - queuing.pm1) * 100;
    queuing.pm25 = pm25;
    queuing.dot_pm25 = (pm25 - queuing.pm25) * 100 ;
    queuing.pm10 = pm10;
    queuing.dot_pm10 = (pm10 - queuing.pm10) * 100;
  
    queuing.co = co;
    queuing.dot_co = (co - queuing.co) * 100;
  
    queuing.epoch_time = unix;
    enQueue(queuing);
    Serial.println(F("queuing completed"));
}
