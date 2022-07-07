#include "murata93_serial_handler.h"
#include "conn_status_code.h"
#include <string.h>


Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
static uint8_t resp_msg_unknown[Murata93_ATCmd_evts::resp_msg_unknown_num][Murata93_ATCmd_evts::resp_msg_len_max];
static uint8_t resp_msg_unknown_i = 0;

Murata93_serial_handler::Murata93_serial_handler( Wtc_serial_interface& wtc_serial_murata93, Fifo<uint8_t>& fifo_rx, Fifo<uint8_t>& fifo_tx, uC_DelayGenerator& delayGenerator_0,
                                                  uC_SerialController& debugPrinter_0 ) :
    state( idle ),
    resp_buffer_i( 0 ),
    delayGenerator( delayGenerator_0 ),
    serialController( debugPrinter_0 ),
    wtc_serial_murata93( wtc_serial_murata93 ),
    fifo_rx( fifo_rx ),
    fifo_tx( fifo_tx ),
    resp_evt_queue( resp_evt_queue_max ) {
    event.params.version.version_ok = false;
    memset( resp_buffer, 0, Murata93_ATCmd_evts::resp_msg_len_max );
    memset( aux_buffer, 0, Murata93_ATCmd_evts::resp_msg_len_max );
}

Murata93_serial_handler::~Murata93_serial_handler(){};

void Murata93_serial_handler::run_task( void ) {
    uint8_t data = 0;

    while ( fifo_rx.available() > 0 ) {
        delayGenerator.delay_ms( 1 );
        fifo_rx.get( data );
        parse_rx_byte( data );
    }
}

