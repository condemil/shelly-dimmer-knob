#pragma once

#include <Arduino.h>

namespace encoder {
static uint8_t _pin_a;
static uint8_t _pin_b;
std::function<void(int8_t)> _on_rotate;
volatile int8_t _position;
volatile byte _a_flag;   // when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte _b_flag;   // when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent
                         // (opposite direction to when a flag is set)
volatile int _reading_a; // somewhere to store the direct values we read from our interrupt pins before checking to see
                         // if we have moved a whole detent
volatile int _reading_b; // somewhere to store the direct values we read from our interrupt pins before checking to see
                         // if we have moved a whole detent

void ICACHE_RAM_ATTR PinA() {
    cli(); // stop interrupts happening before we read pin values
    _reading_a = digitalRead(_pin_a);
    _reading_b = digitalRead(_pin_b);
    if (_reading_a && _reading_b && _a_flag) { // check that we have both pins at detent (HIGH) and that we are expecting
                                               // detent on this pin's rising edge
        _position++;
        _a_flag = 0;
        _b_flag = 0;
    } else if (_reading_a && !_reading_b) {
        _b_flag = 1; // signal that we're expecting pin B to signal the transition to detent from free rotation
    }
    sei(); // restart interrupts
}

void ICACHE_RAM_ATTR PinB() {
    cli(); // stop interrupts before we read pin values
    _reading_a = digitalRead(_pin_a);
    _reading_b = digitalRead(_pin_b);
    if (_reading_a && _reading_b && _b_flag) { // check that we have both pins at detent (HIGH) and that we are expecting
                                            // detent on this pin's rising edge
        _position--;
        _a_flag = 0;
        _b_flag = 0;
    } else if (!_reading_a && _reading_b) {
        _a_flag = 1; // signal that we're expecting pinA to signal the transition to detent from free rotation
    }
    sei(); // restart interrupts
}

void setup(uint8_t pin_a, uint8_t pin_b, std::function<void(int8_t)> on_rotate) {
    _pin_a = pin_a;
    _pin_b = pin_b;
    _on_rotate = on_rotate;

    pinMode(pin_a, INPUT_PULLUP);
    pinMode(pin_b, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin_a), PinA, RISING);
    attachInterrupt(digitalPinToInterrupt(pin_b), PinB, RISING);
}

void handle() {
    if (_position == 0) {
        return;
    }

    _on_rotate(_position);
    _position = 0;
}
} // namespace encoder
