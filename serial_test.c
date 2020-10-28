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

// command line args
int _cl_baud = 0;
char *_cl_port = NULL;
int _cl_divisor = 0;
int _cl_rx_dump = 0;
int _cl_rx_dump_ascii = 0;
int _cl_tx_detailed = 0;
int _cl_stats = 0;
int _cl_stop_on_error = 0;
int _cl_single_byte = -1;
int _cl_another_byte = -1;
int _cl_rts_cts = 0;
int _cl_2_stop_bit = 0;
int _cl_parity = 0;
int _cl_odd_parity = 0;
int _cl_stick_parity = 0;
int _cl_loopback = 0;
int _cl_dump_err = 0;
int _cl_no_rx = 0;
int _cl_no_tx = 0;
int _cl_rx_delay = 0;
int _cl_tx_delay = 0;
int _cl_tx_bytes = 0;
int _cl_rs485_after_delay = -1;
int _cl_rs485_before_delay = 0;
int _cl_rs485_rts_after_send = 0;
int _cl_tx_time = 0;
int _cl_rx_time = 0;
int _cl_ascii_range = 0;

// Module variables
unsigned char _write_count_value = 0;
unsigned char _read_count_value = 0;
int _fd = -1;
unsigned char * _write_data;
ssize_t _write_size;

// keep our own counts for cases where the driver stats don't work
long long int _write_count = 0;
long long int _read_count = 0;
long long int _error_count = 0;

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

