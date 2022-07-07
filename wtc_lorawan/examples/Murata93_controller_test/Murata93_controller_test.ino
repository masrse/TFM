#include "murata93_controller.h"
#include "wtc_serial_arduino.h"
#include "conn_config.h"
#include "wtc_network.h"
#include "conn_status_code.h"
#include "config_mgr.h"
#include "arduino_impl.h"
#include "lossy.h"

Arduino_PinController pinController;
Arduino_SerialController serialController;
Arduino_DelayGenerator delayGenerator;
Watchdog_impl watchdog;
Eeprom_impl eeprom_impl;
// Selección del hardware a utilizar
// #define HW_RTLS_V3_1_0
// #define HW_RTLS_V4_0_X
#define HW_RTLS_V4_1_X
#include "boards.h"

// Intentos de recepcion ACK
#define attempts_ack 2
#define pkt_size     200

char NETWORK_ARG1[]                              = "008000000000E19C";                 // APN_NAME
char NETWORK_ARG2[]                              = "531BD9C5EC5D8BA5EF3B262CEBFB3E66"; // APN_USER

uint32_t Config_mgr_params::first_valid_byte     = (uint32_t)0;
uint32_t Config_mgr_params::last_valid_byte      = (uint32_t)2047;
uC_Eeprom_interface* Config_mgr_params::eeprom_0 = &eeprom_impl;

const uint8_t Conn_config::conn_max              = 1;
const uint16_t Pkt_size::payload_len             = 150;
Lossy lossy;
Data_formatter_interface* Pkt_byte_sync::data_formatter_interface = &lossy;
Data_formatter_interface* Payload_formatter::data_formatter_impl  = &lossy;

static const uint8_t len_fifo                                     = 64;

Fifo<uint8_t> lora_fifo_rx( len_fifo ); // Buffer rx
Fifo<uint8_t> lora_fifo_tx( len_fifo ); // Buffer tx
Wtc_serial_arduino w_serial_lora( lora_fifo_rx, lora_fifo_tx, lora_serial_rx, lora_serial_tx );
Murata93_controller controller( w_serial_lora, lora_fifo_rx, lora_fifo_tx, lora_reset, (baud_rate_t)murata93_baudrate, serialController, pinController, delayGenerator, watchdog );
Network comms( serialController, delayGenerator, pkt_size, attempts_ack );

uint8_t rssi, snr;
uint8_t band;
uint8_t status_connection;
const uint8_t data_to_send[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
bool version                 = false;

void setup() {
    comms.add( &controller );

    pinController.set_pin_mode_input( board_sim_pin_ps );
    pinController.set_pin_mode_output( board_sim_pin_key );
    pinController.output_high( board_sim_pin_key );
    pinController.set_pin_mode_output( board_sim_pin_mosfet );
    pinController.output_low( board_sim_pin_mosfet );

    serialController.config_serial_baud_rate( serial_baudrate );

    bool encendida = false;

    while ( encendida == false ) {
        if ( pinController.read_digital_input( board_sim_pin_ps ) == 0 ) {
            serialController.serial_println( "apagada, encendemos" );

            pinController.output_low( board_sim_pin_key );
            delayGenerator.delay_ms( 3000 );
            pinController.output_high( board_sim_pin_key );
            delayGenerator.delay_ms( 4000 );
        }

        if ( pinController.read_digital_input( board_sim_pin_ps ) == 1 ) {
            serialController.serial_println( "encendida" );
            encendida = true;
        }
    }

    serialController.serial_println( "Inicio test murata93_controller" );

    delayGenerator.delay_ms( 3000 );

    controller.check_version( version );
    controller.set_band( 5 );
    controller.get_band( band );
    controller.set_data_format( 1 );
    controller.set_duty_cycle( 0 );
    controller.set_data_rate( 0 );
    controller.set_rfpower( 0, 1 );
    controller.set_activation_mode( 1 );

    while ( controller.setup_network( NETWORK_ARG1, NETWORK_ARG2 ) != wtc_success ) {
        delayGenerator.delay_ms( 1000 );
    }

    while ( controller.join() != wtc_success ) {
        delayGenerator.delay_ms( 1000 );
    }
}

void loop() {
    controller.send( 0, data_to_send, sizeof( data_to_send ) );
    controller.get_rssi( rssi, snr );

    while ( controller.get_status_connection( status_connection ) != wtc_success ) {
        delayGenerator.delay_ms( 1000 );
    }

    if ( status_connection != Conn_status::cipstatus_code_connected ) {
        while ( controller.join() != wtc_success ) {
            delayGenerator.delay_ms( 1000 );
        }
    }
}
