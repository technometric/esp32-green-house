#ifndef SERIALEVENT_H
#define SERIALEVENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <DS3231.h>
#include <BluetoothSerial.h>

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
extern char json[128];
extern RTClib rtc;
String parseJsonSerialIn(BluetoothSerial SerialBT, char *devId, int *rdloop, String jsonStr, std::function<void(String)> EEPROM_put, std::function<void(void)> EEPROM_get);
int StringToCharArray(String, char *);
#endif