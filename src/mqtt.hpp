#pragma once

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#include "config.hpp"
#include "helpers.hpp"
#include "logger.hpp"

namespace mqtt {
const uint8_t COMMAND_RESET = 0;
const uint8_t COMMAND_RESTART = 1;

char MQTT_TOPIC_STATE[15];
char MQTT_TOPIC_COMMAND[20];

std::function<void()> _onSubscribed;
std::function<void(uint8_t)> _onCommand;
std::function<void(uint8_t)> _onBrightness;
std::function<void(bool)> _onPower;

WiFiClient _esp_client;
PubSubClient _client(_esp_client);
helpers::elapsedMillis _reconnect_time_elapsed;
const unsigned int RECONNECT_DELAY = 5000;

void publish(bool currentPower, uint8_t currentBrightness) {
    StaticJsonDocument<128> doc;

    doc["state"] = currentPower ? "ON" : "OFF";
    doc["brightness"] = currentBrightness;

    char buffer[128];
    serializeJson(doc, buffer);

    _client.publish(MQTT_TOPIC_STATE, buffer, true);
}

void _callback(char *topic, byte *payload, unsigned int length) {
    if (length >= MQTT_MAX_PACKET_SIZE) {
        logger::errorln(F("mqtt: payload is too long"));
        return;
    }

    logger::debugf("mqtt: message arrived [%s]\n", topic);

    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        logger::errorln(F("mqtt: failed to deserialize payload"));
        return;
    }

    if (doc["command"].is<char *>()) {
        if (doc["command"] == "reset") {
            _onCommand(COMMAND_RESET);
        } else if (doc["command"] == "restart") {
            _onCommand(COMMAND_RESTART);
        }
    }

    if (doc["brightness"].is<int>()) {
        _onBrightness(doc["brightness"]);
    }

    if (doc["state"].is<char *>()) {
        if (doc["state"] == "ON") {
            _onPower(true);
        } else if (doc["state"] == "OFF") {
            _onPower(false);
        }
    }
}

void _reconnect() {
    logger::debugln(F("mqtt: attempting to connect"));

    if (_client.connect(config::HOSTNAME, config::conf.mqtt_login, config::conf.mqtt_pass)) {
        logger::debugln(F("mqtt: connected"));
        _client.subscribe(MQTT_TOPIC_COMMAND);
        _onSubscribed();
        logger::debugf("mqtt: subscribed to %s\n", MQTT_TOPIC_COMMAND);
    } else {
        logger::debugf(
            "mqtt: connect failed, rc=%d try again in %u seconds\n", _client.state(), RECONNECT_DELAY / 1000);
    }
}

void setup(std::function<void()> onSubscribed, std::function<void(uint8_t)> onCommand,
    std::function<void(uint8_t)> onBrightness, std::function<void(bool)> onPower) {
    _onSubscribed = onSubscribed;
    _onCommand = onCommand;
    _onBrightness = onBrightness;
    _onPower = onPower;
    sprintf(MQTT_TOPIC_STATE, config::MQTT_TOPIC_STATE_FORMAT, ESP.getChipId());
    sprintf(MQTT_TOPIC_COMMAND, config::MQTT_TOPIC_COMMAND_FORMAT, ESP.getChipId());
    _client.setServer(config::conf.mqtt_host, config::conf.mqtt_port);
    _client.setCallback(_callback);
}

void handle() {
    if (_client.connected()) {
        _client.loop();
        return;
    }

    if (_reconnect_time_elapsed >= RECONNECT_DELAY) {
        _reconnect_time_elapsed = 0; // reset timer
        _reconnect();
    }
}
} // namespace mqtt
