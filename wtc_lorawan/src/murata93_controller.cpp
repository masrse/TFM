#include "murata93_controller.h"

#include "stdlib.h"
#include "conn_status_code.h"
#include "router.h"
#include "pkt_filter.h"

Murata93_base_controller::Murata93_base_controller( Wtc_serial_interface& lora_serial, Fifo<uint8_t>& fifo_rx_0, Fifo<uint8_t>& fifo_tx_0, const uint8_t reset_pin_0,
                                                    const baud_rate_t module_serial_baudrate_0, uC_SerialController& debugPrinter_0, uC_PinController& pinController_0,
                                                    uC_DelayGenerator& uC_delay_0, uC_wdt_interface& watchdog_0 ) :
    p_handler_murata93( new Murata93_serial_handler( lora_serial, fifo_rx_0, fifo_tx_0, uC_delay_0, debugPrinter_0 ) ),
    serial_murata93( lora_serial ),
    fifo_rx( fifo_rx_0 ),
    fifo_tx( fifo_tx_0 ),
    reset_pin( reset_pin_0 ),
    module_serial_baudrate( module_serial_baudrate_0 ),
    serialController( debugPrinter_0 ),
    pinController( pinController_0 ),
    uC_delay( uC_delay_0 ),
    watchdog( watchdog_0 ) {
    pinController.set_pin_mode_output( reset_pin );
    pinController.output_high( reset_pin );

    serial_murata93.begin( (baud_rate_t)module_serial_baudrate );
}

Murata93_base_controller::~Murata93_base_controller( void ) {
    delete ( p_handler_murata93 );
}

void Murata93_base_controller::run_task( void ) {
    p_handler_murata93->run_task();
}

Wtc_err_t Murata93_base_controller::resp_evt_wait_next( Murata93_ATCmd_evts::ATCmd_resp_evt_t& resp_evt, const uint32_t timeout_ms ) {
    bool got_any_event = false;

    int32_t timeout = timeout_ms;
    do {
        if ( p_handler_murata93->resp_evt_available() ) {
            p_handler_murata93->resp_evt_get( resp_evt );
            got_any_event = true;
        }
        run_task();
        watchdog.watchdog_reset();
        uC_delay.delay_ms( 1 );
    } while ( ( timeout-- > 0 ) && !got_any_event );

    if ( !got_any_event ) {
        return wtc_err_timeout;
    }

    return wtc_success;
}

void Murata93_base_controller::handle_all_queued_events( void ) {
#ifndef TEST_FLAG
    run_task();
    while ( p_handler_murata93->resp_evt_available() > 0 ) {
        Murata93_ATCmd_evts::ATCmd_resp_evt_t unexp_evt;
        p_handler_murata93->resp_evt_get( unexp_evt );
        handle_unexpected_event( unexp_evt );
    }
    // Vaciamos el buffer
    p_handler_murata93->flush();
#endif
}

void Murata93_base_controller::handle_unexpected_event( const Murata93_ATCmd_evts::ATCmd_resp_evt_t& event ) {
    // Añadir comandos que se sabe que el módulo manda, pero con los que en principio, no se hacen nada
}

