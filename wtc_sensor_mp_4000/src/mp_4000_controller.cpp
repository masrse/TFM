#include "mp_4000_controller.h"
#include "crc.h"
#include "string.h"
#include "stdlib.h"

Mp_4000_Controller::Mp_4000_Controller( uC_DelayGenerator& delayGenerator_0, uC_SerialController& debugPrinter_0, uC_wdt_interface& watchdog_0, uint16_t timeout_sensor_0 ):
    state( idle ),
    timeout_sensor( timeout_sensor_0 ),
    delayGenerator( delayGenerator_0 ),
    serialController( debugPrinter_0 ),
    watchdog( watchdog_0 ) {
}

Mp_4000_Controller::~Mp_4000_Controller( void ) {
}

void Mp_4000_Controller::init( uint16_t baudrate_controller ) {
    serialController.config_serial_baud_rate( baudrate_controller );
}

bool Mp_4000_Controller::set_sensor_read_out( void ) {
    uint16_t checksum = 0;
    uint8_t cmd_sensor_read_out[] = { start_esc, start_sof, type, address_h, address_l,
                                      format_data, cmd_data, sub_cmd_data, 
                                      request_cmd_data_sensor, 0, 0, end_esc, end_eof };

    // The CRC16 is generated holding the CRC16 of the byte starting with Frame type (FT) and ending with data (Byte n)
    checksum = CRC16::calculate_crc( ( uint8_t* ) ( cmd_sensor_read_out + 2 ) , sizeof( cmd_sensor_read_out ) - 6 );

    cmd_sensor_read_out[ 9 ] = ( uint8_t ) ( checksum >> 8 );
    cmd_sensor_read_out[ 10 ] = ( uint8_t ) ( checksum );

    serialController.serial_write( cmd_sensor_read_out, sizeof( cmd_sensor_read_out ) );

    //Reset params
    reply_params.reply_sensor.supply_air1_temperature = 0;
    reply_params.reply_sensor.return_air_temperature = 0;

    return wait_reply_controller( timeout_sensor );
}

bool Mp_4000_Controller::set_control_read_out( void ) {
    uint16_t checksum = 0;
    uint8_t cmd_control_read_out[] = { start_esc, start_sof, type, address_h, address_l,
                                      format_data, cmd_data, sub_cmd_data, 
                                      request_cmd_data_control, 0, 0, end_esc, end_eof };

    // The CRC16 is generated holding the CRC16 of the byte starting with Frame type (FT) and ending with data (Byte n) 
    checksum = CRC16::calculate_crc( ( uint8_t* ) ( cmd_control_read_out + 2 ) , sizeof( cmd_control_read_out ) - 6 );

    cmd_control_read_out[ 9 ] = ( uint8_t ) ( checksum >> 8 );
    cmd_control_read_out[ 10 ] = ( uint8_t ) ( checksum );

    serialController.serial_write( cmd_control_read_out, sizeof( cmd_control_read_out ) );

    //Reset params
    reply_params.reply_control.temperature_set_point = 0;
    reply_params.reply_control.power_state = 0;

    return wait_reply_controller( timeout_sensor );
}

bool Mp_4000_Controller::wait_reply_controller( uint16_t timeout_ms ) {
    int16_t timeout = timeout_ms;

    reply_params.got_reply = false;

    do {
        while ( serialController.serial_available() ) {
            uint8_t data_byte;
            serialController.serial_read( &data_byte );
            parse_reply_byte( data_byte );
        }
        watchdog.watchdog_reset();
        delayGenerator.delay_ms( 1 );
    } while ( ( timeout-- > 0 ) && !reply_params.got_reply );

    return reply_params.got_reply;
}

void Mp_4000_Controller::parse_reply_byte( uint8_t byte ) {
    switch ( state ) {
        case idle: {
            reply_vector_i = 0;

            if ( byte == start_esc ) { // ESC START byte
                reply_vector[ reply_vector_i++ ] = byte;
                state = wait_start_reply;
            }
            break;
        }

        case wait_start_reply: {
            if ( byte == start_sof ) { // SOF START byte
                reply_vector[ reply_vector_i++ ] = byte;
                state = wait_reply_end;
            }
            else {
                memset( reply_vector, 0, sizeof( reply_vector ) );
                state = idle;
            }
            break;
        }

        case wait_reply_end: {
            if ( reply_vector_i < reply_vector_lenght ) {
                // Check byte stuffer and delete it if it exist
                if ( byte == byte_stuffed ) {
                    if ( reply_vector[reply_vector_i - 1] != byte_stuffed ) {
                        reply_vector[reply_vector_i++] = byte;
                    }
                }
                else {
                    reply_vector[reply_vector_i++] = byte;
                }

                // Wait ESC and EOF end bytes 
                if ( ( reply_vector[ reply_vector_i - 2 ] == end_esc ) && ( reply_vector[ reply_vector_i - 1 ] == end_eof ) ) {
                    if ( ( reply_vector_i >= sensor_read_out_len && reply_vector[ 7 ] == request_cmd_data_sensor ) || ( reply_vector_i >= control_read_out_len && reply_vector[ 7 ] == request_cmd_data_control ) ) {
                        uint16_t checksum = 0;
                        //If the CRC16 is generated over the CRC data area and the CRC itself, the new CRC holds the value 0.
                        checksum = CRC16::calculate_crc( ( uint8_t* ) ( reply_vector + 2 ) , reply_vector_i - 4 );
                        if ( checksum == 0 ) {
                            if ( reply_vector[ 7 ] == request_cmd_data_sensor ) {
                                reply_params.reply_sensor.supply_air1_temperature = reply_vector[ first_num_byte_data_sensor + Num_byte_reply_sensor::supply_air1_temperature_H ];
                                reply_params.reply_sensor.supply_air1_temperature <<= 8;
                                reply_params.reply_sensor.supply_air1_temperature |= reply_vector[ first_num_byte_data_sensor + Num_byte_reply_sensor::supply_air1_temperature_L ];
                                reply_params.reply_sensor.return_air_temperature = reply_vector[ first_num_byte_data_sensor + Num_byte_reply_sensor::return_air_temperature_H ];
                                reply_params.reply_sensor.return_air_temperature <<= 8;
                                reply_params.reply_sensor.return_air_temperature |= reply_vector[ first_num_byte_data_sensor + Num_byte_reply_sensor::return_air_temperature_L ];
                                reply_params.got_reply = true;
                            }
                            else if ( reply_vector[ 7 ] == request_cmd_data_control ) {
                                reply_params.reply_control.temperature_set_point = reply_vector[ first_num_byte_data_control + Num_byte_reply_control::temperature_set_point_H ];
                                reply_params.reply_control.temperature_set_point <<= 8;
                                reply_params.reply_control.temperature_set_point |= reply_vector[ first_num_byte_data_control + Num_byte_reply_control::temperature_set_point_L ];
                                reply_params.reply_control.power_state = reply_vector[ first_num_byte_data_control + Num_byte_reply_control::power_state ];
                                reply_params.got_reply = true;
                            }
                        }
                        memset( reply_vector, 0, sizeof( reply_vector ) );
                        state = idle;
                    }
                }
            }
            else {
                memset( reply_vector, 0, sizeof( reply_vector ) );
                state = idle;
            }
            break;
        }

        default:
            break;
    }
}