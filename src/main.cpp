#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// #include <TroykaDHT.h>
#include <DHT.h>
#include <Wire.h>
#include <DS3231.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

namespace pin
{
  const byte tds_sensor = 34;
  const byte ph_sensor = 35;
  const byte soil_sensor = 32;
  const byte dht21_sensor = 19;
  const byte one_wire_bus = 17;
  const byte led_builtin = 2;
  const byte relay1 = 16;
  const byte relay2 = 4;
  const byte relay3 = 2;
  const byte relay4 = 15;
}

namespace device
{
  float aref = 3.3;
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
DHT dht(pin::dht21_sensor, DHT21);
RTClib rtc;
DS3231 t;

char cssid[50]; // = "Technometric2";
char cpswd[50]; // = "12345678";
char cip[30];   // = "192.168.0.255";
char device_id[8];
int rdloop = 0;
char json[128] = "\0";
char devId[8];
// Are we currently connected?
boolean connected = false;
boolean reconnect;
// The udp library class
WiFiUDP udp;
int node = 0;
int def;
String host;
String dev_id;
int remote_port;
int localport;
String ssid;
String pswd;

int eeAddr = 0;
char packetBuffer[512];
char ReplyBuffer[] = "acknowledged";
String inputString = "";     // a String to hold incoming data
bool stringComplete = false; // whether the string is complete
void (*resetFunc)(void) = 0;
int StringToCharArray(String, char *);
void EEPROM_default();
void EEPROM_put(String);
double getTemperature();
void EEPROM_putJson(char *);
String IpAddress2String(const IPAddress &ipAddress);
void WiFiEvent(WiFiEvent_t event);
void connectToWiFi(const char *, const char *);
void EEPROM_get();
int EEPROM_getOutput();
void EEPROM_putOutput(int ot);
void readTdsQuick();
void getSoilPercent();

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
  dht.begin();

  if (!EEPROM.begin(1024))
  {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  EEPROM_get();
  if (def != 0)
  {
    EEPROM_default();
    EEPROM_putOutput(0);
  }

  SerialBT.begin("green-house-bt"); // Bluetooth device name
  Serial.println("esp32gh started");

  StringToCharArray(ssid, cssid);
  StringToCharArray(pswd, cpswd);
  connectToWiFi(cssid, cpswd);

  pinMode(pin::relay1, OUTPUT);
  pinMode(pin::relay2, OUTPUT);
  pinMode(pin::relay3, OUTPUT);
  pinMode(pin::relay4, OUTPUT);

  digitalWrite(pin::relay1, LOW);
  digitalWrite(pin::relay2, LOW);
  digitalWrite(pin::relay3, LOW);
  digitalWrite(pin::relay4, LOW);

  delay(500);
  Serial.println("esp32 Ready!");
  /*
  t.setYear(23);
  t.setMonth(2);
  t.setDate(8);
  t.setDoW(3);
  t.setHour(22);
  t.setMinute(43);
  t.setSecond(0);
  */
}
float ph(float voltage)
{
  return 7 + ((1.65 - voltage) / 0.18);
}

int samples = 10;
float adc_resolution = 4096.0;
void loop()
{
  int measurings = 0;
  for (int i = 0; i < samples; i++)
  {
    measurings += analogRead(pin::ph_sensor);
    delay(10);
  }

  float voltage = 5 / adc_resolution * measurings / samples;
  Serial.println(ph(voltage));
  readTdsQuick();
  /*
  dht.read();
  // проверяем состояние данных
  switch(dht.getState()) {
    // всё OK
    case DHT_OK:
      // выводим показания влажности и температуры
      Serial.print("Temperature = ");
      Serial.print(dht.getTemperatureC());
      Serial.println(" C \t");
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
*/
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));

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

  delay(1000);
}

void readTdsQuick()
{
  // dallasTemp.requestTemperatures();
  sensor::waterTemp = 25; // dallasTemp.getTempCByIndex(0);
  float rawEc = analogRead(pin::tds_sensor) * device::aref / 4096;
  float tempCoefficient = 1.0 + 0.02 * (sensor::waterTemp - 25.0);
  sensor::ec = (rawEc / tempCoefficient) * sensor::ecCalibration;
  sensor::tds = (113.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec * 857.39 * sensor::ec) * 0.5;
  // tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
  Serial.println(sensor::ec);
  Serial.println(sensor::tds);
}

