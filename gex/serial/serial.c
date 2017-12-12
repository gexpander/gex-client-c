#include "serial.h"

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int set_interface_attribs(int fd, int speed, int parity)
{
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		printf("error %d from tcgetattr\n", errno);
		return -1;
	}

	cfsetospeed(&tty, (speed_t) speed);
	cfsetispeed(&tty, (speed_t) speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
	// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN] = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
	// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		printf("error %d from tcsetattr\n", errno);
		return -1;
	}
	return 0;
}

static void set_blocking(int fd, bool should_block, int read_timeout_0s1)
{
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr(fd, &tty) != 0) {
		fprintf(stderr, "error %d from tggetattr\n", errno);
		return;
	}

	tty.c_cc[VMIN] = (cc_t) (should_block ? 1 : 0);
	tty.c_cc[VTIME] = (cc_t) read_timeout_0s1;

	if (tcsetattr(fd, TCSANOW, &tty) != 0)
		fprintf(stderr, "error %d setting term attributes\n", errno);
}

int serial_open(const char *device, bool blocking, int timeout_0s1)
{
	int fd = open (device, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		fprintf (stderr, "FAILED TO OPEN SERIAL! Error %d opening %s: %s\n", errno, device, strerror (errno));
		return -1;
	}

	set_interface_attribs(fd, B115200, 0);
	set_blocking (fd, blocking, timeout_0s1);

	return fd;
}
