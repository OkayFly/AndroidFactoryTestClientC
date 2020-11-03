// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <linux/serial.h>
#include <errno.h>
#include <stdbool.h>

#include <string.h>
#include "factory_test_client.h"
#include "data.h"

extern bool STOPTEST;

#define LENUSERINPUT 1024

static const char commandlist[NCOMMANDS][10] = 
{
	"START",
	"END"
};

#define UART_BAUD 9600

/*
 * glibc for MIPS has its own bits/termios.h which does not define
 * CMSPAR, so we vampirise the value from the generic bits/termios.h
 */
#ifndef CMSPAR
#define CMSPAR 010000000000
#endif

/*
 * Define modem line bits
 */
#ifndef TIOCM_LOOP
#define TIOCM_LOOP	0x8000
#endif

#define UART_BAUD 9600

// USED
int _cl_rts_cts = 0;
int _cl_2_stop_bit = 0;
int _cl_parity = 0;

int _cl_no_rx = 0; //Don't receive data 
int _cl_no_tx = 0; //Dont't ransmit data
int _cl_loopback = 0; //

int _cl_rs485_rts_after_send = 0; //
int _cl_rs485_before_delay = 0; //
int _cl_rs485_after_delay = -1; //


int _cl_odd_parity = 0;
int _cl_stick_parity = 0;


static void dump_data(unsigned char * b, int count)
{
	printf("%i bytes: ", count);
	int i;
	for (i=0; i < count; i++) {
		printf("%02x ", b[i]);
	}

	printf("\n");
}

static void dump_data_ascii(unsigned char * b, int count)
{
	int i;
	for (i=0; i < count; i++) {
		printf("%c", b[i]);
	}
}

static void set_baud_divisor(int fd, int speed, int custom_divisor)
{
	// default baud was not found, so try to set a custom divisor
	struct serial_struct ss;
	if (ioctl(fd, TIOCGSERIAL, &ss) < 0) {
		perror("TIOCGSERIAL failed");
		exit(1);
	}

	ss.flags = (ss.flags & ~ASYNC_SPD_MASK) | ASYNC_SPD_CUST;
	if (custom_divisor) {
		ss.custom_divisor = custom_divisor;
	} else {
		ss.custom_divisor = (ss.baud_base + (speed/2)) / speed;
		int closest_speed = ss.baud_base / ss.custom_divisor;

		if (closest_speed < speed * 98 / 100 || closest_speed > speed * 102 / 100) {
			fprintf(stderr, "Cannot set speed to %d, closest is %d\n", speed, closest_speed);
			exit(1);
		}

		printf("closest baud = %i, base = %i, divisor = %i\n", closest_speed, ss.baud_base,
				ss.custom_divisor);
	}

	if (ioctl(fd, TIOCSSERIAL, &ss) < 0) {
		perror("TIOCSSERIAL failed");
		exit(1);
	}
}

static void clear_custom_speed_flag(int fd)
{
	struct serial_struct ss;
	if (ioctl(fd, TIOCGSERIAL, &ss) < 0) {
		// return silently as some devices do not support TIOCGSERIAL
		return;
	}

	if ((ss.flags & ASYNC_SPD_MASK) != ASYNC_SPD_CUST)
		return;

	ss.flags &= ~ASYNC_SPD_MASK;

	if (ioctl(fd, TIOCSSERIAL, &ss) < 0) {
		perror("TIOCSSERIAL failed");
		exit(1);
	}
}

// converts integer baud to Linux define
static int get_baud(int baud)
{
	switch (baud) {
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	case 230400:
		return B230400;
	case 460800:
		return B460800;
	case 500000:
		return B500000;
	case 576000:
		return B576000;
	case 921600:
		return B921600;
#ifdef B1000000
	case 1000000:
		return B1000000;
#endif
#ifdef B1152000
	case 1152000:
		return B1152000;
#endif
#ifdef B1500000
	case 1500000:
		return B1500000;
#endif
#ifdef B2000000
	case 2000000:
		return B2000000;
#endif
#ifdef B2500000
	case 2500000:
		return B2500000;
#endif
#ifdef B3000000
	case 3000000:
		return B3000000;
#endif
#ifdef B3500000
	case 3500000:
		return B3500000;
#endif
#ifdef B4000000
	case 4000000:
		return B4000000;
#endif
	default:
		return -1;
	}
}

bool set_modem_lines(int fd, int bits, int mask)
{
	int status, ret;

	ret = ioctl(fd, TIOCMGET, &status);
	if (ret < 0) {
		perror("TIOCMGET failed");
		//exit(1);
		return false;
	}

	status = (status & ~mask) | (bits & mask);

	ret = ioctl(fd, TIOCMSET, &status);
	if (ret < 0) {
		perror("TIOCMSET failed");
		//exit(1);
		return false;
	}
	return true;
}

