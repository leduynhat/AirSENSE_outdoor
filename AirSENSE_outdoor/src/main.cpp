/*
  AIR SENSE code 

  when pressing a pushbutton arduino starts "smart config"
    The circuit:
  - sds011 sleeps from s0 until s30
  - read data at s0
  - sds011 wake up at s30
  - read htu21d data every 30 seconds
  - every three minutes, 3 data from sensor will be sent to mqtt

  - note: successfully
  created 2019
  by Lê Duy Nhật <https://www.facebook.com/conca.bietbay.7>
  modified 16/11/2019
  by hocj2me- Lê Chí Tuyền
  
*/
#include "./ht_data.h"
#include "./sd_card.h"
#include "./sds_data.h"
#include "./json_message.h"
#include "./init_.h"
#include "./queuing_data.h"
unsigned char bytes[19] = {
  0xAA, // head
  0xB4, // command id
  0x08, // data byte 1
  0x01, // data byte 2 set
  0x01, // data byte 3 work period 5 minutes: measure for 30 seconds + sleep the rest
  0x00, // data byte 4
  0x00, // data byte 5
  0x00, // data byte 6
  0x00, // data byte 7
  0x00, // data byte 8
  0x00, // data byte 9
  0x00, // data byte 10
  0x00, // data byte 11
  0x00, // data byte 12
  0x00, // data byte 13
  0xFF, // data byte 14 (device id byte 1)
  0xFF, // data byte 15 (device id byte 2)
  0x10, // checksum (low 8 bit of sum of data bytes) CHƯA TÍNH
  0xAB  // tail
};
void logDataToSD(float _temperature, float _humidity, uint8_t _pm1, float _pm25, float _pm10, uint8_t _COppm)
{
  SD.begin(PIN_CS_SD);
  char fileName[11];
  char nameAndDay[24];
  sprintf(fileName, "%02d%02d%02d.txt", dt.year - 2000, dt.month, dt.day);
  for(uint8_t i = 0; i<12; i++)
  {
    nameAndDay[i] = nameDevice[i];
  }
  nameAndDay[12] = '-';
  for(uint8_t i = 0; i<10; i++)
  {
    nameAndDay[i + 13] = fileName[i];
  }
  nameAndDay[23] = NULL;
  File f = SD.open(nameAndDay, FILE_WRITE);

  if (f)
  { 
    f.print(nameDevice);
    f.print(",");
    
    f.print(dt.year);
    f.print("-");
    f.print(dt.month);
    f.print("-");
    f.print(dt.day);
    f.print(" ");

    f.print(dt.hour);
    f.print(":");
    f.print(dt.minute);
    f.print(":");
    f.print(dt.second);
    f.print(",");
    f.print(dt.unixtime-7*3600);
    f.print(",");
    
    f.print(_temperature);
    f.print(",");
    f.print(_humidity);
    f.print(",");
    
    f.print(_pm25);
    f.print(",");
    f.print(_pm10); 

    f.println();   

    f.close();
  }
  SD.end();
}