static void set_baud_divisor(int speed, int custom_divisor)
{
	// default baud was not found, so try to set a custom divisor
	struct serial_struct ss;
	if (ioctl(_fd, TIOCGSERIAL, &ss) < 0) {
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

	if (ioctl(_fd, TIOCSSERIAL, &ss) < 0) {
		perror("TIOCSSERIAL failed");
		exit(1);
	}
}

static void clear_custom_speed_flag()
{
	struct serial_struct ss;
	if (ioctl(_fd, TIOCGSERIAL, &ss) < 0) {
		// return silently as some devices do not support TIOCGSERIAL
		return;
	}

	if ((ss.flags & ASYNC_SPD_MASK) != ASYNC_SPD_CUST)
		return;

	ss.flags &= ~ASYNC_SPD_MASK;

	if (ioctl(_fd, TIOCSSERIAL, &ss) < 0) {
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

static void process_options(int argc, char * argv[])
{
	for (;;) {
		int option_index = 0;
		static const char *short_options = "hb:p:d:R:TsSy:z:cBertq:Ql:a:w:o:i:P:kA";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"baud", required_argument, 0, 'b'},
			{"port", required_argument, 0, 'p'},
			{"divisor", required_argument, 0, 'd'},
			{"rx_dump", required_argument, 0, 'R'},
			{"detailed_tx", no_argument, 0, 'T'},
			{"stats", required_argument, 0, 's'},
			//{"stats", no_argument, 0, 's'},
			{"stop-on-err", no_argument, 0, 'S'},
			{"single-byte", no_argument, 0, 'y'},
			{"second-byte", no_argument, 0, 'z'},
			{"rts-cts", no_argument, 0, 'c'},
			{"2-stop-bit", no_argument, 0, 'B'},
			{"parity", required_argument, 0, 'P'},
			{"loopback", no_argument, 0, 'k'},
			{"dump-err", no_argument, 0, 'e'},
			{"no-rx", no_argument, 0, 'r'},
			{"no-tx", no_argument, 0, 't'},
			{"rx-delay", required_argument, 0, 'l'},
			{"tx-delay", required_argument, 0, 'a'},
			{"tx-bytes", required_argument, 0, 'w'},
			{"rs485", required_argument, 0, 'q'},
			{"rs485_rts", no_argument, 0, 'Q'},
			{"tx-time", required_argument, 0, 'o'},
			{"rx-time", required_argument, 0, 'i'},
			{"ascii", no_argument, 0, 'A'},
			{0,0,0,0},
		};

		int c = getopt_long(argc, argv, short_options,
				long_options, &option_index);

		if (c == EOF) {
			break;
		}

		switch (c) {
		case 0:
		case 'h':
			display_help();
			exit(0);
			break;
		case 'b':
			_cl_baud = atoi(optarg);
			break;
		case 'p':
			_cl_port = strdup(optarg);
			break;
		case 'd':
			_cl_divisor = strtol(optarg, NULL, 0);
			break;
		case 'R':
			_cl_rx_dump = 1;
			_cl_rx_dump_ascii = !strcmp(optarg, "ascii");
			break;
		case 'T':
			_cl_tx_detailed = 1;
			break;
		case 's':
			_cl_stats = 1;
			char *endptr;
			_cl_rx_time = strtol(optarg, &endptr, 0);
			printf("s:%d\n",_cl_rx_time);
			
			break;
		case 'S':
			_cl_stop_on_error = 1;
			break;
		case 'y': {
			char * endptr;
			_cl_single_byte = strtol(optarg, &endptr, 0);
			break;
		}
		case 'z': {
			char * endptr;
			_cl_another_byte = strtol(optarg, &endptr, 0);
			break;
		}
		case 'c':
			_cl_rts_cts = 1;
			break;
		case 'B':
			_cl_2_stop_bit = 1;
			break;
		case 'P':
			_cl_parity = 1;
			_cl_odd_parity = (!strcmp(optarg, "mark")||!strcmp(optarg, "odd"));
			_cl_stick_parity = (!strcmp(optarg, "mark")||!strcmp(optarg, "space"));
			break;
		case 'k':
			_cl_loopback = 1;
			break;
		case 'e':
			_cl_dump_err = 1;
			break;
		case 'r':
			_cl_no_rx = 1;
			break;
		case 't':
			_cl_no_tx = 1;
			break;
		case 'l': {
			char *endptr;
			_cl_rx_delay = strtol(optarg, &endptr, 0);
			break;
		}
		case 'a': {
			char *endptr;
			_cl_tx_delay = strtol(optarg, &endptr, 0);
			break;
		}
		case 'w': {
			char *endptr;
			_cl_tx_bytes = strtol(optarg, &endptr, 0);
			break;
		}
		case 'q': {
			char *endptr;
			_cl_rs485_after_delay = strtol(optarg, &endptr, 0);
			_cl_rs485_before_delay = strtol(endptr+1, &endptr, 0);
			break;
		}
		case 'Q':
			_cl_rs485_rts_after_send = 1;
			break;
		case 'o': {
			char *endptr;
			_cl_tx_time = strtol(optarg, &endptr, 0);
			break;
		}
		case 'i': {
			char *endptr;
			_cl_rx_time = strtol(optarg, &endptr, 0);
			break;
		}
		case 'A':
			_cl_ascii_range = 1;
			break;
		}
	}
}

static void dump_serial_port_stats(void)
{
	struct serial_icounter_struct icount = { 0 };

	printf("%s: count for this session: rx=%lld, tx=%lld, rx err=%lld\n", _cl_port, _read_count, _write_count, _error_count);

	int ret = ioctl(_fd, TIOCGICOUNT, &icount);
	if (ret != -1) {
		printf("%s: TIOCGICOUNT: ret=%i, rx=%i, tx=%i, frame = %i, overrun = %i, parity = %i, brk = %i, buf_overrun = %i\n",
				_cl_port, ret, icount.rx, icount.tx, icount.frame, icount.overrun, icount.parity, icount.brk,
				icount.buf_overrun);
	}
}

static unsigned char next_count_value(unsigned char c)
{
	c++;
	if (_cl_ascii_range && c == 127)
		c = 32;
	return c;
}

static void process_read_data(void)
{
	unsigned char rb[95];

	//unsigned char rb[1024];
	int c = read(_fd, &rb, sizeof(rb));
	if (c > 0) {
		if (_cl_rx_dump) {
			if (_cl_rx_dump_ascii)
				dump_data_ascii(rb, c);
			else
				dump_data(rb, c);
		}

		// verify read count is incrementing
		int i;
		for (i = 0; i < c; i++) {
			if (rb[i] != _read_count_value) {
				if (_cl_dump_err) {
					printf("Error, count: %lld, expected %02x, got %02x\n",
							_read_count + i, _read_count_value, rb[i]);
				}
				_error_count++;
				if (_cl_stop_on_error) {
					dump_serial_port_stats();
					exit(1);
				}
				_read_count_value = rb[i];
			}
			_read_count_value = next_count_value(_read_count_value);
		}
		_read_count += c;
	}
}

static void process_write_data(void)
{
	ssize_t count = 0;
	int repeat = (_cl_tx_bytes == 0);

	do
	{
		ssize_t i;
		for (i = 0; i < _write_size; i++) {
			_write_data[i] = _write_count_value;
			_write_count_value = next_count_value(_write_count_value);
		}

		ssize_t c = write(_fd, _write_data, _write_size);

		if (c < 0) {
			if (errno != EAGAIN) {
				printf("write failed - errno=%d (%s)\n", errno, strerror(errno));
			}
			c = 0;
		}

		count += c;

		if (c < _write_size) {
			_write_count_value = _write_data[c];
			repeat = 0;
		}
	} while (repeat);

	_write_count += count;

	if (_cl_tx_detailed)
		printf("wrote %zd bytes\n", count);
}


static bool setup_serial_port(int baud)
{
	struct termios newtio;
	struct serial_rs485 rs485;

	_fd = open(_cl_port, O_RDWR | O_NONBLOCK);

	if (_fd < 0) {
		perror("Error opening serial port");
		free(_cl_port);
	//	exit(1);
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
	tcflush(_fd, TCIOFLUSH);
	tcsetattr(_fd,TCSANOW,&newtio);

	/* enable/disable rs485 direction control */
	if(ioctl(_fd, TIOCGRS485, &rs485) < 0) {
		if (_cl_rs485_after_delay >= 0) {
			/* error could be because hardware is missing rs485 support so only print when actually trying to activate it */
			perror("info getting RS-485 mode");
		}
	} else if (_cl_rs485_after_delay >= 0) {
		rs485.flags |= SER_RS485_ENABLED | SER_RS485_RX_DURING_TX |
			(_cl_rs485_rts_after_send ? SER_RS485_RTS_AFTER_SEND : SER_RS485_RTS_ON_SEND);
		rs485.flags &= ~(_cl_rs485_rts_after_send ? SER_RS485_RTS_ON_SEND : SER_RS485_RTS_AFTER_SEND);
		rs485.delay_rts_after_send = _cl_rs485_after_delay;
		rs485.delay_rts_before_send = _cl_rs485_before_delay;
		if(ioctl(_fd, TIOCSRS485, &rs485) < 0) {
			perror("info setting RS-485 mode");
		}
	} else {
		rs485.flags &= ~(SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND | SER_RS485_RTS_AFTER_SEND);
		rs485.delay_rts_after_send = 0;
		rs485.delay_rts_before_send = 0;
		if(ioctl(_fd, TIOCSRS485, &rs485) < 0) {
			perror("info setting RS-232 mode");
		}
	}

	return true;
}



bool check_count(const char* p)
{
	bool ret = true;

	if(p == NULL)
		return false;
	else
	{
		while(*p != '\0')
		{
			if(*p <= '9' && *p++ >= '0')
				continue;
			else
			{
				return false;
			}
			
		}
	}
	

	return ret;
}
static void append_count(struct command* c, char* s)
{
	if(check_count(s))
		c->count = atoi(s);
	else
	{
		printf("--_--%s is not count\n",s);
	}
	
	
}

static char* enum2str(int comid)
{
	switch (comid)
	{
	case START:
		return "START";
		break;
	case END:
		return "END";
		break;
	
	default:
		break;
	}

	return NULL;
}

struct command* userinputtocommand(char s[LENUSERINPUT])
{
	//printf("userinput:%s\n",s);
	struct command* cmd = (struct command*) malloc(sizeof(struct command));
	cmd->comid = -1;
	cmd->count = -1;
	int i, j;
	char* token;
	char* savestate;
	for(i=0; ;i++, s=NULL)
	{
		token = strtok_r(s, " \t\n", &savestate);
		//printf("token:%s\n",token);
		if(token == NULL)
			break;

		if(cmd->comid == -1 )
		{
			for(j=0; i<NCOMMANDS; j++)
			{
				if(!strcmp(token, commandlist[j]))
				{
					cmd->comid = j;
					break;
				}
			}
		}
		else
		{
			if(cmd->comid == START)
			{
				append_count(cmd, token);
			}
		}

	}

	if(cmd->comid != -1 && cmd->count != -1)
	{
		printf("\t\tcmd:[%s]\n", enum2str(cmd->comid));
		printf("\t\tcount:[%d]\n",cmd->count);
		return cmd;
	}
	else
	{
		if(cmd->comid == -1)
			printf("Please input [START] \n");
		else
		{
			printf("pleas after [START] input count num\n");
		}
		
		fprintf(stderr, "\t Error parsing command\n");
		return NULL;
	}

}


static void serial_process_read_data(AndriodProduct* product, fsm_state_t* fsm);


static void reply_end(char* serial, AndriodProduct* product, fsm_state_t* fsm)
{
	char cmd = CTRL_SEND_END;
	char write_data[120];
	//wrap_data(CPU_ID, CTRL_SEND_MAC);
	write_data[0] = 0xAA;
	write_data[1] = cmd;
	memcpy(write_data+2, CPU_ID, strlen(CPU_ID));
	write_data[2+strlen(CPU_ID)] = 0x55;
	write_data[3+strlen(CPU_ID)] = '\0';
	printf("\n\t write_data:%s\n",write_data);
	for(int i=0; i<strlen(write_data);i++)
	{	
		printf("%02x ", write_data[i]);

	}
	ssize_t c = write(_fd, write_data,strlen(write_data));
	usleep(1000000);//1s
	//while(1)
	{
		serial_process_read_data(product, fsm);
	}
	
}

static void serial_process_write_data(char* serial, AndriodProduct* product, fsm_state_t* fsm)
{
	char cmd = CTRL_SEND_MAC;
	char write_data[120];
	//wrap_data(CPU_ID, CTRL_SEND_MAC);
	write_data[0] = 0xAA;
	write_data[1] = cmd;
	memcpy(write_data+2, CPU_ID, strlen(CPU_ID));
	write_data[2+strlen(CPU_ID)] = 0x55;
	write_data[3+strlen(CPU_ID)] = '\0';
	printf("\n\t write_data:%s\n",write_data);
	for(int i=0; i<strlen(write_data);i++)
	{	
		printf("%02x ", write_data[i]);

	}
	ssize_t c = write(_fd, write_data,strlen(write_data));
	usleep(1000000);//1s
	//while(1)
	{
		serial_process_read_data(product, fsm);
	}
	
}


static void serial_process_read_data(AndriodProduct* product, fsm_state_t* fsm)
{
	unsigned char rb[95] = {};
	unsigned char data[95] = {};
	int data_length;
	int c = read(_fd, &rb, sizeof(rb));

	if(get_data(rb, c, data, &data_length) == DATA_PROCESS_SUCCESS)
	{
		
		printf("=====================================>get data:%s, length[%d]\n",data, data_length);

		for(int i=0; i<data_length; i++)
		{
			printf("\t\t %02x ", data[i]);
		}
		process_data(data, data_length, product, fsm);
	
	}
}

void serial_process(char* serial, AndriodProduct* product)
{
	printf("**\t serail_process1111111111:%s\n", serial);
	int baud = B115200;
	_cl_port = serial;
	if(!setup_serial_port(baud))
		return ;
	clear_custom_speed_flag();
	if(!set_modem_lines(_fd, _cl_loopback ? TIOCM_LOOP : 0, TIOCM_LOOP))
		return;

	struct pollfd serial_poll;
	serial_poll.fd = _fd;
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


	printf("** \t wait write cpu ID\n");
	fflush(stdout);
	struct timespec start_write;
	clock_gettime(CLOCK_MONOTONIC, &start_write);

	struct timespec current;

	fsm_state_t fsm;
	if(!strcmp(serial, TTYS1Port))
		fsm = product->TTYS1;
	else if(!strcmp(serial, TTYS3Port))
		fsm = product->TTYS3;
	else
	{
		printf("mddddddddddddddddddddddddddddddddddddddddd\n");
	}

	while(!STOPTEST)
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




		switch (fsm)
		{
		case FSM_IDLE:
			serial_process_write_data( serial, product, &fsm);
			break;
		case FSM_GET_MAC:
			reply_end( serial, product, &fsm);
		
			break;
		case FSM_GET_END:
			STOPTEST = true;
			break;
		default:
			break;
		}

	clock_gettime(CLOCK_MONOTONIC, &current);

	if(diff_ms(&current,&start_write) >= 30000 )//30s
	{
		printf("\t\t Error %s time consuming >30s but can't receive corrent data\n", serial);
		STOPTEST = true;
	}

	}

	//close serial
	printf("**\t close serial:%s\n", serial);
	fflush(stdout);
	tcdrain(_fd);
	dump_serial_port_stats();
	set_modem_lines(_fd, 0, TIOCM_LOOP);
	tcflush(_fd, TCIOFLUSH);
	//free(_cl_port);


	STOPTEST = false;

}


void serial_test(AndriodProduct* product)
{
	serial_process(TTYS1Port, product);
	serial_process(TTYS3Port, product);
}
