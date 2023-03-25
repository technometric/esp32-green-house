#ifndef UDPEVENT_H
#define UDPEVENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <DS3231.h>
#include "param_limit.h"
#include "param_timer.h"
#include "pins.h"
#include "sensors.h"
StaticJsonBuffer<200> jsonBuffer;
char json[128];
RTClib rtc;
String pharseJsonUdpIn(WiFiUDP udp, char *devId, bool connected, int rdloop, int remote_port, const char *jsonStr, std::function<void(String)> EEPROM_put);
int StringToCharArray(String, char *);
#endif