#pragma once

#include <Arduino.h>

#include "config.hpp"
#include "helpers.hpp"

namespace dimmer {
const uint8_t CMD_CHANGE_BRIGHTNESS = 0x01;
const uint8_t CMD_VERSION = 0x11;

uint8_t _packet_counter = 0;
uint8_t _packet_start_marker = 0x01;
uint8_t _packet_end_marker = 0x04;

const uint8_t buffer_size = 128;
char tx_buffer[buffer_size];
char rx_buffer[buffer_size];
uint8_t rx_idx = 0;
uint8_t rx_payload_size = 0;

uint16_t crc(char *buffer, uint8_t len) {
    uint16_t c = 0;
    for (int i = 1; i < len; i++) {
        c += buffer[i];
    }
    return c;
}

void receivePacket() {
    while (Serial.available() > 0) {
        uint8_t b = Serial.read();
        rx_buffer[rx_idx] = b;

        if (rx_idx == 0 && b != _packet_start_marker) { // start marker
            logger::debugf("dimmer: received wrong start marker: 0x%02X\n", b);
            continue;
        }

        if (rx_idx == buffer_size - 1) {
            logger::debugln(F("dimmer: rx buffer overflow"));
            rx_idx = 0;
            continue;
        }

        if (rx_idx == 1 && b != _packet_counter - 1) { // packet counter is same as previous tx packet
            logger::debugf("dimmer: received wrong packet counter: 0x%02X\n", b);
            rx_idx = 0;
            continue;
        }

        if (rx_idx == 2) { // payload size
            rx_payload_size = b;
        }

        if (rx_idx == (3 + rx_payload_size + 2)) { // checksum
            uint16_t c = (rx_buffer[rx_idx - 1] << 8) + b;
            if (c != crc(rx_buffer, rx_idx - 1)) {
                logger::debugln(F("dimmer: received wrong checksum"));
                rx_idx = 0;
                continue;
            }
        }

        if (rx_idx == (3 + rx_payload_size + 3) && b != _packet_end_marker) { // end marker
            logger::debugf("dimmer: received wrong end marker: 0x%02X\n", b);
            rx_idx = 0;
            continue;
        }

        if (rx_idx == (3 + rx_payload_size + 3)) { // end marker
            logger::debugln(F("dimmer: received package"));
            helpers::printHex(rx_buffer, rx_idx + 1);
            rx_idx = 0;
            continue;
        }

        rx_idx++;
    }
}

void sendCommand(uint8_t cmd, uint8_t *payload, uint8_t len) {
    uint8_t b = 0;

    tx_buffer[b++] = _packet_start_marker;
    tx_buffer[b++] = _packet_counter;
    tx_buffer[b++] = cmd;
    tx_buffer[b++] = len;

    if (payload) {
        memcpy(tx_buffer + b, payload, len);
    }
    b += len;

    uint16_t c = crc(tx_buffer, b);

    tx_buffer[b++] = c >> 8; // crc first byte (big/network endian)
    tx_buffer[b++] = c; // crc second byte (big/network endian)
    tx_buffer[b] = _packet_end_marker;

    b++;

    Serial.write(tx_buffer, b);
    logger::debugln(F("dimmer: send package"));
    helpers::printHex(tx_buffer, b);

    _packet_counter++;
}

void sendVersion() {
    sendCommand(CMD_VERSION, 0, 0);
}

void sendBrightness(uint8_t b) {
    logger::debugf("dimmer: set brightness: %d\n", b);
    uint8_t payload[] = {
        (uint8_t)(b * 10), (uint8_t)((b * 10) >> 8)}; // b*10 second byte, b*10 first byte (little endian)
    sendCommand(CMD_CHANGE_BRIGHTNESS, payload, sizeof(payload));
}

void setup() {
    Serial.begin(115200);
    pinMode(config::STM_NRST_PIN, OUTPUT);
    pinMode(config::STM_BOOT0_PIN, OUTPUT);
    digitalWrite(config::STM_BOOT0_PIN, LOW); // boot stm from its own flash memory
    digitalWrite(config::STM_NRST_PIN, LOW); // start stm reset
    delay(50);
    digitalWrite(config::STM_NRST_PIN, HIGH); // end stm reset
    delay(50);
    sendVersion();
}

void handle() {
    receivePacket();
}
} // namespace dimmer
