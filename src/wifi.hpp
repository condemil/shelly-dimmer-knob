#pragma once

#include <DNSServer.h>
#include <Syslog.h>
#include <WiFiManager.h>

#include "config.hpp"
#include "helpers.hpp"
#include "logger.hpp"

namespace wifi {
WiFiManagerParameter _custom_mqtt_host("mqtt_host", "MQTT Server", "", 40);
WiFiManagerParameter _custom_mqtt_port("mqtt_port", "MQTT Port", "1883", 8);
WiFiManagerParameter _custom_mqtt_login("mqtt_login", "MQTT Login", "", 40);
WiFiManagerParameter _custom_mqtt_pass("mqtt_pass", "MQTT Password", "", 40, "type='password'");
WiFiManagerParameter _custom_syslog_host("syslog_host", "SYSLOG Server", "", 40);
WiFiManagerParameter _custom_syslog_port("syslog_port", "SYSLOG Port", "514", 8);
WiFiManagerParameter _custom_ota_token("ota_token", "OTA Token", "", 40);

helpers::elapsedMillis _reconnect_time_elapsed;
const unsigned int RECONNECT_DELAY = 5000;

void _saveConfigCallback() {
    logger::debugln(F("wifi: connection established, set network mode to provisioned"));
    config::conf.provisioned = true;
    strcpy(config::conf.mqtt_host, _custom_mqtt_host.getValue());
    config::conf.mqtt_port = atol(_custom_mqtt_port.getValue());
    strcpy(config::conf.mqtt_login, _custom_mqtt_login.getValue());
    strcpy(config::conf.mqtt_pass, _custom_mqtt_pass.getValue());
    strcpy(config::conf.syslog_host, _custom_syslog_host.getValue());
    config::conf.syslog_port = atol(_custom_syslog_port.getValue());
    strcpy(config::conf.ota_token, _custom_ota_token.getValue());
    config::persist();
}

void _connect() {
    WiFi.begin();

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        logger::errorln(F("wifi: connection failed"));
    }

    logger::debugf("wifi: connected, ip address: %s\n", WiFi.localIP().toString().c_str());
}

void setup() {
    if (!config::conf.provisioned) {
        logger::debugln(F("wifi: creating AP for wifi provisioning, connect to http://192.168.4.1"));
        WiFiManager wifiManager;

        wifiManager.setDebugOutput(false);
        wifiManager.setSaveConfigCallback(_saveConfigCallback);

        wifiManager.addParameter(&_custom_mqtt_host);
        wifiManager.addParameter(&_custom_mqtt_port);
        wifiManager.addParameter(&_custom_mqtt_login);
        wifiManager.addParameter(&_custom_mqtt_pass);
        wifiManager.addParameter(&_custom_syslog_host);
        wifiManager.addParameter(&_custom_syslog_port);
        wifiManager.addParameter(&_custom_ota_token);

        wifiManager.startConfigPortal(config::HOSTNAME);
        return;
    }

    WiFi.mode(WIFI_STA);
    _connect();
}

void handle() {
    if (WiFi.status() == WL_CONNECTED)
        return;

    if (_reconnect_time_elapsed >= RECONNECT_DELAY) {
        _reconnect_time_elapsed = 0; // reset timer
        logger::errorln(F("wifi: connection lost, reconnecting"));
        _connect();
    }
}
} // namespace wifi
