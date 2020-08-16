#pragma once

#include <Arduino.h>
#include <FS.h>

#include "logger.hpp"

namespace helpers {
void printHex(const char *s, uint8_t len) {
    char output[len * 2 + 1];
    char *ptr = &output[0];
    for (int i = 0; i < len; i++) {
        ptr += sprintf(ptr, "%02X", s[i]);
    }
    logger::debugln(output);
}

class elapsedMillis // modified version of https://github.com/pfeerick/elapsedMillis
{
  private:
    uint16_t ms;

  public:
    elapsedMillis(void) {
        ms = millis();
    }
    operator uint16_t() const {
        return millis() - ms;
    }
    elapsedMillis &operator=(uint16_t val) {
        ms = millis() - val;
        return *this;
    }
};
} // namespace helpers