void Murata93_serial_handler::parse_rx_byte( uint8_t data ) {
    serialController.serial_write( data );

    switch ( state ) {
        case idle: {
            memset( resp_buffer, 0, sizeof( resp_buffer ) );
            resp_buffer_i = 0;

            if ( data == char_idle_to_prepare_recv ) { // Llega un '+'
                resp_buffer[resp_buffer_i++] = data;
                state                        = wait_resp_msg;
            }
            break;
        }

        case wait_resp_msg: {
            if ( resp_buffer_i < Murata93_ATCmd_evts::resp_msg_len_max ) {
                resp_buffer[resp_buffer_i++] = data;
                if ( ( resp_buffer[resp_buffer_i - 3] == char_any_to_wait_resp_end ) && ( resp_buffer[resp_buffer_i - 2] == char_wait_resp_end_to_idle ) &&
                     ( resp_buffer[resp_buffer_i - 1] == char_any_to_wait_resp_end ) ) {
                    state = wait_resp_end;
                }
            }
            else {
                state = idle;
            }
            break;
        }

        case wait_resp_end: {
            if ( data == char_wait_resp_end_to_idle ) {
                bool resp_msm_error          = true;
                resp_buffer[resp_buffer_i++] = data;
                if ( resp_buffer_i > 0 ) {
                    if ( strstr_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_fw_version ) != NULL ) {
                        event.code                      = Murata93_ATCmd_evts::atcmd_resp_evt_code_fw_version;
                        event.params.version.version_ok = true;
                        resp_msm_error                  = false;
                    }
                    else if ( strncmp_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_msg_ok_with_params, sizeof( Murata93_ATCmd_evts::resp_msg_ok_with_params ) - 1 ) == 0 ) {
                        event.code = Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params;
                        // Si en el buffer despues de = tenemos un -, almacenamos rssi y snr "+OK=-90,6\r\n\r\n "
                        if ( resp_buffer[4] == '-' ) {
                            // Eliminamos signo negativo de p_rssi
                            char* p_rssi = strtok( (char*)resp_buffer, "-" );
                            if ( p_rssi != NULL ) {
                                p_rssi = strtok( NULL, "," );
                                if ( p_rssi != NULL ) {
                                    event.params.rf_parameters.rssi = atoi( p_rssi );
                                    char* p_snr                     = strtok( NULL, "\r" );
                                    if ( p_snr != NULL ) {
                                        event.params.rf_parameters.snr = atoi( p_snr );
                                        resp_msm_error                 = false;
                                    }
                                }
                            }
                        }
                        // Si el buffer tiene 9 bytes almacenamos el tipo de banda "+OK=5\r\n\r\n"
                        else if ( resp_buffer_i == 9 ) {
                            char* p_band = strtok( (char*)resp_buffer, "=" );
                            if ( p_band != NULL ) {
                                p_band = strtok( NULL, "\r" );
                                if ( p_band != NULL ) {
                                    event.params.type_band.band = atoi( p_band );
                                    resp_msm_error              = false;
                                }
                            }
                        }
                        // Si el buffer tiene 12 o 13 bytes almacenamos el baudrate ( 4800, 9600, 19200, 38400 ) "+OK=19200,8,1,0,0\r\n\r\n"
                        else if ( ( resp_buffer_i == 20 ) || ( resp_buffer_i == 21 ) ) {
                            char* p_baudrate = strtok( (char*)resp_buffer, "=" );
                            if ( p_baudrate != NULL ) {
                                p_baudrate = strtok( NULL, "," );
                                if ( p_baudrate != NULL ) {
                                    event.params.baud_rate.baudrate = atol( p_baudrate );
                                    if ( ( event.params.baud_rate.baudrate >= 4800 ) && ( event.params.baud_rate.baudrate <= 38400 ) ) {
                                        resp_msm_error = false;
                                    }
                                }
                            }
                        }
                        // Si el buffer tiene 24 bytes almacenamos el eui "+OK=01020304AABBCCDD\r\n\r\n"
                        else {
                            if ( resp_buffer_i == 24 ) {
                                char* p_eui = strtok( (char*)resp_buffer, "=" );
                                if ( p_eui != NULL ) {
                                    p_eui = strtok( NULL, "\r" );
                                    if ( p_eui != NULL ) {
                                        event.params.deveui.eui = (uint8_t*)( p_eui );
                                        resp_msm_error          = false;
                                    }
                                }
                            }
                        }
                    }
                    else if ( strncmp_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_msg_ok, sizeof( Murata93_ATCmd_evts::resp_msg_ok ) - 1 ) == 0 ) {
                        event.code     = Murata93_ATCmd_evts::atcmd_resp_evt_code_ok;
                        resp_msm_error = false;
                    }
                    else if ( strncmp_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_msg_error, sizeof( Murata93_ATCmd_evts::resp_msg_error ) - 1 ) == 0 ) {
                        event.code    = Murata93_ATCmd_evts::atcmd_resp_evt_code_error;
                        char* p_error = strtok( (char*)resp_buffer, "=" );
                        if ( p_error != NULL ) {
                            p_error = strtok( NULL, "\0" );
                            if ( p_error != NULL ) {
                                event.params.error_type.error = atoi( p_error );
                                resp_msm_error                = false;
                            }
                        }
                    }
                    else if ( strncmp_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_msg_ack, sizeof( Murata93_ATCmd_evts::resp_msg_ack ) - 1 ) == 0 ) {
                        event.code                    = Murata93_ATCmd_evts::atcmd_resp_evt_code_ack;
                        event.params.ack_response.ack = 1;
                        resp_msm_error                = false;
                    }
                    else if ( strncmp_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_msg_noack, sizeof( Murata93_ATCmd_evts::resp_msg_noack ) - 1 ) == 0 ) {
                        event.code                    = Murata93_ATCmd_evts::atcmd_resp_evt_code_noack;
                        event.params.ack_response.ack = 0;
                        resp_msm_error                = false;
                    }
                    else if ( strncmp_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_msg_conection, sizeof( Murata93_ATCmd_evts::resp_msg_conection ) - 1 ) == 0 ) {
                        /* Events codes module status
                         * reboots_ok->          type 0, event_number 0
                         *
                         * Events codes join result
                         * join_rejected->       type 1, event_number 0
                         * join_accepted->       type 1, event_number 1
                         *
                         * Events codes network status
                         * link_lost->           type 2, event_number 0
                         * link_connected->      type 2, event_number 1
                         * module_status_noack-> type 2, event_number 2
                         */

                        event.code = Murata93_ATCmd_evts::atcmd_resp_evt_code_event;

                        char* p_type = strtok( (char*)resp_buffer, "=" );
                        if ( p_type != NULL ) {
                            p_type = strtok( NULL, "," );
                            if ( p_type != NULL ) {
                                event.params.device_status.type = atoi( p_type );
                                char* p_event_number            = strtok( NULL, "\r" );
                                if ( p_event_number != NULL ) {
                                    event.params.device_status.event_number = atoi( p_event_number );
                                    if ( ( event.params.device_status.type == 1 ) && ( event.params.device_status.event_number == 1 ) ) {
                                        event.params.device_status.status = Conn_status::cipstatus_code_attached;
                                    }
                                    else if ( ( event.params.device_status.type == 2 ) && ( event.params.device_status.event_number == 1 ) ) {
                                        event.params.device_status.status = Conn_status::cipstatus_code_connected;
                                    }
                                    else {
                                        event.params.device_status.status = Conn_status::cipstatus_code_unattached;
                                    }
                                    resp_msm_error = false;
                                }
                            }
                        }
                    }
                    else if ( strncmp_P( (char*)resp_buffer, Murata93_ATCmd_evts::resp_msg_received, sizeof( Murata93_ATCmd_evts::resp_msg_received ) - 1 ) == 0 ) {
                        event.code = Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data;

                        char* p_port = strtok( (char*)resp_buffer, "=" );
                        if ( p_port != NULL ) {
                            p_port = strtok( NULL, "," );
                            if ( p_port != NULL ) {
                                event.params.received_data.port = atoi( p_port );
                                char* p_length                  = strtok( NULL, "\r" );
                                if ( p_length != NULL ) {
                                    event.params.received_data.length = atoi( p_length );
                                    resp_msm_error                    = false;
                                }
                            }
                        }
                    }
                    else {
                        event.code = Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp;
                        // Eliminamos \r\n\r\n
                        resp_buffer_i -= 4;
                        memcpy( resp_msg_unknown[resp_msg_unknown_i], resp_buffer, resp_buffer_i );
                        resp_msg_unknown[resp_msg_unknown_i][resp_buffer_i] = '\0';
                        event.params.unknown_response.p_resp                = resp_msg_unknown[resp_msg_unknown_i];
                        resp_msg_unknown_i                                  = ( resp_msg_unknown_i + 1 ) % Murata93_ATCmd_evts::resp_msg_unknown_num;
                        event.params.unknown_response.resp_len              = resp_buffer_i;
                        resp_msm_error                                      = false;
                    }

                    if ( resp_msm_error ) {
                        state = idle;
                    }
                    else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data ) {
                        state = save_data;
                        memset( resp_buffer, 0, sizeof( resp_buffer ) );
                        memset( aux_buffer, 0, sizeof( aux_buffer ) );
                        resp_buffer_i = 0;
                    }
                    else {
                        resp_evt_queue.put( event );
                        state = idle;
                    }
                }
            }
            else {
                state = idle;
            }
            break;
        }

        case save_data: {
            if ( data >= '0' && data <= '9' ) {
                data = data - '0';
            }
            if ( data >= 'A' && data <= 'F' ) {
                data = data - 55;
            }
            resp_buffer[resp_buffer_i++] = data;
            if ( resp_buffer_i >= ( event.params.received_data.length ) * 2 ) {
                uint8_t j = 0;
                for ( uint8_t i = 0; i < event.params.received_data.length; i++ ) {
                    aux_buffer[i] = resp_buffer[j] << 4 | resp_buffer[j + 1];
                    j += 2;
                }
                event.params.received_data.response = aux_buffer;
                resp_evt_queue.put( event );
                state = idle;
            }
            break;
        }

        default:
            break;
    }
}

uint8_t Murata93_serial_handler::resp_evt_available( void ) {
    return resp_evt_queue.available();
}

bool Murata93_serial_handler::resp_evt_get( Murata93_ATCmd_evts::ATCmd_resp_evt_t& resp_evt ) {
    return resp_evt_queue.get( resp_evt );
}

Murata93_ATCmd_evts::ATCmd_resp_evt_t Murata93_serial_handler::resp_evt_peek( uint8_t idx ) {
    return resp_evt_queue[idx];
}

void Murata93_serial_handler::reset( void ) {
    resp_evt_queue.reset();
    state = idle;
}

void Murata93_serial_handler::flush( void ) {
    fifo_rx.reset();
    state = idle;
}