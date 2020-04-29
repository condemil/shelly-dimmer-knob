#!/usr/bin/env python

import sys
import threading
import termios
import time

import serial

GREEN = 32
YELLOW = 33


class Sniff(threading.Thread):
    def __init__(self, port, baudrate, color, prefix):
        threading.Thread.__init__(self)

        self.port = port
        self.baudrate = baudrate
        self.color = color
        self.prefix = prefix

        self.serial = None
        self.alive = True
        self.connected = False
        self.setDaemon = True
        self.start()

    def connect(self):
        try:
            self.serial = serial.serial_for_url(
                self.port, self.baudrate, parity="N", rtscts=False, xonxoff=False, timeout=1
            )
            self.connected = True
        except (serial.SerialException, termios.error) as e:
            self.println(e)
            time.sleep(1)

    def println(self, s):
        sys.stdout.write("\x1b[{}m{}: {}\x1b[0m\n".format(self.color, self.prefix, s))
        sys.stdout.flush()

    def read(self, cnt):
        out = b''
        while len(out) < cnt and self.alive:
            if not self.connected or not self.serial:
                self.connect()
                continue

            try:
                out += self.serial.read(1)
            except serial.SerialException:
                self.connected = False
                sys.stderr.write("serial exception: %s" % e)
                time.sleep(1)

        return out

    def run(self):
        while self.alive:
            out = ''

            start = self.read(1).hex()

            if start != '01' and self.alive:
                self.println('error: expected start byte: 0x01 received: 0x{}'.format(start))
                continue

            cnt = self.read(1).hex()
            cmd = self.read(1).hex()
            payload_len = int.from_bytes(self.read(1), byteorder='big')
            payload_le = b''
            payload_be = b''

            for _ in range(payload_len):
                p = self.read(1)
                payload_le += p
                payload_be = p + payload_be

            payload_le = payload_le.hex()
            payload_be = payload_be.hex()

            crc = self.read(2).hex()

            end = self.read(1).hex()

            if end != '04' and self.alive:
                self.println('error: expected end byte: 0x04 received: 0x{}'.format(end))

            if self.alive:
                self.println('start={} cnt={} cmd={} len={} payload_le={}, payload_be={}, crc={}, end={}'.format(
                    start, cnt, cmd, payload_len, payload_le, payload_be, crc, end
                ))


if __name__ == "__main__":
    nt1 = Sniff(
        sys.argv[1],
        115200,
        GREEN,
        'tx'
    )
    nt2 = Sniff(
        sys.argv[2],
        115200,
        YELLOW,
        'rx'
    )

    while threading.active_count() > 1:
        try:
            time.sleep(0.1)
        except KeyboardInterrupt:
            nt1.alive = False
            nt2.alive = False

    print()
