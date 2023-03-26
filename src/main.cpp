#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <DS3231.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>
#include "udpEvent.h"
#include "serialEvent.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

namespace device
{
  float aref = 3.3;
} // namespace device

namespace param_limit
{
  int output_en = 0;
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
namespace param_timer
{
  int timer1_on = 0;
  int timer2_on = 0;
  int timer3_on = 0;
  int timer4_on = 0;
  int timer1_off = 0;
  int timer2_off = 0;
  int timer3_off = 0;
  int timer4_off = 0;
}
namespace pin
{
  int tds_sensor = 35;
  int ph_sensor = 34;
  int soil_sensor = 32;
  int dht21_sensor = 19;
  int one_wire_bus = 17;
  int led_builtin = 2;
  int relay1 = 16;
  int relay2 = 4;
  int relay3 = 5;
  int relay4 = 15;
}

namespace sensor
{
  float ec;
  int tds;
  float waterTemp;
  float ecCalibration;
  int smvalue;
  int smpercent;
  float suhu_udara;
  float kelembaban;
  float ph;
}

OneWire oneWire(pin::one_wire_bus);
DallasTemperature dallasTemp(&oneWire);
DHT dht(pin::dht21_sensor, DHT21);
RTClib rtc;
DS3231 t;

float pH7 = 1.65;
float pH4 = 0;
char cssid[50]; // = "Technometric2";
char cpswd[50]; // = "12345678";
char cip[30];   // = "192.168.0.255";
char device_id[8];
int rdloop = 0;
char json[128] = "\0";
char devId[8];
int ot1, ot2, ot3, ot4;
int output = 0;
StaticJsonBuffer<200> jsonBuffer;
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
// int StringToCharArray(String, char *);
void EEPROM_default();
void EEPROM_put(String);
double getTemperature();
void EEPROM_putJson(char *);
String IpAddress2String(const IPAddress &ipAddress);
void WiFiEvent(WiFiEvent_t event);
void connectToWiFi(char *, char *);
void EEPROM_get();
int EEPROM_getOutput();
void EEPROM_putOutput(int ot);
void readTdsQuick();
void getSoilPercent();
String parseJsonSerialBTIn(String jsonStr);

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

  pinMode(pin::led_builtin, OUTPUT);
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
  // float ph_step = (pH4 - pH7) / 3;
  return -5.70 * voltage + 21.34;
}