static void display_help(void)
{
	printf("Usage: linux-serial-test [OPTION]\n"
			"\n"
			"  -h, --help\n"
			"  -b, --baud        Baud rate, 115200, etc (115200 is default)\n"
			"  -p, --port        Port (/dev/ttyS0, etc) (must be specified)\n"
			"  -d, --divisor     UART Baud rate divisor (can be used to set custom baud rates)\n"
			"  -R, --rx_dump     Dump Rx data (ascii, raw)\n"
			"  -T, --detailed_tx Detailed Tx data\n"
			"  -s, --stats       Dump serial port stats every 5s\n"
			"  -S, --stop-on-err Stop program if we encounter an error\n"
			"  -y, --single-byte Send specified byte to the serial port\n"
			"  -z, --second-byte Send another specified byte to the serial port\n"
			"  -c, --rts-cts     Enable RTS/CTS flow control\n"
			"  -B, --2-stop-bit  Use two stop bits per character\n"
			"  -P, --parity      Use parity bit (odd, even, mark, space)\n"
			"  -k, --loopback    Use internal hardware loop back\n"
			"  -e, --dump-err    Display errors\n"
			"  -r, --no-rx       Don't receive data (can be used to test flow control)\n"
			"                    when serial driver buffer is full\n"
			"  -t, --no-tx       Don't transmit data\n"
			"  -l, --rx-delay    Delay between reading data (ms) (can be used to test flow control)\n"
			"  -a, --tx-delay    Delay between writing data (ms)\n"
			"  -w, --tx-bytes    Number of bytes for each write (default is to repeatedly write 1024 bytes\n"
			"                    until no more are accepted)\n"
			"  -q, --rs485       Enable RS485 direction control on port, and set delay from when TX is\n"
			"                    finished and RS485 driver enable is de-asserted. Delay is specified in\n"
			"                    bit times. To optionally specify a delay from when the driver is enabled\n"
			"                    to start of TX use 'after_delay.before_delay' (-q 1.1)\n"
			"  -Q, --rs485_rts   Deassert RTS on send, assert after send. Omitting -Q inverts this logic.\n"
			"  -o, --tx-time     Number of seconds to transmit for (defaults to 0, meaning no limit)\n"
			"  -i, --rx-time     Number of seconds to receive for (defaults to 0, meaning no limit)\n"
			"  -A, --ascii       Output bytes range from 32 to 126 (default is 0 to 255)\n"
			"\n"
	      );
}





static bool setup_serial(int baud, char* port, int *fd)
{
	struct termios newtio;
	struct serial_rs485 rs485;

	*fd = open(port, O_RDWR | O_NONBLOCK);

	if (*fd < 0) {
		perror("Error opening serial port");
		//free(_cl_port1);?????????????????TODO
		//exit(1);
		return false;
	}

	bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

	/* man termios get more info on below settings */
	newtio.c_cflag = baud | CS8 | CLOCAL | CREAD;

	if (_cl_rts_cts) {
		newtio.c_cflag |= CRTSCTS;
	}

	if (_cl_2_stop_bit) {
		newtio.c_cflag |= CSTOPB;
	}

	if (_cl_parity) {
		newtio.c_cflag |= PARENB;
		if (_cl_odd_parity) {
			newtio.c_cflag |= PARODD;
		}
		if (_cl_stick_parity) {
			newtio.c_cflag |= CMSPAR;
		}
	}

	newtio.c_iflag = 0;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;

	// block for up till 128 characters
	newtio.c_cc[VMIN] = 128;

	// 0.5 seconds read timeout
	newtio.c_cc[VTIME] = 5;

	/* now clean the modem line and activate the settings for the port */
	tcflush(*fd, TCIOFLUSH);
	tcsetattr(*fd,TCSANOW,&newtio);

	/* enable/disable rs485 direction control */
	if(ioctl(*fd, TIOCGRS485, &rs485) < 0) {
		if (_cl_rs485_after_delay >= 0) {
			/* error could be because hardware is missing rs485 support so only print when actually trying to activate it */
			//perror("Error getting RS-485 mode");
		}
	} else if (_cl_rs485_after_delay >= 0) {
		rs485.flags |= SER_RS485_ENABLED | SER_RS485_RX_DURING_TX |
			(_cl_rs485_rts_after_send ? SER_RS485_RTS_AFTER_SEND : SER_RS485_RTS_ON_SEND);
		rs485.flags &= ~(_cl_rs485_rts_after_send ? SER_RS485_RTS_ON_SEND : SER_RS485_RTS_AFTER_SEND);
		rs485.delay_rts_after_send = _cl_rs485_after_delay;
		rs485.delay_rts_before_send = _cl_rs485_before_delay;
		if(ioctl(*fd, TIOCSRS485, &rs485) < 0) {
			//perror("Error setting RS-485 mode");
		}
	} else {
		rs485.flags &= ~(SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND | SER_RS485_RTS_AFTER_SEND);
		rs485.delay_rts_after_send = 0;
		rs485.delay_rts_before_send = 0;
		if(ioctl(*fd, TIOCSRS485, &rs485) < 0) {
			//perror("Error setting RS-232 mode");
		}
	}

	return true;
}





