#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TroykaDHT.h>
#include <Wire.h>
#include <DS3231.h>

namespace pin{
  const byte tds_sensor = 34;
  const byte ph_sensor = 35;
  const byte soil_sensor = 36; 
  const byte dht21_sensor = 19; 
  const byte one_wire_bus = 7;
  const byte led_builtin = 2;
}

namespace device
{
  float aref = 4.3;
} // namespace device

namespace sensor
{
  float ec = 0;
  unsigned int tds = 0;
  float waterTemp = 0;
  float ecCalibration = 1;
  int smvalue = 0;
  int smpercent = 0;
} // namespace sensor

OneWire oneWire(pin::one_wire_bus);
DallasTemperature dallasTemp(&oneWire);
//DHT dht(pin::dht21_sensor, DHT21);
RTClib rtc;
void readTdsQuick();
void getSoilPercent();
void setup() {
  // put your setup code here, to run once:
  pinMode(pin::led_builtin,OUTPUT);
  Serial.begin(115200);
  //Wire.begin();
  delay(500);
  Serial.println("esp32 Ready!");
}
 
void loop() {
  // put your main code here, to run repeatedly:
  /*readTdsQuick();
  dht.read();
  // проверяем состояние данных
  switch(dht.getState()) {
    // всё OK
    case DHT_OK:
      // выводим показания влажности и температуры
      Serial.print("Temperature = ");
      Serial.print(dht.getTemperatureC());
      Serial.println(" C \t");
      Serial.print("Temperature = ");
      Serial.print(dht.getTemperatureK());
      Serial.println(" K \t");
      Serial.print("Temperature = ");
      Serial.print(dht.getTemperatureF());
      Serial.println(" F \t");
      Serial.print("Humidity = ");
      Serial.print(dht.getHumidity());
      Serial.println(" %");
      break;
    // ошибка контрольной суммы
    case DHT_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    // превышение времени ожидания
    case DHT_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    // данных нет, датчик не реагирует или отсутствует
    case DHT_ERROR_NO_REPLY:
      Serial.println("Sensor not connected");
      break;
  }
  
  DateTime now = rtc.now();
    
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  */
  digitalWrite(pin::led_builtin,HIGH);
  delay(500);
  digitalWrite(pin::led_builtin,LOW);
  delay(500);
}

void readTdsQuick(){
  dallasTemp.requestTemperatures();
  sensor::waterTemp = dallasTemp.getTempCByIndex(0);
  float rawEc = analogRead(pin::tds_sensor) * device::aref / 1024;
  float tempCoefficient = 1.0 + 0.02 * (sensor::waterTemp - 25.0);
  sensor::ec = (rawEc / tempCoefficient) * sensor::ecCalibration;
  sensor::tds = (113.42 * pow(sensor::ec,3) - 255.86 * sensor::ec * sensor::ec * 857.39 * sensor::ec) * 0.5;
}

void getSoilPercent(){
  sensor::smvalue = analogRead(pin::soil_sensor);
  sensor::smpercent = map(sensor::smvalue,0,1023,100,0);
}