void createAndPrintJSON(data _send_data, String names)
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
    doc["NodeId"]     = "names";
    //Serial.println();
    serializeJson(doc, mes);
    Serial.println(mes);
    if(mqttClient.publish(topic, mes, true))
    {
      Serial.println(F("***Published successfully!***"));
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
bool readDataSDS(float *p25,float *p10)
{
  int i=0;
  int out25, out10;
  int checksum = 0;
  bool oke = false;
  bool done = false;
  while((Serial.available()>0)){
    byte k = Serial.read();
    switch(i){
        case (0): if (k != 170) { i = -1; }; break;
        case (1): if (k != 192) { i = -1; }; break;
        case (2): out25 = k;        checksum = k; break;
        case (3): out25 += (k << 8);  checksum += k; break;
        case (4): out10 = k;        checksum += k; break;
        case (5): out10 += (k << 8);  checksum += k; break;
        case (6):                   checksum += k; break;
        case (7):                   checksum += k; break;
        case (8): if (k == (checksum % 256)) { 
                    oke = true; 
                  } else {
                    i = -1; 
                  }; break;
        case (9): if (k != 171) i=-1;break;
      }
      i++;
    if ((oke==true)&&(i==10))
    {
      *p25 = (float)out25/10;
      *p10 = (float)out10/10;
      oke = false;
      i=0;
      done = true;
    }
    yield();
  }
  return done;
}
int calculateChecksum() 
{
    int sum = 0;
    for (int i = 2; i < 14; ++i) {
      sum += bytes[i];
    }
    sum += bytes[15] + bytes[16];
    return (sum % 256);
}
void getHTUData()
{
  Temperature = htu.readTemperature();
  Humidity = htu.readHumidity();
  temperatureSum += Temperature;
  humiditySum += Humidity;
  HTUCount++;
}
void Work_per_minute(int times)
{
  bytes[4] = times;
  bytes[17] = calculateChecksum();
  for(uint8_t i = 0;i<19;i++) 
  {
    Serial.write(bytes[i]);
  }
  Serial.flush();//delay cho gui xong
}

//====================
// kiểm tra thời gian
//====================
void checkAlarm()
{
  if(Clock.isAlarm1())
  { 
    ControlFlag |= 0b01010100;//set fag sleep PMS,
  }
  if(Clock.isAlarm2())
  {
    ControlFlag |= 0b10101000;//set bit wake PMS, 
    ControlFlag ++;
  }
}

//===========================================================
// xử lý dữ liệu nhiệt độ và độ ẩm thành kết quả trung bình
//===========================================================
void processingData()
 {
   if(HTUCount != 0)
   {
     Humidity = (float)humiditySum/HTUCount;
     Temperature = (float)temperatureSum/HTUCount;
     HTUCount = 0;
     humiditySum = 0;
     temperatureSum = 0;
   }
   if(SDScount != 0)
   {    
     PM2_5 = (float)PM2_5Sum/SDScount;
     PM10 = (float)PM10Sum/SDScount;
     SDScount = 0;
     PM2_5Sum = 0;
     PM10Sum = 0;
   }
 }

void setup() 
  {
//=======================================================
        timeClient.begin();
        Serial.begin(9600);
        Wire.begin();
        Clock.begin();
        mqtt_begin();
        SPIFFS.begin();
        htu.begin();
//========================================================
        
//        Serial.setTimeout(50);
        
        pinMode(PIN_BUTTON, INPUT);
        pinMode(PIN_LED, OUTPUT);
        digitalWrite(PIN_LED, HIGH);
        WiFi.mode(WIFI_STA);
//        delay(1000);
        
        Work_per_minute(1); // 1 minute
        Clock.armAlarm1(false);
        Clock.armAlarm2(false);
        Clock.clearAlarm1();
        Clock.clearAlarm2();
        Clock.setAlarm2(0, 0, 0, DS3231_EVERY_MINUTE,true);
        Clock.setAlarm1(0, 0, 0, 30, DS3231_MATCH_S,true);
        f_init(fileName);
//        Serial.println(F("setup done"));
    	  randomSeed(analogRead(A0));
        ESP.wdtDisable();//watchdog timer
//        dt = Clock.getDateTime();
//        logDataToSD(Temperature, Humidity, 0, PM2_5, PM10, 0);
//========================================================
}
void loop() 
{
//========================================================
//        delay(10);
        checkAlarm();
        if (longPress())
        {
            digitalWrite(PIN_LED, LOW);
            if(WiFi.beginSmartConfig())
            {
//               Serial.println(F("Enter smart config"));
            }
     
        } 
        else if(WiFi.status() == WL_CONNECTED)
        {

          //==========================================================
          digitalWrite(PIN_LED, HIGH);  
          if(((lastGetTime == 0) || ((millis() - lastGetTime) > 163000))&& (millis() < timeLimit))
          {
              if(timeClient.update())
              {
                  lastGetTime = millis();
                  uint32_t internetTime = timeClient.getEpochTime();
                  dt = Clock.getDateTime();
                  if (abs(internetTime - dt.unixtime) > 3)
                  {
                    Clock.setDateTime(internetTime);
                    Serial.println("done update time"); 
                  } 
              }
          }
          //========================================================================================
          if((isQueueEmpty()==false) && (checkQueue == 1) && (millis() < timeLimit))// (1)
          {
              time2SendMessage = millis() + random(50,100)*100; //time delay to send
              checkQueue = 0;
          }
          if((millis() > time2SendMessage)&& (checkQueue == 0))
          {
              if(mqttClient.connected())
              {
                    data send_data = deQueue();
                    createAndPrintJSON(send_data, nameDevice);
                    checkQueue = 1;
                    mqttClient.loop();      
              }
              else if(mqttClient.connect(espID))
              {
                   Serial.println(F("reconnected mqtt"));
              }   
          }      
        }
//========================================================================           
//       if(ControlFlag & 0b00001000)
//       {
//          getHTUData();
//          Serial.println("Get Data");
//          ControlFlag &= 0b11110111;
//          
//       }
//       else if(ControlFlag & 0b00000100)
//       {
//          getHTUData();
//          ControlFlag &= 0b11111011;
//       }
         if(ControlFlag & 0b00001100)
         {
           float p2_5, p10;
            if (readDataSDS(&p2_5, &p10))
            {
               PM10Sum += p10;
               PM2_5Sum += p2_5;
               SDScount ++;
            }
            getHTUData();// get temperature and humidity data
            ControlFlag &= 0b11110011;
//            Serial.println("get data completed");
         }

       if((ControlFlag & 0b00000011) == 1)  // 3 phút xử lý dữ liệu 1 lần
       {
          processingData();
          dt = Clock.getDateTime();
          queueData(Temperature, Humidity, 0, PM2_5, PM10, 0, dt.unixtime);
          logDataToSD(Temperature, Humidity, 0, PM2_5, PM10, 0);
          ControlFlag &= 0b11111100;
        //  Serial.println(F("queuing completed!"));
       }
//==========================================================================
       ESP.wdtFeed();  

}
