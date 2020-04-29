#pragma once

#include <Arduino.h>
#include <Syslog.h>
#include <WiFiUdp.h>

namespace logger {
WiFiUDP _udpClient;
Syslog _syslog(_udpClient);
bool _syslog_active = false;

static Print &_out = Serial1;

void debugln(const char *);
void debugln(const __FlashStringHelper *);
void errorln(const __FlashStringHelper *);
void debugf(const char *, ...);

void setup() {
    Serial1.begin(115200);
}

void setupSyslog(const char *syslog_host, uint16_t syslog_port, const char *hostname, const char *name) {
    _syslog.server(syslog_host, syslog_port);
    _syslog.deviceHostname(hostname);
    _syslog.appName(name);
    _syslog.defaultPriority(LOG_KERN);
    _syslog_active = true;
}

void debugln(const char *s) {
    _out.println(s);
    if (_syslog_active) {
        _syslog.log(LOG_DEBUG, s);
    }
}

void debugln(const __FlashStringHelper *s) {
    _out.println(s);
    if (_syslog_active) {
        _syslog.log(LOG_DEBUG, s);
    }
}

void errorln(const __FlashStringHelper *s) {
    _out.println(s);
    if (_syslog_active) {
        _syslog.log(LOG_ERR, s);
    }
}

void debugf(const char *format, ...) {
    va_list arg;
    va_start(arg, format);
    char temp[64];
    char* buffer = temp;
    size_t len = vsnprintf(temp, sizeof(temp), format, arg);
    va_end(arg);
    if (len > sizeof(temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) {
            debugln(F("Not enough space for debugf string"));
            return;
        }
        va_start(arg, format);
        vsnprintf(buffer, len + 1, format, arg);
        va_end(arg);
    }
    len = _out.write((const uint8_t*) buffer, len);
    if (_syslog_active) {
        _syslog.log(LOG_DEBUG, buffer);
    }
    if (buffer != temp) {
        delete[] buffer;
    }
}
}
