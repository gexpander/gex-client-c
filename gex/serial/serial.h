#ifndef SERIAL_H
#define SERIAL_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Open a serial port
 *
 * @param device - device to open, e.g. /dev/ttyACM0
 * @return file descriptor
 */
int serial_open(const char *device);

void serial_shouldwait(int fd, int ms);

void serial_noblock(int fd);

#endif
