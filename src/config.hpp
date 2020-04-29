#pragma once

#include <ArduinoJson.h>
#include <FS.h>

#include "logger.hpp"

namespace config {
static const char *NAME = "shelly-dimmer-knob";
static const uint8_t ENCODER_PIN_A = 12; // S1
static const uint8_t ENCODER_PIN_B = 14; // S2
static const uint8_t POWER_BUTTON_PIN = 0;
static const uint8_t RESET_BUTTON_PIN = 13;
static const uint8_t STM_NRST_PIN = 5;
static const uint8_t STM_BOOT0_PIN = 4;
static const char *MQTT_TOPIC_STATE_FORMAT = "esp8266/%06x";
static const char *MQTT_TOPIC_COMMAND_FORMAT = "esp8266/%06x/set";
static char HOSTNAME[15];

struct Config {
    char mqtt_host[40];
    uint16_t mqtt_port;
    char mqtt_login[40];
    char mqtt_pass[40];
    char syslog_host[40];
    uint16_t syslog_port;
    char ota_token[40];
    bool provisioned;
};

Config conf;
static const char *_filename = "/config.json";

void setup() {
    sprintf(HOSTNAME, "esp8266-%06x", ESP.getChipId());
    logger::debugf("Hostname: %s\n", config::HOSTNAME);

    if (!SPIFFS.begin()) {
        logger::errorln(F("config: failed to mount FS"));
        return;
    }

    File file = SPIFFS.open(_filename, "r");

    if (!file) {
        logger::errorln(F("config: failed to open file to read"));
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);

    if (error) {
        logger::errorln(F("config: failed to deserialize"));
        file.close();
        return;
    }

    conf.provisioned = doc["provisioned"];
    strlcpy(conf.mqtt_host, doc["mqtt_host"], sizeof(conf.mqtt_host));
    conf.mqtt_port = doc["mqtt_port"];
    strlcpy(conf.mqtt_login, doc["mqtt_login"], sizeof(conf.mqtt_login));
    strlcpy(conf.mqtt_pass, doc["mqtt_pass"], sizeof(conf.mqtt_pass));
    strlcpy(conf.syslog_host, doc["syslog_host"], sizeof(conf.syslog_host));
    conf.syslog_port = doc["syslog_port"];
    strlcpy(conf.ota_token, doc["ota_token"], sizeof(conf.ota_token));

    file.close();
}

void persist() {
    File file = SPIFFS.open(_filename, "w");

    if (!file) {
        logger::errorln(F("config: failed to open file to write"));
        return;
    }

    StaticJsonDocument<512> doc;

    doc["provisioned"] = conf.provisioned;
    doc["mqtt_host"] = conf.mqtt_host;
    doc["mqtt_port"] = conf.mqtt_port;
    doc["mqtt_login"] = conf.mqtt_login;
    doc["mqtt_pass"] = conf.mqtt_pass;
    doc["syslog_host"] = conf.syslog_host;
    doc["syslog_port"] = conf.syslog_port;
    doc["ota_token"] = conf.ota_token;

    if (serializeJson(doc, file) == 0) {
        logger::errorln(F("config: failed to write file"));
    }

    file.close();
    logger::debugln(F("config: persisted"));
}

void truncate() {
    conf.provisioned = false;
    conf.mqtt_host[0] = (char)0;
    conf.mqtt_port = 1883;
    conf.mqtt_login[0] = (char)0;
    conf.mqtt_pass[0] = (char)0;
    conf.syslog_host[0] = (char)0;
    conf.syslog_port = 514;
    conf.ota_token[0] = (char)0;
    persist();
}
} // namespace config
