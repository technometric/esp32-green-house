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

namespace param_timer
{
  String timer1_on = "06.00";
  String timer2_on = "07.00";
  String timer3_on = "08.00";
  String timer4_on = "09.00";
  String timer1_off = "06.10";
  String timer2_off = "07.10";
  String timer3_off = "08.10";
  String timer4_off = "09.10";
}

namespace param_limit
{
  int timer1_on = 0, timer1_off = 0;
  int timer2_on = 0, timer2_off = 0;
  int timer3_on = 0, timer3_off = 0;
  int timer4_on = 0, timer4_off = 0;
  int timer1_en = 0;
  int timer2_en = 0;
  int timer3_en = 0;
  int timer4_en = 0;
  int temp_on = 25, temp_off = 30;
  int soil_on = 20, soil_off = 50;
  float ec_on = 1.0, ec_off = 2.0;
  float tds_on = 100, tds_off = 200;
  float ph_on = 6.0, ph_off = 7.0;
}

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
int output = 0;
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
int timer_now = 0;
void loop()
{
  int moisturePercentage = (100.00 - ((analogRead(pin::soil_sensor) / 1023.00) * 100.00));
  Serial.print("Kelembaban Tanah: ");
  Serial.println(moisturePercentage);

  int measurings = 0;
  for (int i = 0; i < samples; i++)
  {
    measurings += analogRead(pin::ph_sensor);
    delay(10);
  }

  float voltage = 3.3 / adc_resolution * measurings / samples;
  Serial.print("pH: ");
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

  if (param_limit::timer1_en == 1)
  {
    if (param_limit::timer1_on == timer_now)
    {
      digitalWrite(pin::relay1, HIGH);
    }
    if (param_limit::timer1_off == timer_now)
    {
      digitalWrite(pin::relay1, LOW);
    }
  }

  if (param_limit::timer2_en == 1)
  {
    if (param_limit::timer2_on == timer_now)
    {
      digitalWrite(pin::relay1, HIGH);
    }
    if (param_limit::timer2_off == timer_now)
    {
      digitalWrite(pin::relay1, LOW);
    }
  }

  if (param_limit::timer3_en == 1)
  {
    if (param_limit::timer3_on == timer_now)
    {
      digitalWrite(pin::relay1, HIGH);
    }
    if (param_limit::timer2_off == timer_now)
    {
      digitalWrite(pin::relay1, LOW);
    }
  }

  if (param_limit::timer4_en == 1)
  {
    if (param_limit::timer4_on == timer_now)
    {
      digitalWrite(pin::relay1, HIGH);
    }
    if (param_limit::timer4_off == timer_now)
    {
      digitalWrite(pin::relay1, LOW);
    }
  }

  delay(1000);
}

