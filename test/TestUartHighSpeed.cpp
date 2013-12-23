
#include <random>
#include <iostream>
#include <stdio.h>
#include <unistd.h>			//Used for UART
#include <fcntl.h>			//Used for UART
#include <termios.h>		//Used for UART
#include <sys/ioctl.h>

#include <linux/serial.h>

#include <csignal>
#include <cstdint>
#include <bitset>

#include <QElapsedTimer>

static volatile bool _running;

void signal_handler(int signum)
{
	_running = false;

}

int main()
{
	_running = true;
	signal(SIGTERM, &signal_handler);

	//-------------------------
	//----- SETUP USART 0 -----
	//-------------------------
	//At bootup, pins 8 and 10 are already set to UART0_TXD, UART0_RXD (ie the alt0 function) respectively
	int uart0_filestream = -1;

	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}

//	if (0)
	{
		//CONFIGURE THE UART
		//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
		//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
		//	CSIZE:- CS5, CS6, CS7, CS8
		//	CLOCAL - Ignore modem status lines
		//	CREAD - Enable receiver
		//	IGNPAR = Ignore characters with parity errors
		//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
		//	PARENB - Parity enable
		//	PARODD - Odd parity (else even)
		struct termios options;
		tcgetattr(uart0_filestream, &options);
		options.c_cflag = B4000000 | CS8 | CLOCAL | CREAD;		//<Set baud rate
		options.c_iflag = IGNPAR;
		options.c_oflag = 0;
		options.c_lflag = 0;
		tcflush(uart0_filestream, TCIFLUSH);

		std::cout << "options.c_cflag = " << options.c_cflag << std::endl;
		std::cout << "options.c_iflag = " << options.c_iflag << std::endl;
		std::cout << "options.c_oflag = " << options.c_oflag << std::endl;
		std::cout << "options.c_lflag = " << options.c_lflag << std::endl;

		tcsetattr(uart0_filestream, TCSANOW, &options);
		// Let's verify configured options
		tcgetattr(uart0_filestream, &options);

		std::cout << "options.c_cflag = " << options.c_cflag << std::endl;
		std::cout << "options.c_iflag = " << options.c_iflag << std::endl;
		std::cout << "options.c_oflag = " << options.c_oflag << std::endl;
		std::cout << "options.c_lflag = " << options.c_lflag << std::endl;
	}
	{
		struct serial_struct ser;

		if (-1 == ioctl(uart0_filestream, TIOCGSERIAL, &ser))
		{
			std::cerr << "Failed to obtian 'serial_struct' for setting custom baudrate" << std::endl;
		}

		std::cout << "Current divisor: " << ser.custom_divisor << " ( = " << ser.baud_base << " / 4000000" << std::endl;

		// set custom divisor
		ser.custom_divisor = ser.baud_base / 8000000;
		// update flags
		ser.flags &= ~ASYNC_SPD_MASK;
		ser.flags |= ASYNC_SPD_CUST;

		std::cout << "Current divisor: " << ser.custom_divisor << " ( = " << ser.baud_base << " / 8000000" << std::endl;


		if (-1 == ioctl(uart0_filestream, TIOCSSERIAL, &ser))
		{
			std::cerr << "Failed to configure 'serial_struct' for setting custom baudrate" << std::endl;
		}

		// Check result
		if (-1 == ioctl(uart0_filestream, TIOCGSERIAL, &ser))
		{
			std::cerr << "Failed to obtian 'serial_struct' for setting custom baudrate" << std::endl;
		}

		std::cout << "Current divisor: " << ser.custom_divisor << " ( = " << ser.baud_base << " / 4000000" << std::endl;
	}


	if (uart0_filestream < 0)
	{
		std::cerr << "Opening the device has failed" << std::endl;
		return -1;
	}

	//----- TX BYTES -----
	uint8_t tx_buffer[3*3*8*4];
	uint8_t *p_tx_buffer;

//	for (int i=0; i<3; ++i)
//	{
		// Writing 0xFF, 0x00, 0x00
//		*p_tx_buffer++ = 0x8C;
//		*p_tx_buffer++ = 0x8C;
//		*p_tx_buffer++ = 0x8C;
//		*p_tx_buffer++ = 0x8C;

	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(1,2);

	p_tx_buffer = &tx_buffer[0];
	for (int i=0; i<9; ++i)
	{
		int coinFlip = distribution(generator);
		if (coinFlip == 1)
		{
			*p_tx_buffer++ = 0xCE;
			*p_tx_buffer++ = 0xCE;
			*p_tx_buffer++ = 0xCE;
			*p_tx_buffer++ = 0xCE;
		}
		else
		{
			*p_tx_buffer++ = 0x8C;
			*p_tx_buffer++ = 0x8C;
			*p_tx_buffer++ = 0x8C;
			*p_tx_buffer++ = 0x8C;
		}
	}

	std::cout << "Binary stream: [";
	for (unsigned char* txIt=&(tx_buffer[0]); txIt!=p_tx_buffer; ++txIt)
	{
		std::cout << "  1 " << (std::bitset<8>) (*txIt) << " 0 ";
	}
	std::cout << "]" << std::endl;

	std::cout << "Type 'c' to continue, 'q' or 'x' to quit: ";
	while (_running)
	{
		char c = getchar();
		if (c == 'q' || c == 'x')
		{
			break;
		}
		if (c != 'c')
		{
			continue;
		}

		int count = write(uart0_filestream, &tx_buffer[0], (p_tx_buffer - &tx_buffer[0]));		//Filestream, bytes to write, number of bytes to write
		if (count < 0)
		{
			std::cerr << "UART TX error" << std::endl;

			//----- CLOSE THE UART -----
			close(uart0_filestream);
			return -1;
		}
		std::cout << "Writing " << count << " bytes to uart" << std::endl;

		p_tx_buffer = &tx_buffer[0];
		for (int i=0; i<9; ++i)
		{
			int coinFlip = distribution(generator);
			if (coinFlip == 1)
			{
				*p_tx_buffer++ = 0xCE;
				*p_tx_buffer++ = 0xCE;
				*p_tx_buffer++ = 0xCE;
				*p_tx_buffer++ = 0xCE;
			}
			else
			{
				*p_tx_buffer++ = 0x8C;
				*p_tx_buffer++ = 0x8C;
				*p_tx_buffer++ = 0x8C;
				*p_tx_buffer++ = 0x8C;
			}
		}
	}

	//----- CLOSE THE UART -----
	close(uart0_filestream);

	std::cout << "Program finished" << std::endl;

	return 0;
}