int samples = 10;
float adc_resolution = 4095.0;
int timer_now = 0;
int dly = 0;
void loop()
{
  if (!connected)
  {
    if (reconnect)
    {
      reconnect = false;
      connectToWiFi(cssid, cpswd);
    }
    digitalWrite(pin::led_builtin, HIGH);
    delay(100);
    digitalWrite(pin::led_builtin, LOW);
    delay(100);
  }
  else
  {
    digitalWrite(pin::led_builtin, HIGH);
  }
  int packetSize = udp.parsePacket();
  if (packetSize)
  {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remoteIp = udp.remoteIP();
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(udp.remotePort());

    // read the packet into packetBufffer
    int len = udp.read(packetBuffer, 512);
    if (len > 0)
    {
      packetBuffer[len] = 0;
    }
    parseJsonUdpIn(devId, connected, rdloop, remote_port, packetBuffer, EEPROM_put);
  }
  getSoilPercent();
  // int moisturePercentage = (100.00 - ((analogRead(pin::soil_sensor) / 1023.00) * 100.00));
  // sensor::smpercent = map(analogRead(pin::soil_sensor), 0, 4095, 100, 0);
  // Serial.print("Kelembaban Tanah: ");
  // Serial.println(moisturePercentage);

  int measurings = 0;
  for (int i = 0; i < samples; i++)
  {
    measurings += analogRead(pin::ph_sensor);
    delay(20);
  }

  float voltage = (3.3 / adc_resolution) * (measurings / samples);
  sensor::ph = ph(voltage);
  // Serial.print("pH: ");
  // Serial.println(ph(voltage));
  readTdsQuick();

  sensor::kelembaban = dht.readHumidity();
  // Read temperature as Celsius (the default)
  sensor::suhu_udara = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(sensor::kelembaban) || isnan(sensor::suhu_udara))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  /*
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
  */
  DateTime now = rtc.now();
  Serial.printf("%02d/%02d/%4d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
  Serial.println();

  timer_now = (now.hour() * 60) + now.minute();
  if (param_limit::output_en == 1)
  {
    if (param_limit::timer1_en == 1)
    {
      if (param_timer::timer1_on == timer_now)
      {
        digitalWrite(pin::relay1, HIGH);
      }
      if (param_timer::timer1_off == timer_now)
      {
        digitalWrite(pin::relay1, LOW);
      }
    }

    if (param_limit::timer2_en == 1)
    {
      if (param_timer::timer2_on == timer_now)
      {
        digitalWrite(pin::relay1, HIGH);
      }
      if (param_timer::timer2_off == timer_now)
      {
        digitalWrite(pin::relay1, LOW);
      }
    }

    if (param_limit::timer3_en == 1)
    {
      if (param_timer::timer3_on == timer_now)
      {
        digitalWrite(pin::relay1, HIGH);
      }
      if (param_timer::timer2_off == timer_now)
      {
        digitalWrite(pin::relay1, LOW);
      }
    }

    if (param_limit::timer4_en == 1)
    {
      if (param_timer::timer4_on == timer_now)
      {
        digitalWrite(pin::relay1, HIGH);
      }
      if (param_timer::timer4_off == timer_now)
      {
        digitalWrite(pin::relay1, LOW);
      }
    }

    if (sensor::suhu_udara >= param_limit::temp_on)
    {
      digitalWrite(pin::relay2, HIGH);
    }
    if (sensor::suhu_udara <= param_limit::temp_off)
    {
      digitalWrite(pin::relay2, LOW);
    }

    if (sensor::smpercent == param_limit::soil_on)
    {
      digitalWrite(pin::relay3, HIGH);
    }
    if (sensor::smpercent == param_limit::soil_off)
    {
      digitalWrite(pin::relay3, LOW);
    }

    if (sensor::ec == param_limit::ec_on)
    {
      digitalWrite(pin::relay4, HIGH);
    }
    if (sensor::ec == param_limit::ec_off)
    {
      digitalWrite(pin::relay4, LOW);
    }
  }
  else
  {
    digitalWrite(pin::relay1, LOW);
    digitalWrite(pin::relay2, LOW);
    digitalWrite(pin::relay3, LOW);
    digitalWrite(pin::relay4, LOW);
  }
  if (rdloop > 0)
  {
    if (++dly >= rdloop)
    {
      dly = 0;
      if (connected)
      {
        ot1 = digitalRead(pin::relay1);
        ot2 = digitalRead(pin::relay2);
        ot3 = digitalRead(pin::relay3);
        ot4 = digitalRead(pin::relay4);
        StringToCharArray(dev_id, devId);
        udp.beginPacket(udp.remoteIP(), remote_port);
        udp.printf("{\"Status\":0,\"device_id\":\"%s\",\"Data\":{\"ph\":%.2f,\"soil\":%d,\"tds\":%d,\"ec\":%.2f,\"temp\":%.2f,\"ot1\":%d,\"ot2\":%d,\"ot3\":%d,\"ot4\":%d}}", devId, node, sensor::ph, sensor::smpercent, sensor::tds, sensor::ec, sensor::suhu_udara, ot1, ot2, ot3, ot4);
        udp.endPacket();
        SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\",\"Data\":{\"ph\":%.2f,\"soil\":%d,\"tds\":%d,\"ec\":%.2f,\"temp\":%.2f,\"ot1\":%d,\"ot2\":%d,\"ot3\":%d,\"ot4\":%d}}", devId, node, sensor::ph, sensor::smpercent, sensor::tds, sensor::ec, sensor::suhu_udara, ot1, ot2, ot3, ot4);
      }
    }
  }
  // Serial.printf("{\"Status\":0,\"device_id\":\"%s\",\"Data\":{\"ph\":%.2f,\"soil\":%d,\"tds\":%d,\"ec\":%.2f,\"temp\":%.2f,\"ot1\":%d,\"ot2\":%d,\"ot3\":%d,\"ot4\":%d}}", devId, node, sensor::ph, sensor::smpercent, sensor::tds, sensor::ec, sensor::suhu_udara, ot1, ot2, ot3, ot4);
  // Serial.println();

  if (stringComplete)
  {
    Serial.println(inputString);
    parseJsonSerialIn(devId, &rdloop, inputString, EEPROM_put, EEPROM_get, connectToWiFi);
    // parseJsonSerialBTIn(inputString);
    inputString = "";
    stringComplete = false;
  }
  while (SerialBT.available())
  {
    // get the new byte:
    char inChar = (char)SerialBT.read();
    // Serial.print(inChar);
    inputString += inChar;
    if (inChar == '\r')
    {
      stringComplete = true;
    }
  }

  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // Serial.print(inChar);
    inputString += inChar;
    if (inChar == '\r')
    {
      stringComplete = true;
    }
  }
  delay(1000);
}

