#ifndef SERIALEVENT_H
#define SERIALEVENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <DS3231.h>
#include <BluetoothSerial.h>
#include "param_limit.h"
#include "param_timer.h"
#include "pins.h"
#include "sensors.h"
// StaticJsonBuffer<200> jsonBuffer;
char json[128];
RTClib rtc;
String serialJsonUdpIn(BluetoothSerial SerialBT, char *devId, bool connected, int rdloop, const char *jsonStr, std::function<void(String)> EEPROM_put, std::function<void()> EEPROM_get);
int StringToCharArray(String, char *);
#endif