static void serial_process_read_data(char* buf, int *buff_len, AndriodProduct* product, int fd,  fsm_state_t* fsm);


static void reply_end(char*buf,  int* buff_len, char* serial, AndriodProduct* product, int fd, fsm_state_t* fsm)
{
	printf_func_mark(__func__);
	char cmd = CTRL_SEND_END;
	char write_data[120] = {0};
	write_data[0] = 0xAA;
	write_data[1] = cmd;
	printf("1");
	memcpy(write_data+2, product->cpu_sn, strlen(product->cpu_sn));
	printf("1 :%d\n", strlen(product->cpu_sn));
	write_data[2+strlen(product->cpu_sn)] = 0x55;
	write_data[3+strlen(product->cpu_sn)] = '\0';
	printf("1");
	printf("\n\t write_data:%s\n",write_data);
	fflush(stdout);
	for(int i=0; i<strlen(write_data);i++)
	{	
		printf("%02x ", write_data[i]);

	}
	ssize_t c = write(fd, write_data,strlen(write_data));

	printf("\n****\twrit size:%d\n", c);
	usleep(1000000);//1s
	//while(1)
	{
		serial_process_read_data(buf, buff_len, product, fd,  fsm);
	}
	
}

static void reply_idle(char* serial, int fd,  AndriodProduct* product, fsm_state_t* fsm)
{
	printf_func_mark(__func__);
	char cmd = CTRL_SEND_IDLE;
	char write_data[120];
	write_data[0] = 0xAA;
	write_data[1] = cmd;
	memcpy(write_data+2, product->cpu_sn, strlen(product->cpu_sn));
	write_data[2+strlen(product->cpu_sn)] = 0x55;
	write_data[3+strlen(product->cpu_sn)] = '\0';
	printf("\n\t write_data:%s\n",write_data);
	for(int i=0; i<strlen(write_data);i++)
	{	
		printf("%02x ", write_data[i]);

	}
	ssize_t c = write(fd, write_data,strlen(write_data));
	usleep(1000000);//1s
}

static void serial_process_write_data(char* buff,  int* buff_len, char* serial, int fd, AndriodProduct* product, fsm_state_t* fsm)
{
	printf_func_mark(__func__);
	char cmd = CTRL_SEND_MAC;
	char write_data[120];
	write_data[0] = 0xAA;
	write_data[1] = cmd;
	memcpy(write_data+2, product->cpu_sn, strlen(product->cpu_sn));
	write_data[2+strlen(product->cpu_sn)] = 0x55;
	write_data[3+strlen(product->cpu_sn)] = '\0';
	printf("\n\t write_data:%s\n",write_data);
	for(int i=0; i<strlen(write_data);i++)
	{	
		printf("%02x ", write_data[i]);

	}
	ssize_t c = write(fd, write_data,strlen(write_data));
	usleep(1000000);//1s
	//while(1)
	{
		serial_process_read_data(buff, buff_len, product, fd,  fsm);
	}
	
}


static void serial_process_read_data(char* buff, int* buff_len, AndriodProduct* product,  int fd, fsm_state_t* fsm)
{
	printf_func_mark(__func__);
	unsigned char rb[95] = {};
	unsigned char data[95] = {};
	int data_length;
	int c = read(fd, &rb, sizeof(rb));
	if (c <= 0)
		return;

	printf(":c:%d\n",c);
	fflush(stdout);
	for(int i=0; i<c; i++)
	{
		printf("[%02x]",rb[i]);
	}

	printf("\n");
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	//put int serail buffer //TODO 超过了怎么说
	*buff_len = strlen(buff);
	memcpy(&buff[*buff_len], rb, c);
	*buff_len += c;

	if(parse_data(buff, *buff_len, data, &data_length) == DATA_PROCESS_SUCCESS)
	{
		
		printf("=====================================>get data:%s, length[%d]\n",data, data_length);

		for(int i=0; i<data_length; i++)
		{
			printf("\t\t %02x ", data[i]);
		}
		process_data(data, data_length, product, fsm);
	

		memset(buff,0,1024);
		*buff_len = 0;
	
	}
}




