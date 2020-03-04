
//================================
// nhận dữ liệu nhiệt độ và độ ẩm
//================================
void getHTUData()
{
  Temperature = htu.readTemperature();
  Humidity = htu.readHumidity();
  temperatureSum += Temperature;
  humiditySum += Humidity;
  HTUCount++;
}
