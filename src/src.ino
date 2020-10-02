
/*
  AirSENSE  

  When pressing a pushbutton arduino starts "smart config"
    The circuit:
  - sds011 sleeps from s0 until s30
  - read data at s0
  - sds011 wake up at s30
  - read htu21d data every 30 seconds
  - every three minutes, 3 data from sensor will be sent to mqtt

  - note: successfully
  created 2019
  by Lê Duy Nhật <https://www.facebook.com/conca.bietbay.7>

  modified 04/06/2020
  by hocj2me- Lê Chí Tuyền
  
*/
#include "./esp.h"
#include "./sds_tab.h"
#include "./sd_card.h"
#include "./data_json.h"
#include "./ht_data.h"



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
        pinMode(PIN_BUTTON, INPUT);
        pinMode(PIN_LED, OUTPUT);
        digitalWrite(PIN_LED, HIGH);
        Serial.setTimeout(50);
//led báo hiệu khi thẻ nhớ không được gắn vào thiết bị
        if(SD.begin(PIN_CS_SD)!= true){
          for(uint8_t i = 0; i< 10; i++){
            digitalWrite(PIN_LED, LOW);
            delay(200);
            digitalWrite(PIN_LED, HIGH);
            delay(200);
          }
        }
        else{
            digitalWrite(PIN_LED, LOW);
            delay(2000);
            digitalWrite(PIN_LED, HIGH);
        }
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
            WiFi.beginSmartConfig();
			int loopCounter = 0;
			Serial.println(F("Enter smart config"));
			  while (!WiFi.smartConfigDone()) 
			  {
				 Serial.println("."); 
				 if(loopCounter >= 40)
				 {
					 loopCounter = 0;
					 Serial.printf("\n");
				 }
				 delay(600);
				 ++loopCounter;
				 ESP.wdtFeed();
			  }
			//digitalWrite(PIN_LED, HIGH);
        } 
        else if(WiFi.status() == WL_CONNECTED)
        {
			digitalWrite(PIN_LED, HIGH);

          //==========================================================
          
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
//                    Serial.println("done update time"); 
                  } 
              }
          }
          //========================================================================================
//          if((isQueueEmpty()==false) && (checkQueue == 1) && (millis() < timeLimit))// (1)
//          {
//              time2SendMessage = millis() + random(50,100)*100; //time delay to send
//              checkQueue = 0;
//          }
//          if((millis() > time2SendMessage)&& (checkQueue == 0))
//          {
//              if(mqttClient.connected())
//              {
//                    data send_data = deQueue();
//                    createAndPrintJSON(send_data, nameDevice);
//                    checkQueue = 1;
//                    mqttClient.loop();      
//              }
//              else if(mqttClient.connect(espID))
//              {
////                    Serial.println(F("reconnected mqtt"));
//              }   
//          }
          if(isQueueEmpty()==false){
             if(mqttClient.connected()){
                data send_data = deQueue();
                createAndPrintJSON(send_data, nameDevice);
                mqttClient.loop();   
             }
             else if(mqttClient.connect(espID)){
//                Serial.println(F("reconnected mqtt"));
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

       if((ControlFlag & 0b00000011) == 3)  // 3 phút xử lý dữ liệu 1 lần
       {
          processingData();
          dt = Clock.getDateTime();
          queueData(Temperature, Humidity, 0, PM2_5, PM10, 0, dt.unixtime);
          logDataToSD(Temperature, Humidity, 0, PM2_5, PM10, 0);
          ControlFlag &= 0b11111100;
//          Serial.println(F("queuing completed!"));
       }
//==========================================================================
       ESP.wdtFeed();  
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
   if(millis() > RESTART_PERIOD)
   {
     ESP.restart();
   }
 }
