
// void logDataToSD(float _temperature, float _humidity, uint8_t _pm1, float _pm25, float _pm10, uint8_t _COppm)
// {
//   SD.begin(PIN_CS_SD);
//   char fileName[11];
//   char nameAndDay[24];
//   sprintf(fileName, "%02d%02d%02d.txt", dt.day, dt.month, dt.year - 2000);
//   for(uint8_t i = 0; i<12; i++)
//   {
//     nameAndDay[i] = nameDevice[i];
//   }
//   nameAndDay[12] = '-';
//   for(uint8_t i = 0; i<10; i++)
//   {
//     nameAndDay[i + 13] = fileName[i];
//   }
//   nameAndDay[23] = NULL;
//   File f = SD.open(nameAndDay, FILE_WRITE);

//   if (f)
//   { 
//     f.print(nameDevice);
//     f.print(",");
    
//     f.print(dt.year);
//     f.print("-");
//     f.print(dt.month);
//     f.print("-");
//     f.print(dt.day);
//     f.print(" ");

//     f.print(dt.hour);
//     f.print(":");
//     f.print(dt.minute);
//     f.print(":");
//     f.print(dt.second);
//     f.print(",");
//     f.print(dt.unixtime-7*3600);
//     f.print(",");
    
//     f.print(_temperature);
//     f.print(",");
//     f.print(_humidity);
//     f.print(",");
    
//     f.print(_pm25);
//     f.print(",");
//     f.print(_pm10); 

//     f.println();   

//     f.close();
//   }
//   SD.end();
// }