void serial_process_t(void* params)
{

	printf_func_mark(__func__);
	parameters *data = (parameters*) params;

	int baud = 115200;
#ifdef UART_BAUT
	baud = UART_BAUD;
#endif
	baud = get_baud(baud);


	char *serial_port;
	serial_port = data->port;

	fsm_state_t*  fsm;
	if( !strcmp(serial_port, TTYS1Port))
	{
		printf("@@@1\n");
		fsm = &data->product->TTYS1;
	}
	else if(!strcmp(serial_port, TTYS3Port))
	{
		printf("@@@1\n");
		fsm = &data->product->TTYS3;
	}
	else
	{
		fsm = &data->product->TTYS1;
		printf("????????????????wtftttyUSB0\n");
	}
	

	int serial_fd = -1;

	if(!setup_serial(baud,serial_port, &serial_fd))
	{
		perror("set serial port baud");
		printf("port:%s\n",serial_port);
		free(data->port);
		return;
		//	exit;//??????????????
	}

/////////////////////////////////////////////////////////////////////////////////////////
	//clear_custom_speed_flag
	struct serial_struct ss;
	if (ioctl(serial_fd, TIOCGSERIAL, &ss) < 0) {
		// return silently as some devices do not support TIOCGSERIAL
		return;
	}
	if ((ss.flags & ASYNC_SPD_MASK) != ASYNC_SPD_CUST)
	{
		printf("222 wtf todo\n");
	}
		//return;
	ss.flags &= ~ASYNC_SPD_MASK;
	if (ioctl(serial_fd, TIOCSSERIAL, &ss) < 0) {
		perror("TIOCSSERIAL failed");
		//exit(1);
	}
	printf("3333\n");
	if(!set_modem_lines(serial_fd, _cl_loopback ? TIOCM_LOOP : 0, TIOCM_LOOP))
	{
		perror("et_modem_lines");
		return;
		//	exit;//??????????????

	}

/////////////////////////////////////////////////////////////////////////////////////////
	struct pollfd serial_poll;
	serial_poll.fd = serial_fd;
	if(!_cl_no_rx)
		serial_poll.events |= POLLIN;
	else
	{
		serial_poll.events &= ~POLLIN;
	}
	if(!_cl_no_tx)
		serial_poll.events |= POLLOUT;
	else
	{
		serial_poll.events &= ~POLLOUT;
	}


	printf("** \n\t %s ------ wait write cpu ID\n", serial_port);
	struct timespec  start_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);


	struct timespec current_time;
	char serial_buff[1024] = {0};
	int  buff_len =0;
	bool stoptest = false;
	while(!stoptest)
	{

		int retval = poll(&serial_poll, 1, 1000);
		
		if(retval == -1)
		{
			perror("**\t poll()");
			usleep(10000);
			continue;
		}
		// else if(retval)
		// {
		// 	serial_process_write_data( serial, product);
		// 	last_read = current;
		// }

		switch (*fsm)
		{
		case FSM_IDLE:
			serial_process_write_data( serial_buff, &buff_len,serial_port, serial_fd, data->product, fsm);
			break;
		case FSM_GET_MAC:
			reply_end( serial_buff, &buff_len, serial_port, data->product, serial_fd, fsm);
			break;
		case FSM_GET_END:
			reply_idle(serial_port, serial_fd, data->product, fsm);
			usleep(100);
			reply_idle(serial_port, serial_fd, data->product, fsm);
			usleep(100);
			reply_idle(serial_port, serial_fd, data->product, fsm);
			*fsm = FSM_TEST_OK;
			stoptest = true;
			break;
		default:
			break;
		}

		clock_gettime(CLOCK_MONOTONIC, &current_time);

		if(diff_ms(&current_time,&start_time) >= 30000 )//30s
		{
			printf("\t\t Error %s time consuming >30s but can't receive corrent data\n", serial_port);
			*fsm = FSM_TEST_FAIL;
			stoptest = true;
		}

	}

	//close serial
	printf("**\t close serial:%s\n", data->port);
	fflush(stdout);
	tcdrain(serial_fd);
	set_modem_lines(serial_fd, 0, TIOCM_LOOP);
	tcflush(serial_fd, TCIOFLUSH);
	free(serial_port);
	return;
}