void getSoilPercent()
{
  sensor::smvalue = analogRead(pin::soil_sensor);
  sensor::smpercent = map(sensor::smvalue, 0, 1023, 100, 0);
}

void connectToWiFi(const char *ssid, const char *pwd)
{
  Serial.println("Connecting to WiFi network: " + String(ssid) + " - " + String(pwd));

  // delete old config
  WiFi.disconnect(true);
  // register event handler
  WiFi.onEvent(WiFiEvent);

  // Initiate connection
  WiFi.begin(ssid, pwd);

  Serial.println("Waiting for WIFI connection...");
}

// wifi event handler
void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
  {
    // When connected set
    Serial.print("WiFi connected! IP address: ");
    String ip = IpAddress2String(WiFi.localIP());
    Serial.println(ip);
    StringToCharArray(ip, cip);
    // initializes the UDP state
    // This initializes the transfer buffer
    udp.begin(WiFi.localIP(), localport);
    connected = true;
    reconnect = true;
  }
  break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    connected = false;
    break;
  }
}

int StringToCharArray(String str, char *s)
{
  int i;
  for (i = 0; i < str.length(); i++)
  {
    *s++ = str.charAt(i);
  }
  *s++ = '\0';
  return i;
}

void EEPROM_default()
{
  def = 0;
  eeAddr = 0;
  EEPROM.writeInt(eeAddr, def);

  // host = "192.168.1.255";
  dev_id = "A001";
  eeAddr = 4; // sizeof(def);
  EEPROM.writeString(eeAddr, dev_id);

  remote_port = 8899;
  eeAddr = 20; // sizeof(host);
  EEPROM.writeInt(eeAddr, remote_port);

  localport = 8888;
  eeAddr = 24; // sizeof(host);
  EEPROM.writeInt(eeAddr, localport);

  ssid = "Technometric";
  eeAddr = 28; // sizeof(port);
  EEPROM.writeString(eeAddr, ssid);

  pswd = "87880138";
  eeAddr = 78; // sizeof(ssid);
  EEPROM.writeString(eeAddr, pswd);
  EEPROM.commit();
}
void EEPROM_put(String dev)
{

  eeAddr = 4; // sizeof(def);
  if (!dev.equals(""))
    EEPROM.writeString(eeAddr, dev);

  eeAddr = 20; // sizeof(host);
  EEPROM.writeInt(eeAddr, remote_port);

  eeAddr = 24; // sizeof(host);
  EEPROM.writeInt(eeAddr, localport);

  eeAddr = 28; // sizeof(port);
  EEPROM.writeString(eeAddr, ssid);

  eeAddr = 78; // sizeof(ssid);
  EEPROM.writeString(eeAddr, pswd);
  EEPROM.commit();
}

void EEPROM_get()
{
  eeAddr = 0;
  def = EEPROM.readInt(eeAddr);

  eeAddr = 4; // sizeof(def);
  dev_id = EEPROM.readString(eeAddr);

  eeAddr = 20; // sizeof(host);
  remote_port = EEPROM.readInt(eeAddr);

  eeAddr = 24; // sizeof(host);
  localport = EEPROM.readInt(eeAddr);

  eeAddr = 28; // sizeof(port);
  ssid = EEPROM.readString(eeAddr);

  eeAddr = 78; // sizeof(ssid);
  pswd = EEPROM.readString(eeAddr);
}

int EEPROM_getOutput()
{
  return EEPROM.readInt(128);
}
void EEPROM_putOutput(int ot)
{
  EEPROM.writeInt(128, ot);
  EEPROM.commit();
}

void EEPROM_putJson(char *json)
{
  eeAddr = 128; // sizeof(ssid);
  EEPROM.writeString(eeAddr, json);
  EEPROM.commit();
}
void EEPROM_getJson(char *json)
{
  eeAddr = 128;
  EEPROM.readString(eeAddr, json, 128);
}

void serialEvent()
{
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // Serial.print(inChar);
    //  add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\r')
    {
      stringComplete = true;
    }
  }
}

void serialBTEvent()
{
  while (SerialBT.available())
  {
    // get the new byte:
    char inChar = (char)SerialBT.read();
    // Serial.print(inChar);
    //  add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\r')
    {
      stringComplete = true;
    }
  }
}

String IpAddress2String(const IPAddress &ipAddress)
{
  return String(ipAddress[0]) + String(".") +
         String(ipAddress[1]) + String(".") +
         String(ipAddress[2]) + String(".") +
         String(ipAddress[3]);
}