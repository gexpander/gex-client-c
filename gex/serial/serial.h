#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Open a serial port
 *
 * @param device - device to open, e.g. /dev/ttyACM0
 * @param blocking - true for `read()` to block until at least 1 byte is read
 * @param timeout_0s1 - timeout for `read()` - if blocking, starts after the first character
 * @return file descriptor
 */
int serial_open(const char *device, bool blocking, int timeout_0s1);

#endif
