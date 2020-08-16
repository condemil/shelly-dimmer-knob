#pragma once

#include <ArduinoOTA.h>
#include <FS.h>

#include "config.hpp"
#include "logger.hpp"

namespace ota {
void setup() {
    ArduinoOTA.setPort(8266); // port defaults to 8266
    ArduinoOTA.setHostname(config::HOSTNAME); // hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setPassword(config::conf.ota_token);

    ArduinoOTA.onStart([]() {
        if (ArduinoOTA.getCommand() == U_FLASH) {
            logger::debugln(F("ota: start update"));
        } else {
            logger::debugln(F("ota: filesystem update is unsupported"));
        }
    });

    ArduinoOTA.onEnd([]() { logger::debugln(F("ota: update end")); });

    ArduinoOTA.onError([](ota_error_t error) {
        logger::debugf("ota: error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            logger::debugln(F("auth failed"));
        else if (error == OTA_BEGIN_ERROR)
            logger::debugln(F("begin failed"));
        else if (error == OTA_CONNECT_ERROR)
            logger::debugln(F("connect failed"));
        else if (error == OTA_RECEIVE_ERROR)
            logger::debugln(F("receive failed"));
        else if (error == OTA_END_ERROR)
            logger::debugln(F("end failed"));
    });

    ArduinoOTA.begin();
}

void handle() {
    ArduinoOTA.handle();
}
} // namespace ota