Wtc_err_t Murata93_base_controller::set_data_format( const uint8_t format ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+DFORMAT=" ) << format << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_duty_cycle( const uint8_t state ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+DUTYCYCLE=" ) << state << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_rfpower( const uint8_t mode, const uint8_t index ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+RFPOWER=" ) << mode << PSTR( "," ) << index << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_activation_mode( const uint8_t activation_mode ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+MODE=" ) << activation_mode << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_appeui( void ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_err_comms;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+APPEUI=" ) << const_cast<char*>( appeui ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

void Murata93_base_controller::get_appeui( char* buffer ) {
    memcpy( buffer, appeui, strlen( appeui ) + 1 );
}

Wtc_err_t Murata93_base_controller::set_appkey( void ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_err_comms;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+APPKEY=" ) << const_cast<char*>( appkey ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

void Murata93_base_controller::get_appkey( char* buffer ) {
    memcpy( buffer, appkey, strlen( appkey ) + 1 );
}

Wtc_err_t Murata93_base_controller::join( void ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_err_comms;
    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+JOIN" ) << Murata93_ATCmd_cmds::endl;
    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            result = resp_evt_wait_specific( event, atresp_timeout_net_oper_ms, Murata93_ATCmd_evts::atcmd_resp_evt_code_event, true );
            if ( ( result == wtc_success ) && ( event.params.device_status.status == Conn_status::cipstatus_code_attached ) ) {
                return wtc_success;
            }
            else {
                return wtc_err_comms;
            }
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_data_rate( const uint8_t datarate ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+DR=" ) << datarate << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::check_version( bool& version ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;
    version          = false;
    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+VER?" ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_fw_version ) {
            if ( event.params.version.version_ok ) {
                version = true;
                return result;
            }
            return wtc_err_params;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::get_deveui( uint8_t deveui[] ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result   = wtc_success;
    uint8_t len_deveui = 16;
    memset( deveui, 0, len_deveui );
    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+DEVEUI?" ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params ) {
            for ( uint8_t i = 0; i < len_deveui; i++ ) {
                deveui[i] = event.params.deveui.eui[i];
            }
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_deveui( uint32_t deveui ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+DEVEUI=" );

    // Guarda el deveui en formato hexadecimal
    for ( uint8_t i = 0; i < 4; i++ ) {
        p_handler_murata93->write_byte_to_serial_in_hex( 0 );
        p_handler_murata93->write_byte_to_serial_in_hex( 0 );
    }

    uint8_t deveui_vector[4];
    deveui_vector[0] = (uint8_t)( deveui >> 24 );
    deveui_vector[1] = (uint8_t)( deveui >> 16 );
    deveui_vector[2] = (uint8_t)( deveui >> 8 );
    deveui_vector[3] = (uint8_t)( deveui );

    for ( uint8_t i = 0; i < sizeof( deveui_vector ); i++ ) {
        if ( deveui_vector[i] < 16 ) {
            p_handler_murata93->write_byte_to_serial_in_hex( 0 );
            p_handler_murata93->write_byte_to_serial_in_hex( deveui_vector[i] );
        }
        else {
            p_handler_murata93->write_byte_to_serial_in_hex( deveui_vector[i] );
        }
    }

    *p_handler_murata93 << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::get_band( uint8_t& band ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;
    band             = 0;
    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+BAND?" ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params ) {
            band = event.params.type_band.band;
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_band( const uint8_t band ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+BAND=" ) << band << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::get_rssi( uint8_t& rssi, uint8_t& snr ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+RFQ?" ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params ) {
            rssi = event.params.rf_parameters.rssi;
            snr  = event.params.rf_parameters.snr;
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::send( const Conn_config::Conn_id_t conn_id, const uint8_t buffer[], const uint16_t len ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    Pkt_filter pkt_filter( buffer, len );

    if ( !pkt_filter.filter_pkt( model_id_mp_4000, model_id_powered, model_id_battery, model_id_location_info ) ) {
        return wtc_err_comms;
    }

    uint16_t size_pkt_filter = pkt_filter.pkt_filtered.get_size();

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    command_send( size_pkt_filter );

    // Envia el pkt en formato hexadecimal
    for ( uint8_t i = 0; i < size_pkt_filter; i++ ) {
        if ( pkt_filter.pkt_filtered.bytes()[i] < 16 ) {
            p_handler_murata93->write_byte_to_serial_in_hex( 0 );
            p_handler_murata93->write_byte_to_serial_in_hex( pkt_filter.pkt_filtered.bytes()[i] );
        }
        else {
            p_handler_murata93->write_byte_to_serial_in_hex( pkt_filter.pkt_filtered.bytes()[i] );
        }
    }

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::reboot( void ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+REBOOT" ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            result = resp_evt_wait_specific( event, atresp_timeout_net_oper_ms, Murata93_ATCmd_evts::atcmd_resp_evt_code_event );
            if ( ( result == wtc_success ) && ( event.params.device_status.status == Conn_status::cipstatus_code_unattached ) ) {
                return wtc_success;
            }
            else {
                return wtc_err_comms;
            }
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::get_status_connection( uint8_t& status ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_err_comms;
    status           = Conn_status::cipstatus_code_unattached;
    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+LNCHECK" ) << Murata93_ATCmd_cmds::endl;
    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            result = resp_evt_wait_specific( event, atresp_timeout_net_oper_ms, Murata93_ATCmd_evts::atcmd_resp_evt_code_event, true );
            if ( result == wtc_success ) {
                status = event.params.device_status.status;
                return wtc_success;
            }
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
    }
    return result;
}

Wtc_err_t Murata93_base_controller::set_baudrate( baud_rate_t baud_rate ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+UART=" ) << (uint32_t)baud_rate << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok ) {
            return wtc_success;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

Wtc_err_t Murata93_base_controller::get_baudrate( baud_rate_t& baud_rate ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result;

    // Primero vacia la cola si hay algun evento pendiente
    handle_all_queued_events();

    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+UART?" ) << Murata93_ATCmd_cmds::endl;

    result = resp_evt_wait_next( event, atresp_timeout_default_ms );
    if ( result == wtc_success ) {
        if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params ) {
            baud_rate = (baud_rate_t)event.params.baud_rate.baudrate;
            return result;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_error ) {
            return wtc_err_command;
        }
        else if ( event.code == Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp ) {
            handle_unexpected_event( event );
            return wtc_err_comms;
        }
        result = wtc_err_comms;
    }
    return result;
}

bool Murata93_base_controller::check_baud_rate( baud_rate_t baud_rate ) {
    uint8_t num_baudrates_checked = 4;
    Wtc_err_t result;
    baud_rate_t baud_rate_read;

    serial_murata93.begin( (baud_rate_t)baud_rate );

    result = get_baudrate( baud_rate_read );

    // Cuando al baudrate coincide con el configurado y se han enviado comandos a un baudrate diferente
    // el murata responde con +ERR=-1, es necesario enviar comandos con el baudrate al que responde hasta
    // conseguir el baudrate.
    if ( result == wtc_err_command ) {
        for ( uint8_t i = 0; result != wtc_success && i < num_baudrates_checked; i++ ) {
            result = get_baudrate( baud_rate_read );
        }
    }

    if ( ( result == wtc_success ) && ( baud_rate_read == baud_rate ) ) {
        return true;
    }

    return false;
}

bool Murata93_base_controller::detect_baud_rate( baud_rate_t& detected_baud_rate ) {
    // vector with the baudrates that will be checked
    baud_rate_t baud_vector[] = {
        // ordered by most common first
        baud_rate_19200, // default baud rate
        baud_rate_4800,
        baud_rate_9600,
        baud_rate_38400,
    };

    const uint8_t elements_number = ( sizeof( baud_vector ) / sizeof( baud_vector[0] ) );
    bool found_baudrate           = false;

    for ( uint8_t i = 0; found_baudrate == false && i < elements_number; i++ ) {
        detected_baud_rate = baud_vector[i];
        if ( check_baud_rate( detected_baud_rate ) == true ) {
            found_baudrate = true;
        }
    }

    return found_baudrate;
}

bool Murata93_base_controller::change_baud_rate( const baud_rate_t new_baud_rate ) {
    baud_rate_t detected_baud_rate;

    if ( detect_baud_rate( detected_baud_rate ) ) {
        if ( new_baud_rate == detected_baud_rate ) {
            return true;
        }
    }

    serialController.serial_print( "Cambio baudrate a: " );
    serialController.serial_println( (uint32_t)new_baud_rate );
    if ( set_baudrate( new_baud_rate ) == wtc_success ) {
        serialController.serial_println( "set_baudrate ok" );
        // Cuando el baudrate es modificado es necesario realizar un reset para validar la nueva configuracion.
        reboot();
        if ( check_baud_rate( new_baud_rate ) == true ) {
            serialController.serial_println( "reboot ok" );
            return true;
        }
    }

    return false;
}

Wtc_err_t Murata93_base_controller::receive( const Conn_config::Conn_id_t conn_id, uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout ) {
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Wtc_err_t result = wtc_success;

    result = wait_receive_command( event, timeout );
    if ( result != wtc_success ) {
        handle_unexpected_event( event );
        return result;
    }

    // Nos aseguramos de tener capacidad suficiente para todos los datos
    if ( event.params.received_data.length > max_len ) {
        // Vaciamos el buffer
        p_handler_murata93->flush();
        len = 0;
        return wtc_err_params;
    }

    copy_receive_msg( event, buffer, len );

    return wtc_success;
}

Wtc_err_t Murata93_base_controller::check_attach( void ) {
    uint8_t status_connection = 0;
    return get_status_connection( status_connection );
}

Wtc_err_t Murata93_base_controller::conn_status( const Conn_config::Conn_id_t conn_id, uint8_t& status ) {
    Wtc_err_t result;
    uint8_t status_connection = 0;
    result                    = get_status_connection( status_connection );
    status                    = status_connection;
    return result;
}

void Murata93_base_controller::on( void ) {
    p_handler_murata93->reset();
    listen();
    serial_murata93.begin( (baud_rate_t)module_serial_baudrate );
    // Vaciamos el buffer
    p_handler_murata93->flush();
    // Configuracion por defecto
    Wtc_err_t result = wtc_err_init;
    for ( uint8_t i = 0; result != wtc_success && i < 3; i++ ) {
        result = set_band( 5 );
    }
    result = wtc_err_init;
    for ( uint8_t i = 0; result != wtc_success && i < 3; i++ ) {
        result = set_data_format( 1 );
    }
    result = wtc_err_init;
    for ( uint8_t i = 0; result != wtc_success && i < 3; i++ ) {
        result = set_duty_cycle( 0 );
    }
    result = wtc_err_init;
    for ( uint8_t i = 0; result != wtc_success && i < 3; i++ ) {
        result = set_rfpower( 0, 1 );
    }
    result = wtc_err_init;
    for ( uint8_t i = 0; result != wtc_success && i < 3; i++ ) {
        result = set_activation_mode( 1 );
    }
    result = wtc_err_init;
    for ( uint8_t i = 0; result != wtc_success && i < 3; i++ ) {
        result = set_data_rate( 3 );
    }
}

void Murata93_base_controller::off( void ){};

Wtc_err_t Murata93_base_controller::resp_evt_wait_specific( Murata93_ATCmd_evts::ATCmd_resp_evt_t& resp_evt, const uint32_t timeout_ms,
                                                            const Murata93_ATCmd_evts::ATCmd_resp_evt_code_t expected_code, const bool ignore_others ) {
    bool got_any_event      = false;
    bool got_expected_event = false;
    memset( (void*)&resp_evt, 0, sizeof( resp_evt ) );

    int32_t timeout = timeout_ms;
    do {
        if ( p_handler_murata93->resp_evt_available() ) {
            p_handler_murata93->resp_evt_get( resp_evt );
            got_any_event = true;
            if ( resp_evt.code == expected_code ) {
                got_expected_event = true;
            }
            else {
                handle_unexpected_event( resp_evt );
            }
        }

        run_task();
        watchdog.watchdog_reset();
        uC_delay.delay_ms( 1 );

    } while ( ( timeout-- > 0 ) && ( ( !got_expected_event ) && ( !got_any_event || ignore_others ) ) );

    return ( got_expected_event ) ? wtc_success : wtc_err_timeout;
}

bool Murata93_base_controller::is_data_pending( const Conn_config::Conn_id_t conn_id ) {
    run_task();

    if ( p_handler_murata93->resp_evt_available() > 0 ) {
        if ( p_handler_murata93->resp_evt_peek( 0 ).code == Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data ) {
            return ( conn_id == p_handler_murata93->resp_evt_peek( 0 ).params.received_data.port );
        }
    }

    return false;
}

void Murata93_base_controller::listen( void ) {
    if ( serial_murata93.isListening() == false ) {
        serial_murata93.listen();
    }
}

Id_controller_t Murata93_base_controller::get_id_controller( void ) {
    return id_controller_murata93;
}

Wtc_err_t Murata93_base_controller::connect( const Conn_config::Conn_id_t conn_id, const char* ip, const uint16_t port, const Conn_config::Conn_type_t type ) {
    return wtc_success;
}

Wtc_err_t Murata93_base_controller::set_params( char* arg1, char* arg2 ) {
    Wtc_err_t result_appeui = wtc_err_comms;
    Wtc_err_t result_appkey = wtc_err_comms;
    memcpy( appeui, arg1, strlen( arg1 ) + 1 );
    memcpy( appkey, arg2, strlen( arg2 ) + 1 );
    listen();
    result_appeui = set_appeui();
    result_appkey = set_appkey();
    if ( ( result_appeui == wtc_success ) && ( result_appkey == wtc_success ) ) {
        return wtc_success;
    }
    return wtc_err_comms;
}

Wtc_err_t Murata93_base_controller::disconnect( const Conn_config::Conn_id_t conn_id ) {
    return wtc_success;
}

Wtc_err_t Murata93_base_controller::init( void ) {
    Wtc_err_t retval_eui = set_appeui();
    Wtc_err_t retval_key = set_appkey();
    if ( retval_eui == wtc_success && retval_key == wtc_success ) {
        return wtc_success;
    }
    return wtc_err_comms;
}

Murata93_controller::Murata93_controller( Wtc_serial_interface& lora_serial, Fifo<uint8_t>& fifo_rx_0, Fifo<uint8_t>& fifo_tx_0, const uint8_t reset_pin_0,
                                          const baud_rate_t module_serial_baudrate_0, uC_SerialController& debugPrinter_0, uC_PinController& pinController_0,
                                          uC_DelayGenerator& uC_delay_0, uC_wdt_interface& watchdog_0 ) :
    Murata93_base_controller( lora_serial, fifo_rx_0, fifo_tx_0, reset_pin_0, module_serial_baudrate_0, debugPrinter_0, pinController_0, uC_delay_0, watchdog_0 ) {}

Murata93_controller::~Murata93_controller() {}

void Murata93_controller::command_send( uint16_t size_pkt ) {
    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+UTX " ) << size_pkt << Murata93_ATCmd_cmds::endl;
}

Wtc_err_t Murata93_controller::wait_receive_command( Murata93_ATCmd_evts::ATCmd_resp_evt_t& event, const uint32_t timeout ) {
    return resp_evt_wait_specific( event, timeout, Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data, true );
}
void Murata93_controller::copy_receive_msg( Murata93_ATCmd_evts::ATCmd_resp_evt_t event, uint8_t buffer[], uint16_t& length_buffer ) {
    length_buffer = event.params.received_data.length;

    for ( uint_fast8_t i = 0; i < length_buffer; i++ ) {
        buffer[i] = event.params.received_data.response[i];
    }
}

Murata93_controller_autoack::Murata93_controller_autoack( Wtc_serial_interface& lora_serial, Fifo<uint8_t>& fifo_rx_0, Fifo<uint8_t>& fifo_tx_0, const uint8_t reset_pin_0,
                                                          const baud_rate_t module_serial_baudrate_0, uC_SerialController& debugPrinter_0, uC_PinController& pinController_0,
                                                          uC_DelayGenerator& uC_delay_0, uC_wdt_interface& watchdog_0 ) :
    Murata93_base_controller( lora_serial, fifo_rx_0, fifo_tx_0, reset_pin_0, module_serial_baudrate_0, debugPrinter_0, pinController_0, uC_delay_0, watchdog_0 ) {}

Murata93_controller_autoack::~Murata93_controller_autoack() {}

void Murata93_controller_autoack::command_send( uint16_t size_pkt ) {
    *p_handler_murata93 << Murata93_ATCmd_cmds::at << PSTR( "+CTX " ) << size_pkt << Murata93_ATCmd_cmds::endl;
}

Wtc_err_t Murata93_controller_autoack::wait_receive_command( Murata93_ATCmd_evts::ATCmd_resp_evt_t& event, const uint32_t timeout ) {
    return resp_evt_wait_specific( event, timeout, Murata93_ATCmd_evts::atcmd_resp_evt_code_ack, true );
}
void Murata93_controller_autoack::copy_receive_msg( Murata93_ATCmd_evts::ATCmd_resp_evt_t event, uint8_t buffer[], uint16_t& length_buffer ) {
    Pkt ack_pkt( 20 );
    ack_pkt.build( Router::addr_localhost, Router::addr_localhost, 1, 0, NULL, 0 );

    for ( uint_fast8_t i = 0; i < ack_pkt.get_size(); i++ ) {
        buffer[i] = ack_pkt.bytes()[i];
    }
    length_buffer = ack_pkt.get_size();
}