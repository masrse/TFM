#include "sensor_mp_4000.h"
#include "data_formatter_interface.h"
#include "wtc_debug.h"

Sensor_mp_4000::Sensor_mp_4000( Mp_4000_Controller* mp_4000_0, uint16_t baudrate_mp_4000_0, Wtc_serial_interface* wtc_serial_0, Fifo<uint8_t>& fifo_rx_0, uC_SerialController& debugPrinter_0 ):
    Sensor( measure_time ),
    mp_4000( mp_4000_0 ),
    supply1_air_temperature( 0 ),
    return_air_temperature( 0 ),
    temperature_set_point( 0 ),
    power_state( 0 ),
    trigger_state( idle ),
    baudrate_mp_4000( baudrate_mp_4000_0 ),
    fifo_rx( fifo_rx_0 ),
    w_serial( wtc_serial_0 ),
    serialController( debugPrinter_0 )  {}

Sensor_mp_4000::~Sensor_mp_4000( void ) {
}

void Sensor_mp_4000::init( void ) {
    w_serial->stopListening();
    if ( trigger_state == idle ) {
        #if not defined( debug_print )
            mp_4000->init( baudrate_mp_4000 );
        #endif
    }
    else {
        #if defined( debug_print )
            serialController.config_serial_baud_rate( baudrate_mp_4000 );
        #else
            mp_4000->init( baudrate_mp_4000 );
        #endif
    }
    trigger_state = go_to_sleep;
    reply_sensor_read_out = false;
    reply_control_read_out = false;
    measure();
}

void Sensor_mp_4000::measure( void ) {
    if ( reply_sensor_read_out == false ) {
        reply_sensor_read_out = mp_4000->set_sensor_read_out();
        if ( reply_sensor_read_out == true ) {
            mp_4000->get_supply1_air_temperature( supply1_air_temperature );
            mp_4000->get_return_air_temperature( return_air_temperature );
        }
    }

    if ( reply_control_read_out == false ) {
        reply_control_read_out = mp_4000->set_control_read_out();
        if ( reply_control_read_out == true ) {
            mp_4000->get_temperature_set_point( temperature_set_point );
            mp_4000->get_power_state( power_state );
        }
    }
}

void Sensor_mp_4000::append( Payload_mgr* p_payload_mgr, const uint32_t timestamp ) {
    model_mp_4000.timestamp = timestamp;
    model_mp_4000.supply1_air_temperature = supply1_air_temperature;
    model_mp_4000.return_air_temperature = return_air_temperature;
    model_mp_4000.temperature_set_point = temperature_set_point;
    model_mp_4000.power_state = power_state;
    model_mp_4000.to_pkt_payload( p_payload_mgr );
    reply_sensor_read_out = false;
    reply_control_read_out = false;
}

void Sensor_mp_4000::on_ack( void ) {
    supply1_air_temperature = 0;
    return_air_temperature = 0;
    temperature_set_point = 0;
    power_state = 0;
    // reply_sensor_read_out = false;  TODO: descomentar cuando el on_ack se llame siempre aunque no se entre en sleep
    // reply_control_read_out = false;
}

uint8_t Sensor_mp_4000::run_trigger( void ) {
    switch ( trigger_state ) {
        case idle: {
            trigger_state = go_to_sleep;
            break;
        }
       case go_to_sleep: {
            w_serial->begin( baudrate_mp_4000 );
            fifo_rx.reset();
            if ( w_serial->isListening() == false ) {
                w_serial->listen();
            }
            trigger_state = waiting_to_get_out_off_sleep;
            break;
        }
        case waiting_to_get_out_off_sleep: {
            if ( fifo_rx.available() ) {
                return code_wake_up_fast;
            }
            break;
        }
        default: {
            break;
        }
    }
    return code_sleep;
}