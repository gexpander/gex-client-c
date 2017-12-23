#include "serial.h"

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int set_interface_attribs(int fd)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        printf("error %d from tcgetattr\n", errno);
        return -1;
    }

    cfsetospeed(&tty, (speed_t) __MAX_BAUD);
    cfsetispeed(&tty, (speed_t) __MAX_BAUD);

    tty.c_cflag = CLOCAL | CREAD | CS8;     // 8-bit chars
    tty.c_iflag = 0;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;             // read doesn't block
    tty.c_cc[VTIME] = 2;            // 0.2 seconds read timeout

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("error %d from tcsetattr\n", errno);
        return -1;
    }
    return 0;
}


int serial_open(const char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "FAILED TO OPEN SERIAL! Error %d opening %s: %s\n", errno, device, strerror(errno));
        return -1;
    }

    set_interface_attribs(fd);

    return fd;
}

void serial_noblock(int fd)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "error %d from tggetattr\n", errno);
        return;
    }

    tty.c_cc[VMIN] = (cc_t) 0;
    tty.c_cc[VTIME] = (cc_t) 0;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        fprintf(stderr, "error %d setting term attributes\n", errno);
}

void serial_shouldwait(int fd, int ms)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        fprintf(stderr, "error %d from tggetattr\n", errno);
        return;
    }

    tty.c_cc[VMIN] = (cc_t) 0;
    tty.c_cc[VTIME] = (cc_t) (ms + 50 / 100);

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
        fprintf(stderr, "error %d setting term attributes\n", errno);
}
