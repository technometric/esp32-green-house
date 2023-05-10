#include "serialEvent.h"

String parseJsonSerialIn(char *devId, int *rdloop, String jsonStr, std::function<void(String)> EEPROM_put, std::function<void(void)> EEPROM_get, std::function<void(char *, char *)> connectToWiFi)
{
    // char json[128]; // = "{\"cmd\":\"setSSID\",\"devId\":\"001\",\"ssid\":\"Technometric2\",\"pswd\":\"windi09dhika07\",\"localPort\":8888,\"remotePort\":8899}";
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

    // StringToCharArray(dev, devId);
    StringToCharArray(dev_id, devId);
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
        // EEPROM_get();
        // StringToCharArray(dev_id, devId);
        SerialBT.printf("{\"Status\":\"getConfig\",\"devId\":\"%s\",\"ssid\":\"%s\",\"pswd\":\"%s\",\"localIp\":\"%s\",\"portIn\":%d,\"portOut\":%d}\n", devId, cssid, cpswd, cip, localport, remote_port);
        Serial.printf("{\"Status\":\"getConfig\",\"devId\":\"%s\",\"ssid\":\"%s\",\"pswd\":\"%s\",\"localIp\":\"%s\",\"portIn\":%d,\"portOut\":%d}\n", devId, cssid, cpswd, cip, localport, remote_port);
    }
    if (dev.equals(dev_id))
    {
        if (cmd.equals("setTimer1On"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer1_on = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimer2On"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer2_on = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimer3On"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer3_on = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimer4On"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer4_on = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimer1Off"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer1_off = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimer2Off"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer2_off = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimer3Off"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer3_off = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimer4Off"))
        {
            int jam = root["jam"];
            int menit = root["menit"];
            param_timer::timer4_off = (jam * 60) + menit;
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setTimerEnable"))
        {
            param_limit::timer1_en = root["timer1_en"];
            param_limit::timer2_en = root["timer2_en"];
            param_limit::timer3_en = root["timer3_en"];
            param_limit::timer4_en = root["timer4_en"];
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("setOutputEnable"))
        {
            param_limit::output_en = root["state"];
            EEPROM_put("");
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
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
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            SerialBT.printf("{\"Status\":0,\"devId\":\"%s\"}\n", devId);
            delay(1000);
        }
        else if (cmd.equals("getAll"))
        {
            int ot1 = digitalRead(pin::relay1);
            int ot2 = digitalRead(pin::relay2);
            int ot3 = digitalRead(pin::relay3);
            int ot4 = digitalRead(pin::relay4);
            SerialBT.printf("{\"Status\":0,\"device_id\":\"%s\",\"Data\":{\"ph\":%.2f,\"soil\":%d,\"tds\":%d,\"ec\":%.2f,\"temp\":%.2f,\"ot1\":%d,\"ot2\":%d,\"ot3\":%d,\"ot4\":%d}}", devId, sensor::ph, sensor::smpercent, sensor::tds, sensor::ec, sensor::suhu_udara, ot1, ot2, ot3, ot4);
            Serial.printf("{\"Status\":0,\"device_id\":\"%s\",\"Data\":{\"ph\":%.2f,\"soil\":%d,\"tds\":%d,\"ec\":%.2f,\"temp\":%.2f,\"ot1\":%d,\"ot2\":%d,\"ot3\":%d,\"ot4\":%d}}", devId, sensor::ph, sensor::smpercent, sensor::tds, sensor::ec, sensor::suhu_udara, ot1, ot2, ot3, ot4);
            rdloop = 0;
        }
        else if (cmd.equals("setRtc"))
        {
            DS3231 clock;
            int tahun = root["tahun"];
            byte year = tahun % 2000;
            byte month = root["bulan"];
            byte date = root["hari"];
            byte hour = root["jam"];
            byte minute = root["menit"];
            byte second = root["detik"];
            clock.setYear(year);
            clock.setMonth(month);
            clock.setDate(date);
            clock.setHour(hour);
            clock.setMinute(minute);
            clock.setSecond(second);
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\r\n", dev);
        }
        else if (cmd.equals("getRtc"))
        {
            DateTime now = rtc.now();
            Serial.printf("{\"Status\":0,\"devId\":\"%s\",\"tanggal\":%d-%d-%d,\"jam\":%d:%d:%d}\r\n", dev, now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
        }
        else if (cmd.equals("rdLoop"))
        {
            //{"cmd":"rdLoop","devId":"01","delay":1}
            int dly = root["delay"];
            *rdloop = dly;
            Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\r\n", dev);
        }
        /*else if (cmd.equals("setRelay1"))
        {
        //{"cmd":"setRelay2","devId":"A001","state":"1"}
        int ot = (root["state"]) > 0 ? 1 : 0;
        digitalWrite(pin::relay1, ot);
        Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\r\n", dev);
        output &= 0xFE;
        output |= ot;
        EEPROM_putOutput(output);
        }
        else if (cmd.equals("setRelay2"))
        {
        //{"cmd":"setRelay2","devId":"01","state":"0"}
        int ot = (root["state"]) > 0 ? 1 : 0;
        digitalWrite(pin::relay2, ot);
        output &= 0xFD;
        output |= (ot << 1);
        Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\r\n", dev);
        EEPROM_putOutput(output);
        }
        else if (cmd.equals("setRelay3"))
        {
        //{"cmd":"setRelay3","devId":"01","state":"0"}
        int ot = (root["state"]) > 0 ? 1 : 0;
        digitalWrite(pin::relay3, ot);
        output &= 0xFB;
        output |= (ot << 2);
        Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\r\n", dev);
        EEPROM_putOutput(output);
        }
        else if (cmd.equals("setRelay4"))
        {
        //{"cmd":"setRelay4","devId":"01","state":"0"}
        int ot = (root["state"]) > 0 ? 1 : 0;
        digitalWrite(pin::relay4, ot);
        output &= 0xF7;
        output |= (ot << 3);
        Serial.printf("{\"Status\":0,\"devId\":\"%s\"}\r\n", dev);
        EEPROM_putOutput(output);
        }*/

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
            sprintf(tmr1_on, "%02d:%02d", param_timer::timer1_on / 60, param_timer::timer1_on % 60);
            sprintf(tmr2_on, "%02d:%02d", param_timer::timer2_on / 60, param_timer::timer2_on % 60);
            sprintf(tmr3_on, "%02d:%02d", param_timer::timer3_on / 60, param_timer::timer3_on % 60);
            sprintf(tmr4_on, "%02d:%02d", param_timer::timer4_on / 60, param_timer::timer4_on % 60);
            sprintf(tmr1_off, "%02d:%02d", param_timer::timer1_off / 60, param_timer::timer1_off % 60);
            sprintf(tmr2_off, "%02d:%02d", param_timer::timer2_off / 60, param_timer::timer2_off % 60);
            sprintf(tmr3_off, "%02d:%02d", param_timer::timer3_off / 60, param_timer::timer3_off % 60);
            sprintf(tmr4_off, "%02d:%02d", param_timer::timer4_off / 60, param_timer::timer4_off % 60);

            Serial.printf("{\"Status\":0,\"devId\":\"%s\",\"timer1_on\":%s,\"timer2_on\":%s,\"timer3_on\":%s,\"timer4_on\":%s,"
                          "\"timer1_off\":%s,\"timer2_off\":%s,\"timer3_off\":%s,\"timer4_off\":%s}\r\n",
                          devId, tmr1_on, tmr2_on, tmr3_on, tmr4_on, tmr1_off, tmr2_off, tmr3_off, tmr4_off);
        }
        else if (cmd.equals("getTimerState"))
        {
            Serial.printf("{\"Status\":0,\"devId\":\"%s\",\"timer1_en\":%d,\"timer2_en\":%d,\"timer3_en\":%d,\"timer4_en\":%d,}\r\n",
                          devId, param_limit::timer1_en, param_limit::timer2_en, param_limit::timer3_en, param_limit::timer4_en);
        }
        else if (cmd.equals("getLimitParam"))
        {
            Serial.printf("{\"Status\":0,\"devId\":\"%s\",\"temp_on\":%d,\"temp_off\":%d,\"soil_on\":%d,\"soil_off\":%d,"
                          "\"ec_on\":%.2f,\"soil_off\":%.2f,\"tds_on\":%d,\"tds_off\":%d,\"ph_on\":%.2f\"ph_off\":%.2f}\r\n",
                          devId, param_limit::temp_on, param_limit::temp_off, param_limit::soil_on, param_limit::soil_off, param_limit::ec_on, param_limit::ec_off,
                          param_limit::tds_on, param_limit::tds_off, param_limit::ph_on, param_limit::ph_off);
        }

        else if (cmd.equals("forceReset"))
        {
            Serial.printf("{\"Status\":\"Device force reset\",\"devId\":\"%s\"}\r\n", dev);
            delay(1000);
            ESP.restart();
        }
    }
    return "";
}