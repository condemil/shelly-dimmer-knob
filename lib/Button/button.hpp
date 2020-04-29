#pragma once

#include <Arduino.h>

enum class ButtonType {
    pullup,
    pulldown,
};

class Button
{
    public:
        Button();
        void setup(ButtonType buttonType, uint8_t buttonPin, std::function<void()> onClick);
        void handle();
    private:
        uint8_t buttonPin;
        uint8_t pressedState;
        uint8_t previousButtonRead;
        bool buttonPressHandled;
        static const uint8_t debounceDelay = 10;
        unsigned long buttonPressTimeElapsed;
        std::function<void()> onClick;
};
