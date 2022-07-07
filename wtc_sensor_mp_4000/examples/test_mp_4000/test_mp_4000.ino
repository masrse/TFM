#include "sensor_mp_4000.h"
#include "LowPower.h"
#include "lossy.h"
#include "pkt.h"
#include "payload_mgr.h"
#include "wtc_serial_arduino.h"
#include "arduino_impl.h"
#include "fifo.h"

// Selección del hardware a utilizar
// #define HW_RTLS_V3_1_0
// #define HW_RTLS_V4_0_X
#define HW_RTLS_V4_1_X
#include "boards.h"

Fifo<uint8_t> fifo_rx( 5 );
Fifo<uint8_t> fifo_tx( 1 );
Arduino_DelayGenerator delayGenerator;
Arduino_SerialController serialController;
Arduino_SerialController debugPrinter1;
Watchdog_impl watchdog;
Arduino_PinController pinController;
Wtc_serial_arduino w_serial_arduino( fifo_rx, fifo_tx, 8, 9 );

#if defined(HW_RTLS_V4_1_X)
    constexpr uint16_t timeout_mp_4000 = 1000;
    Mp_4000_Controller mp_4000_controller( delayGenerator, serialController, watchdog, timeout_mp_4000 );
    Sensor_mp_4000 mp_4000_sensor( &mp_4000_controller, mp_4000_baudrate, &w_serial_arduino, fifo_rx, serialController );
#endif

//Modo sleep en segundos, multiplos de 8s
uint32_t t_seconds = 60;
uint32_t max_intervals_8s = t_seconds / 8;
uint32_t intervals_8s = 0;

const uint16_t Pkt_size::payload_len  = 100;
Lossy lossy;
uint32_t timestamp = 0;
Data_formatter_interface* Payload_formatter::data_formatter_impl = &lossy;
Payload_mgr payload_mgr( Pkt_size::payload_len );

void setup( void ) {
    watchdog.watchdog_enable();
    debugPrinter1.config_serial_baud_rate( serial_baudrate );
    mp_4000_sensor.init();

    // Activacion alimentacion sensores A con bjt
    pinController.set_pin_mode_output( POWER_A_SENSORS );
    pinController.output_high( POWER_A_SENSORS );

    // Led de debug
    pinController.set_pin_mode_output( pin_led_debug );
    pinController.output_low( pin_led_debug );
}

void loop( void ) {
    debugPrinter1.serial_println( "Sleep" );
    delayGenerator.delay_ms( 1000 );
    pinController.output_low( pin_led_debug );

    while ( ( intervals_8s < max_intervals_8s ) && ( mp_4000_sensor.run_trigger() == false ) ) {
        LowPower.powerDown( SLEEP_8S, ADC_OFF, BOD_OFF );
        intervals_8s++;
    }
    pinController.output_high( pin_led_debug );
    intervals_8s = 0;

    mp_4000_sensor.init();

    debugPrinter1.serial_println( "Wakeup" );

    watchdog.watchdog_enable();

    payload_mgr.restart();
    mp_4000_sensor.measure();
    mp_4000_sensor.append( &payload_mgr, timestamp );
    payload_mgr.restart();
    debugPrinter1.serial_print( "Supply1 air temperature: " );
    debugPrinter1.serial_println( mp_4000_sensor.get_supply1_air_temperature() );
    debugPrinter1.serial_print( "Return air temperature: " );
    debugPrinter1.serial_println( mp_4000_sensor.get_return_air_temperature() );
    debugPrinter1.serial_print( "Temperature set point: " );
    debugPrinter1.serial_println( mp_4000_sensor.get_temperature_set_point() );
    debugPrinter1.serial_print( "Power state: " );
    debugPrinter1.serial_println( mp_4000_sensor.get_power_state() );
    debugPrinter1.serial_print( "Id_model: " );
    debugPrinter1.serial_println( payload_mgr.get_uint8() );
    debugPrinter1.serial_print( "Controller_timestamp: " );
    debugPrinter1.serial_println( ( uint32_t )payload_mgr.get_uint( timestamp_witrac, timestamp_max ) );
    debugPrinter1.serial_println("");
    mp_4000_sensor.on_ack();
    delayGenerator.delay_ms( 1000 );
}