void readTdsQuick()
{
  // dallasTemp.requestTemperatures();
  sensor::waterTemp = 25.0; // dallasTemp.getTempCByIndex(0);
  float rawEc = analogRead(pin::tds_sensor) * device::aref / 4096.0;
  Serial.print("rawEC: ");
  Serial.println(rawEc);
  float tempCoefficient = 1.0 + 0.02 * (sensor::waterTemp - 25.0);
  sensor::ec = (rawEc / tempCoefficient) * sensor::ecCalibration;
  sensor::tds = (113.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec * 857.39 * sensor::ec) * 0.5;
  // tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
  Serial.print("EC: ");
  Serial.println(sensor::ec);
  Serial.print("TDS: ");
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

  eeAddr = 512;
  EEPROM.writeString(eeAddr, param_timer::timer1_on);

  eeAddr = 520;
  EEPROM.writeString(eeAddr, param_timer::timer2_on);

  eeAddr = 528;
  EEPROM.writeString(eeAddr, param_timer::timer3_on);

  eeAddr = 536;
  EEPROM.writeString(eeAddr, param_timer::timer4_on);

  eeAddr = 554;
  EEPROM.writeString(eeAddr, param_timer::timer1_off);

  eeAddr = 562;
  EEPROM.writeString(eeAddr, param_timer::timer2_off);

  eeAddr = 570;
  EEPROM.writeString(eeAddr, param_timer::timer3_off);

  eeAddr = 578;
  EEPROM.writeString(eeAddr, param_timer::timer4_off);

  eeAddr = 586;
  EEPROM.writeInt(eeAddr, param_limit::timer1_en);

  eeAddr = 588;
  EEPROM.writeInt(eeAddr, param_limit::timer2_en);

  eeAddr = 590;
  EEPROM.writeInt(eeAddr, param_limit::timer3_en);

  eeAddr = 592;
  EEPROM.writeInt(eeAddr, param_limit::timer4_en);

  eeAddr = 594;
  EEPROM.writeInt(eeAddr, param_limit::temp_on);
  eeAddr = 596;
  EEPROM.writeInt(eeAddr, param_limit::temp_off);
  eeAddr = 598;
  EEPROM.writeInt(eeAddr, param_limit::soil_on);
  eeAddr = 600;
  EEPROM.writeInt(eeAddr, param_limit::soil_off);
  eeAddr = 602;
  EEPROM.writeFloat(eeAddr, param_limit::ec_on);
  eeAddr = 604;
  EEPROM.writeFloat(eeAddr, param_limit::ec_off);
  eeAddr = 606;
  EEPROM.writeInt(eeAddr, param_limit::tds_on);
  eeAddr = 608;
  EEPROM.writeInt(eeAddr, param_limit::tds_off);
  eeAddr = 610;
  EEPROM.writeFloat(eeAddr, param_limit::ph_on);
  eeAddr = 612;
  EEPROM.writeFloat(eeAddr, param_limit::ph_off);
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

  eeAddr = 512;
  EEPROM.writeString(eeAddr, param_timer::timer1_on);

  eeAddr = 520;
  EEPROM.writeString(eeAddr, param_timer::timer2_on);

  eeAddr = 528;
  EEPROM.writeString(eeAddr, param_timer::timer3_on);

  eeAddr = 536;
  EEPROM.writeString(eeAddr, param_timer::timer4_on);

  eeAddr = 554;
  EEPROM.writeString(eeAddr, param_timer::timer1_off);

  eeAddr = 562;
  EEPROM.writeString(eeAddr, param_timer::timer2_off);

  eeAddr = 570;
  EEPROM.writeString(eeAddr, param_timer::timer3_off);

  eeAddr = 578;
  EEPROM.writeString(eeAddr, param_timer::timer4_off);

  eeAddr = 586;
  EEPROM.writeInt(eeAddr, param_limit::timer1_en);

  eeAddr = 588;
  EEPROM.writeInt(eeAddr, param_limit::timer2_en);

  eeAddr = 590;
  EEPROM.writeInt(eeAddr, param_limit::timer3_en);

  eeAddr = 592;
  EEPROM.writeInt(eeAddr, param_limit::timer4_en);

  eeAddr = 594;
  EEPROM.writeInt(eeAddr, param_limit::temp_on);
  eeAddr = 596;
  EEPROM.writeInt(eeAddr, param_limit::temp_off);
  eeAddr = 598;
  EEPROM.writeInt(eeAddr, param_limit::soil_on);
  eeAddr = 600;
  EEPROM.writeInt(eeAddr, param_limit::soil_off);
  eeAddr = 602;
  EEPROM.writeFloat(eeAddr, param_limit::ec_on);
  eeAddr = 604;
  EEPROM.writeFloat(eeAddr, param_limit::ec_off);
  eeAddr = 606;
  EEPROM.writeInt(eeAddr, param_limit::tds_on);
  eeAddr = 608;
  EEPROM.writeInt(eeAddr, param_limit::tds_off);
  eeAddr = 610;
  EEPROM.writeFloat(eeAddr, param_limit::ph_on);
  eeAddr = 612;
  EEPROM.writeFloat(eeAddr, param_limit::ph_off);
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

  eeAddr = 512;
  param_timer::timer1_on = EEPROM.readString(eeAddr);

  eeAddr = 520;
  param_timer::timer2_on = EEPROM.readString(eeAddr);

  eeAddr = 528;
  param_timer::timer3_on = EEPROM.readString(eeAddr);

  eeAddr = 536;
  param_timer::timer4_on = EEPROM.readString(eeAddr);

  eeAddr = 554;
  param_timer::timer1_off = EEPROM.readString(eeAddr);

  eeAddr = 562;
  param_timer::timer2_off = EEPROM.readString(eeAddr);

  eeAddr = 570;
  param_timer::timer3_off = EEPROM.readString(eeAddr);

  eeAddr = 578;
  param_timer::timer4_off = EEPROM.readString(eeAddr);

  eeAddr = 586;
  param_limit::timer1_en = EEPROM.readInt(eeAddr);

  eeAddr = 588;
  param_limit::timer2_en = EEPROM.readInt(eeAddr);

  eeAddr = 590;
  param_limit::timer3_en = EEPROM.readInt(eeAddr);

  eeAddr = 592;
  param_limit::timer4_en = EEPROM.readInt(eeAddr);

  eeAddr = 594;
  param_limit::temp_on = EEPROM.readInt(eeAddr);
  eeAddr = 596;
  param_limit::temp_off = EEPROM.readInt(eeAddr);
  eeAddr = 598;
  param_limit::soil_on = EEPROM.readInt(eeAddr);
  eeAddr = 600;
  param_limit::soil_off = EEPROM.readInt(eeAddr);
  eeAddr = 602;
  param_limit::ec_on = EEPROM.readFloat(eeAddr);
  eeAddr = 604;
  param_limit::ec_off = EEPROM.readFloat(eeAddr);
  eeAddr = 606;
  param_limit::tds_on = EEPROM.readInt(eeAddr);
  eeAddr = 608;
  param_limit::tds_off = EEPROM.readInt(eeAddr);
  eeAddr = 610;
  param_limit::ph_on = EEPROM.readFloat(eeAddr);
  eeAddr = 612;
  param_limit::ph_off = EEPROM.readFloat(eeAddr);
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

void pharseJsonSerialIn(String jsonStr)
{
  StaticJsonBuffer<200> jsonBuffer;
  char json[128]; // = "{\"cmd\":\"setSSID\",\"device_id\":\"001\",\"ssid\":\"Technometric2\",\"pswd\":\"windi09dhika07\",\"localPort\":8888,\"remotePort\":8899}";
  StringToCharArray(jsonStr, json);

  JsonObject &root = jsonBuffer.parseObject(json);
  if (!root.success())
  {
    // Serial.println("parseObject() failed");
    Serial.printf("{\"Status\":1,\"message\":\"JSON pharsing error\"");
    SerialBT.printf("{\"Status\":1,\"message\":\"JSON pharsing error\"");
    return;
  }
  String cmd = root["cmd"];
  String dev = root["device_id"];
  StringToCharArray(dev, devId);

  if (cmd.equals("setConfig"))
  {
    ssid = root["ssid"].as<String>();
    pswd = root["pswd"].as<String>();
    localport = root["portIn"];
    remote_port = root["portOut"];
    EEPROM_put("");
    connected = false;
    StringToCharArray(ssid, cssid);
    StringToCharArray(pswd, cpswd);
    connectToWiFi(cssid, cpswd);
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", dev);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", dev);
    delay(1000);
  }
  else if (cmd.equals("setSSID"))
  {
    String dev = root["device_id"];

    ssid = root["ssid"].as<String>();
    pswd = root["pswd"].as<String>();
    EEPROM_put("");
    connected = false;
    StringToCharArray(ssid, cssid);
    StringToCharArray(pswd, cpswd);
    connectToWiFi(cssid, cpswd);
    Serial.printf("{\"Status\":0,\"message\":\"Reconnect to network\"");
    SerialBT.printf("{\"Status\":0,\"message\":\"Reconnect to network\"");
    delay(1000);
    EEPROM_get();
  }
  else if (cmd.equals("setTimer1On"))
  {
    param_timer::timer1_on = root["on"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimer2On"))
  {
    param_timer::timer2_on = root["on"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimer3On"))
  {
    param_timer::timer3_on = root["on"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimer4On"))
  {
    param_timer::timer4_on = root["on"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimer1Off"))
  {
    param_timer::timer1_off = root["off"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimer2Off"))
  {
    param_timer::timer2_off = root["off"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimer3Off"))
  {
    param_timer::timer3_off = root["off"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimer4Off"))
  {
    param_timer::timer4_off = root["off"].as<String>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setTimerEnable"))
  {
    param_limit::timer1_en = root["timer1_en"];
    param_limit::timer2_en = root["timer2_en"];
    param_limit::timer3_en = root["timer3_en"];
    param_limit::timer4_en = root["timer4_en"];
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setParamLimit"))
  {
    param_limit::temp_on = root["temp_on"];
    param_limit::temp_off = root["temp_off"];
    param_limit::soil_on = root["soil_on"];
    param_limit::soil_off = root["soil_off"];
    param_limit::ec_on = root["ec_on"].as<float>();
    param_limit::ec_off = root["ec_off"].as<float>();
    param_limit::tds_on = root["tds_on"];
    param_limit::tds_off = root["tds_off"];
    param_limit::ph_on = root["ph_on"].as<float>();
    param_limit::ph_off = root["ph_off"].as<float>();
    EEPROM_put("");
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\"", device_id);
    delay(1000);
  }
  else if (cmd.equals("setDevice"))
  {
    localport = root["portIn"];
    remote_port = root["portOut"];
    EEPROM_put(dev);
    delay(5000);
    EEPROM_get();
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"", dev_id);
  }
  else if (cmd.equals("getConfig"))
  {
    StringToCharArray(dev_id, device_id);
    SerialBT.printf("{\"Status\":\"getConfig\",\"device_id\":\"%s\",\"ssid\":\"%s\",\"pswd\":\"%s\",\"localIp\":\"%s\",\"portIn\":%d,\"portOut\":%d}\n", device_id, cssid, cpswd, cip, localport, remote_port);
    Serial.printf("{\"Status\":\"getConfig\",\"device_id\":\"%s\",\"ssid\":\"%s\",\"pswd\":\"%s\",\"localIp\":\"%s\",\"portIn\":%d,\"portOut\":%d}\n", device_id, cssid, cpswd, cip, localport, remote_port);
  }
  else if (cmd.equals("getAll"))
  {
    int ot1 = digitalRead(pin::relay1);
    int ot2 = digitalRead(pin::relay2);
    int ot3 = digitalRead(pin::relay3);
    int ot4 = digitalRead(pin::relay4);
    SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\",\"Data\":{\"ph\":%.2f,\"soil\":%d,\"tds\":%d,\"ec\":%.2f,\"temp\":%.2f,\"ot1\":%d,\"ot2\":%d,\"ot3\":%d,\"ot4\":%d}}", devId, node, 6.9, 60, 120, 1.3, 28.2, ot1, ot2, ot3, ot4);
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\",\"Data\":{\"ph\":%.2f,\"soil\":%d,\"tds\":%d,\"ec\":%.2f,\"temp\":%.2f,\"ot1\":%d,\"ot2\":%d,\"ot3\":%d,\"ot4\":%d}}", devId, node, 6.9, 60, 120, 1.3, 28.2, ot1, ot2, ot3, ot4);
    rdloop = 0;
  }
  else if (cmd.equals("rdLoop"))
  {
    //{"cmd":"rdLoop","device_id":"01","delay":1}
    int dly = root["delay"];
    rdloop = dly;
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"}\r\n", dev);
  }
  else if (cmd.equals("setRelay1"))
  {
    //{"cmd":"setRelay2","device_id":"A001","state":"1"}
    int ot = (root["state"]) > 0 ? 1 : 0;
    digitalWrite(pin::relay1, ot);
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"}\r\n", dev);
    output &= 0xFE;
    output |= ot;
    EEPROM_putOutput(output);
  }
  else if (cmd.equals("setRelay2"))
  {
    //{"cmd":"setRelay2","device_id":"01","state":"0"}
    int ot = (root["state"]) > 0 ? 1 : 0;
    digitalWrite(pin::relay2, ot);
    output &= 0xFD;
    output |= (ot << 1);
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"}\r\n", dev);
    EEPROM_putOutput(output);
  }
  else if (cmd.equals("setRelay3"))
  {
    //{"cmd":"setRelay3","device_id":"01","state":"0"}
    int ot = (root["state"]) > 0 ? 1 : 0;
    digitalWrite(pin::relay3, ot);
    output &= 0xFB;
    output |= (ot << 2);
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"}\r\n", dev);
    EEPROM_putOutput(output);
  }
  else if (cmd.equals("setRelay4"))
  {
    //{"cmd":"setRelay4","device_id":"01","state":"0"}
    int ot = (root["state"]) > 0 ? 1 : 0;
    digitalWrite(pin::relay4, ot);
    output &= 0xF7;
    output |= (ot << 3);
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\"}\r\n", dev);
    EEPROM_putOutput(output);
  }

  else if (cmd.equals("getTimerParam"))
  {
    char tmr1_on[8];
    char tmr2_on[8];
    char tmr3_on[8];
    char tmr4_on[8];
    char tmr1_off[8];
    char tmr2_off[8];
    char tmr3_off[8];
    char tmr4_off[8];
    StringToCharArray(param_timer::timer1_on, tmr1_on);
    StringToCharArray(param_timer::timer2_on, tmr2_on);
    StringToCharArray(param_timer::timer3_on, tmr3_on);
    StringToCharArray(param_timer::timer4_on, tmr4_on);
    StringToCharArray(param_timer::timer1_off, tmr1_off);
    StringToCharArray(param_timer::timer2_off, tmr2_off);
    StringToCharArray(param_timer::timer3_off, tmr3_off);
    StringToCharArray(param_timer::timer4_off, tmr4_off);

    Serial.printf("{\"Status\":0,\"device_id\":\"%s\",\"timer1_on\":%s,\"timer2_on\":%s,\"timer3_on\":%s,\"timer4_on\":%s,"
                  "\"timer1_off\":%s,\"timer2_off\":%s,\"timer3_off\":%s,\"timer4_off\":%s}\r\n",
                  device_id, tmr1_on, tmr2_on, tmr3_on, tmr4_on, tmr1_off, tmr2_off, tmr3_off, tmr4_off);
  }
  else if (cmd.equals("getTimerState"))
  {
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\",\"timer1_en\":%d,\"timer2_en\":%d,\"timer3_en\":%d,\"timer4_en\":%d,}\r\n",
                  device_id, param_limit::timer1_en, param_limit::timer2_en, param_limit::timer3_en, param_limit::timer4_en);
  }
  else if (cmd.equals("getLimitParam"))
  {
    Serial.printf("{\"Status\":0,\"device_id\":\"%s\",\"temp_on\":%d,\"temp_off\":%d,\"soil_on\":%d,\"soil_off\":%d,"
                  "\"ec_on\":%.2f,\"soil_off\":%.2f,\"tds_on\":%d,\"tds_off\":%d,\"ph_on\":%.2f\"ph_off\":%.2f}\r\n",
                  device_id, param_limit::temp_on, param_limit::temp_off, param_limit::soil_on, param_limit::soil_off, param_limit::ec_on, param_limit::ec_off,
                  param_limit::tds_on, param_limit::tds_off, param_limit::ph_on, param_limit::ph_off);
  }

  else if (cmd.equals("forceReset"))
  {
    Serial.printf("{\"Status\":\"Device force reset\",\"device_id\":\"%s\"}\r\n", dev);
    delay(1000);
    ESP.restart();
  }
}