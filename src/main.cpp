#include <Arduino.h>
#include <SoftwareSerial.h>
#include <math.h>

#include "button.hpp"
#include "config.hpp"
#include "dimmer.hpp"
#include "encoder.hpp"
#include "logger.hpp"
#include "mqtt.hpp"
#include "ota.hpp"
#include "wifi.hpp"

void onSubscribed();
void onCommand(uint8_t);
void onBrightness(uint8_t);
void onPower(bool);
void onPowerClick();
void onRotate(int8_t);
void onResetClick();

static bool current_power;
static uint8_t current_brightness = 255;

Button power_button;
Button reset_button;

void setup() {
    logger::setup();
    logger::debugln(F("\nmain: started"));

    config::setup();
    wifi::setup();
    if (strlen(config::conf.syslog_host) != 0) {
        logger::setupSyslog(config::conf.syslog_host, config::conf.syslog_port, config::HOSTNAME, config::NAME);
    }
    mqtt::setup(onSubscribed, onCommand, onBrightness, onPower);
    ota::setup();
    dimmer::setup();
    encoder::setup(config::ENCODER_PIN_A, config::ENCODER_PIN_B, onRotate);
    power_button.setup(ButtonType::pullup, config::POWER_BUTTON_PIN, onPowerClick);
    reset_button.setup(ButtonType::pullup, config::RESET_BUTTON_PIN, onResetClick);

    logger::debugln(F("main: setup is over"));
}

void loop() {
    wifi::handle();
    mqtt::handle();
    ota::handle();
    dimmer::handle();
    encoder::handle();
    power_button.handle();
    reset_button.handle();
}

void onResetClick() {
    config::truncate();
    ESP.restart();
}

void onSubscribed() {
    mqtt::publish(current_power, current_brightness);
}

void onCommand(uint8_t command) {
    switch (command) {
    case mqtt::COMMAND_RESET:
        config::truncate();
        ESP.restart();
        break;
    case mqtt::COMMAND_RESTART:
        ESP.restart();
        break;
    }
}

void onRotate(int8_t position) {
    position *= 5;

    if (position > 255 - current_brightness) {
        onBrightness(255); // overflow
        return;
    } else if (position < 0 && current_brightness < (position * -1)) {
        onBrightness(0); // underflow
        return;
    }

    onBrightness(current_brightness + position);
}

void updateValues() {
    if (current_power) {
        dimmer::sendBrightness(current_brightness * 100 / 255);
    } else {
        dimmer::sendBrightness(0);
    }

    mqtt::publish(current_power, current_brightness);
}

void onBrightness(uint8_t brightness) {
    if (brightness == current_brightness)
        return;

    current_brightness = brightness;

    if (brightness == 0) {
        current_power = false;
    } else {
        current_power = true;
    }

    updateValues();
}

void onPowerClick() {
    onPower(!current_power);
}

void onPower(bool power) {
    if (power == current_power)
        return;

    current_power = power;

    if (power && current_brightness == 0) {
        current_brightness = 255;
    }

    updateValues();
}
