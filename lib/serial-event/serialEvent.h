#ifndef SERIALEVENT_H
#define SERIALEVENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <DS3231.h>
#include <BluetoothSerial.h>
extern BluetoothSerial SerialBT;
extern StaticJsonBuffer<200> jsonBuffer;
namespace param_limit
{
    extern int output_en;
    extern int timer1_en;
    extern int timer2_en;
    extern int timer3_en;
    extern int timer4_en;
    extern int temp_on, temp_off;
    extern int soil_on, soil_off;
    extern float ec_on, ec_off;
    extern float tds_on, tds_off;
    extern float ph_on, ph_off;
}
namespace param_timer
{
    extern int timer1_on;
    extern int timer2_on;
    extern int timer3_on;
    extern int timer4_on;
    extern int timer1_off;
    extern int timer2_off;
    extern int timer3_off;
    extern int timer4_off;
}
namespace pin
{
    extern int tds_sensor;
    extern int ph_sensor;
    extern int soil_sensor;
    extern int dht21_sensor;
    extern int one_wire_bus;
    extern int led_builtin;
    extern int relay1;
    extern int relay2;
    extern int relay3;
    extern int relay4;
}

namespace sensor
{
    extern float ec;
    extern int tds;
    extern float waterTemp;
    extern float ecCalibration;
    extern int smvalue;
    extern int smpercent;
    extern float suhu_udara;
    extern float kelembaban;
    extern float ph;
}
extern String dev_id;
extern int localport;
extern int remote_port;
extern String ssid;
extern String pswd;
extern bool connected;
extern char cssid[50]; // = "Technometric2";
extern char cpswd[50]; // = "12345678";
extern char json[128];
extern char cip[30];
extern RTClib rtc;
String parseJsonSerialIn(char *devId, int *rdloop, String jsonStr, std::function<void(String)> EEPROM_put, std::function<void(void)> EEPROM_get, std::function<void(char *, char *)> connectToWiFi);
int StringToCharArray(String, char *);
#endif