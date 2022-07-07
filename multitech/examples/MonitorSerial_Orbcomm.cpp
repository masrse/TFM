// C library headers
#include <stdio.h>
#include <string.h>
// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include "iostream"
#include "istream"

//Tamaño del buffer de recepcion y envio
static const uint8_t buffer_size = 250;

static void delay_sec ( uint8_t num_sec ) {
    time_t now = time( 0 );
    //Delay 2s
    while ( time( 0 ) < now + num_sec ) {
    };
}

static int8_t setup_port( void ) {
    // Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
    int serial_port = open( "/dev/ttyAP2", O_RDWR );
    if ( serial_port < 0 ) {
        printf( "Error %i open port: %s\n", errno, strerror( errno ) );
        return -1;
    }
    // Create new termios struc, we call it 'tty' for convention
    struct termios tty;
    // Read in existing settings, and handle any error
    if ( tcgetattr( serial_port, &tty ) != 0 ) {
        printf( "Error %i from tcgetattr: %s\n", errno, strerror( errno ) );
        return -1;
    }
    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes
    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)
    tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;
    // Set in/out baud rate to be 9600
    cfsetispeed( &tty, B9600 );
    cfsetospeed( &tty, B9600 );
    // Save tty settings, also checking for error
    if ( tcsetattr( serial_port, TCSANOW, &tty ) != 0 ) {
        printf( "Error %i from tcsetattr: %s\n", errno, strerror( errno ) );
        return -1;
    }
    return serial_port;
}

int main() {

    int port = setup_port();
    if ( port < 0 ) {
        return -1;
    }
    char read_buf[buffer_size];
    char write_buf[buffer_size];
    int num_bytes = 0;
    printf( "INICIO TEST\n" );

    while ( 1 ) {
        printf( "\nWrite command: " );
        std::cin.getline( write_buf, 100 );
        strcat( write_buf, "\r\n" );
        printf( "\nWrite message: %s\n", write_buf );
        write( port, write_buf, strlen( write_buf ) -1 );

        delay_sec ( 2 );

        num_bytes = read( port, &read_buf, sizeof( read_buf ) );
        printf( "Read %i bytes. Received message: %s\n", num_bytes, read_buf );
        memset( &read_buf, '\0', sizeof( read_buf ) );
        memset( &write_buf, '\0', sizeof( write_buf ) );
        num_bytes = 0;

        delay_sec ( 1 );
    }

    close( port );
    return 0; // success
}