void readTdsQuick()
{
  // dallasTemp.requestTemperatures();
  sensor::waterTemp = 25.0; // dallasTemp.getTempCByIndex(0);
  float rawEc = (analogRead(pin::tds_sensor) * device::aref) / adc_resolution;
  // Serial.print("rawEC: ");
  // Serial.println(rawEc);
  float tempCoefficient = 1.0 + 0.02 * (sensor::waterTemp - 25.0);
  sensor::ec = (rawEc / tempCoefficient) * sensor::ecCalibration;
  sensor::tds = (113.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * 0.5;
  // tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
  // Serial.print("EC: ");
  // Serial.println(sensor::ec);
  // Serial.print("TDS: ");
  // Serial.println(sensor::tds);
}

/*
void readTdsQuick() {
  dallasTemperature.requestTemperatures();
  sensor::waterTemp = dallasTemperature.getTempCByIndex(0);
  float rawEc = analogRead(pin::tds_sensor) * device::aref / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
  float temperatureCoefficient = 1.0 + 0.02 * (sensor::waterTemp - 25.0); // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  sensor::ec = (rawEc / temperatureCoefficient) * sensor::ecCalibration; // temperature and calibration compensation
  sensor::tds = (133.42 * pow(sensor::ec, 3) - 255.86 * sensor::ec * sensor::ec + 857.39 * sensor::ec) * 0.5; //convert voltage value to tds value
  Serial.print(F("TDS:")); Serial.println(sensor::tds);
  Serial.print(F("EC:")); Serial.println(sensor::ec, 2);
  Serial.print(F("Temperature:")); Serial.println(sensor::waterTemp,2);

 display.clearDisplay();
  display.setCursor(10,0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
 display.print("TDS:"+String(sensor::tds));
    display.setCursor(10,20);
  display.setTextSize(2);
 display.print("EC:"+String(sensor::ec, 2));
   display.setCursor(10,45);
  display.setTextSize(2);
 display.print("T:"+String(sensor::waterTemp,2));
  display.display();
    Blynk.virtualWrite(V0,(sensor::tds));

   Blynk.virtualWrite(V1,(sensor::ec));

     Blynk.virtualWrite(V2,(sensor::waterTemp));
}
*/

void getSoilPercent()
{
  sensor::smvalue = analogRead(pin::soil_sensor);
  sensor::smpercent = map(sensor::smvalue, 0, 4095, 100, 0);
}

void connectToWiFi(char *ssid, char *pwd)
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

/*int StringToCharArray(String str, char *s)
{
  int i;
  for (i = 0; i < str.length(); i++)
  {
    *s++ = str.charAt(i);
  }
  *s++ = '\0';
  return i;
}*/

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
  EEPROM.writeInt(eeAddr, param_timer::timer1_on);

  eeAddr = 520;
  EEPROM.writeInt(eeAddr, param_timer::timer2_on);

  eeAddr = 528;
  EEPROM.writeInt(eeAddr, param_timer::timer3_on);

  eeAddr = 536;
  EEPROM.writeInt(eeAddr, param_timer::timer4_on);

  eeAddr = 554;
  EEPROM.writeInt(eeAddr, param_timer::timer1_off);

  eeAddr = 562;
  EEPROM.writeInt(eeAddr, param_timer::timer2_off);

  eeAddr = 570;
  EEPROM.writeInt(eeAddr, param_timer::timer3_off);

  eeAddr = 578;
  EEPROM.writeInt(eeAddr, param_timer::timer4_off);

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
  eeAddr = 614;
  EEPROM.writeInt(eeAddr, param_limit::output_en);
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
  EEPROM.writeInt(eeAddr, param_timer::timer1_on);

  eeAddr = 520;
  EEPROM.writeInt(eeAddr, param_timer::timer2_on);

  eeAddr = 528;
  EEPROM.writeInt(eeAddr, param_timer::timer3_on);

  eeAddr = 536;
  EEPROM.writeInt(eeAddr, param_timer::timer4_on);

  eeAddr = 554;
  EEPROM.writeInt(eeAddr, param_timer::timer1_off);

  eeAddr = 562;
  EEPROM.writeInt(eeAddr, param_timer::timer2_off);

  eeAddr = 570;
  EEPROM.writeInt(eeAddr, param_timer::timer3_off);

  eeAddr = 578;
  EEPROM.writeInt(eeAddr, param_timer::timer4_off);

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
  eeAddr = 614;
  EEPROM.writeInt(eeAddr, param_limit::output_en);
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
  param_timer::timer1_on = EEPROM.readInt(eeAddr);

  eeAddr = 520;
  param_timer::timer2_on = EEPROM.readInt(eeAddr);

  eeAddr = 528;
  param_timer::timer3_on = EEPROM.readInt(eeAddr);

  eeAddr = 536;
  param_timer::timer4_on = EEPROM.readInt(eeAddr);

  eeAddr = 554;
  param_timer::timer1_off = EEPROM.readInt(eeAddr);

  eeAddr = 562;
  param_timer::timer2_off = EEPROM.readInt(eeAddr);

  eeAddr = 570;
  param_timer::timer3_off = EEPROM.readInt(eeAddr);

  eeAddr = 578;
  param_timer::timer4_off = EEPROM.readInt(eeAddr);

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
  eeAddr = 614;
  param_limit::output_en = EEPROM.readFloat(eeAddr);
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
/*
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
*/
String IpAddress2String(const IPAddress &ipAddress)
{
  return String(ipAddress[0]) + String(".") +
         String(ipAddress[1]) + String(".") +
         String(ipAddress[2]) + String(".") +
         String(ipAddress[3]);
}

String parseJsonSerialBTIn(String jsonStr)
{
  // StaticJsonBuffer<200> jsonBuffer;
  char json[128]; // = "{\"cmd\":\"setSSID\",\"devId\":\"001\",\"ssid\":\"Technometric2\",\"pswd\":\"windi09dhika07\",\"localPort\":8888,\"remotePort\":8899}";
  StringToCharArray(jsonStr, json);

  JsonObject &root = jsonBuffer.parseObject(json);
  if (!root.success())
  {
    // Serial.println("parseObject() failed");
    Serial.printf("{\"Status\":1,\"message\":\"JSON pharsing error\"}\n");
    SerialBT.printf("{\"Status\":1,\"message\":\"JSON pharsing error\"}\n");
    return "";
  }
  String cmd = root["cmd"];
  String dev = root["devId"];
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
    Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
    SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
    delay(1000);
  }
  else if (cmd.equals("setSSID"))
  {
    String dev = root["devId"];

    ssid = root["ssid"].as<String>();
    pswd = root["pswd"].as<String>();
    EEPROM_put("");
    connected = false;
    StringToCharArray(ssid, cssid);
    StringToCharArray(pswd, cpswd);
    connectToWiFi(cssid, cpswd);
    Serial.printf("{\"Status\":0,\"message\":\"Reconnect to network\"}\n");
    SerialBT.printf("{\"Status\":0,\"message\":\"Reconnect to network\"}\n");
    delay(1000);
    EEPROM_get();
  }
  else if (cmd.equals("setDevice"))
  {
    localport = root["portIn"];
    remote_port = root["portOut"];
    EEPROM_put(dev);
    delay(5000);
    Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
  }
  else if (cmd.equals("getConfig"))
  {
    SerialBT.printf("{\"Status\":\"getConfig\",\"devId\":\"%s\",\"ssid\":\"%s\",\"pswd\":\"%s\",\"localIp\":\"%s\",\"portIn\":%d,\"portOut\":%d}\n", devId, cssid, cpswd, cip, localport, remote_port);
    Serial.printf("{\"Status\":\"getConfig\",\"devId\":\"%s\",\"ssid\":\"%s\",\"pswd\":\"%s\",\"localIp\":\"%s\",\"portIn\":%d,\"portOut\":%d}\n", devId, cssid, cpswd, cip, localport, remote_port);
  }
  return "";
}