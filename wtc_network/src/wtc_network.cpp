#include "wtc_network.h"
#include "router.h"
#include "elapsed_time.h"
#include "time_util.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "commands_ids.h"
#include "config_mgr.h"
#include "lossy.h"

Network::Network( uC_SerialController& debugPrinter_0, uC_DelayGenerator& uC_delay_0, const uint16_t in_size_0, const uint8_t times_nok_limit_0 ) :
    Port( in_size_0, times_nok_limit_0 ),
    serialController( debugPrinter_0 ),
    uC_delay( uC_delay_0 ) {
    init_cmd_array();
}

Network::~Network() {}

void Network::init_cmd_array( void ) {
    cmd_array[network_params].cmd_to_process = cmd_network_params_conf;
    cmd_array[network_params].function_ptr   = &Network::set_network_params;
    cmd_array[lora_params].cmd_to_process    = cmd_lora_params_conf;
    cmd_array[lora_params].function_ptr      = &Network::set_lora_params;
}

void Network::add( Network_controller_interface* p_controller_arr_0 ) {
    if ( num_controllers < controllers_max ) {
        p_controller_arr[num_controllers] = p_controller_arr_0;

        if ( p_controller_arr[num_controllers]->get_id_controller() == id_controller_murata93 || p_controller_arr[num_controllers]->get_id_controller() == id_controller_imxx ||
             p_controller_arr[num_controllers]->get_id_controller() == id_controller_rhf76 ) {
            send_offline = false;
        }
        num_controllers++;
    }
    p_current_controller = p_controller_arr[0];
}

void Network::setup( const char ip[], const uint16_t port, uint8_t eui_0[], uint8_t key_0[] ) {
    strncpy( config_network_params.config_address, ip, strlen( ip ) );
    config_network_params.config_address[strlen( ip )] = '\0';
    config_network_params.config_port                  = port;
    memcpy( config_lora_params.config_app_eui, eui_0, config_lora_params.max_len_eui );
    memcpy( config_lora_params.config_app_key, key_0, config_lora_params.max_len_key );
}

void Network::step( void ) {
    switch ( state ) {
        case comm_state_idle: {
            state = comm_state_attaching;
            break;
        }
        case comm_state_attaching: {
            join();
            break;
        }
        case comm_state_attached: {
            check_attach();
            break;
        }
        case comm_state_connecting: {
            check_connect();
            break;
        }
        case comm_state_connected: {
            break;
        }
        default: {
            break;
        }
    }
}

void Network::on( void ) {
    for ( uint8_t i = 0; i < num_controllers; i++ ) {
        p_controller_arr[i]->on();
    }
}

void Network::off( void ) {
    for ( uint8_t i = 0; i < num_controllers; i++ ) {
        p_controller_arr[i]->off();
    }
}

void Network::run_task( void ) {
    p_current_controller->listen();
    p_current_controller->run_task();
}

Comm_state Network::get_state( void ) {
    return state;
}

void Network::reset_pkt_len( void ) {
    len_pkt_offline = 0;
}

void Network::setup_connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type ) {
    // Guarda la configuración para un posible re-connect
    server_ip_address = (char*)ip;
    server_port       = port;
    server_conn_type  = type;
}

Wtc_err_t Network::connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type ) {
    Wtc_err_t result = wtc_err_init;

    if ( server_ip_address == NULL || server_port != port || server_conn_type != type ) {
        setup_connect( ip, port, type );
    }

    return result = p_current_controller->connect( Conn_config::conn_id_default, server_ip_address, server_port, server_conn_type );
}

Wtc_err_t Network::disconnect( void ) {
    return p_current_controller->disconnect( Conn_config::conn_id_default );
}

Wtc_err_t Network::send( const char* buffer, const uint16_t len ) {
    return send( (uint8_t*)buffer, len );
}

Wtc_err_t Network::send( const uint8_t buffer[], const uint16_t len ) {
    Wtc_err_t result = wtc_err_init;
    check_connect();
    uint32_t send_timeout = uC_delay.get_time_since_init() + timeout_connect;
    while ( state != comm_state_connected && send_timeout > uC_delay.get_time_since_init() ) {
        step();
    }
    if ( state == comm_state_connected ) {
        result = p_current_controller->send( Conn_config::conn_id_default, buffer, len );
    }
    return result;
}

Wtc_err_t Network::receive( uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout ) {
    return p_current_controller->receive( Conn_config::conn_id_default, buffer, len, max_len, timeout );
}

Wtc_err_t Network::receive( void ) {
    uint16_t len      = 0;
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t buffer[Port::in_size];

    ret_val = p_current_controller->receive( Conn_config::conn_id_default, buffer, len, Port::in_size );
    if ( ret_val == wtc_success ) {
        for ( uint16_t i = 0; i < len; i++ ) {
            if ( Port::pkt.parse( buffer[i] ) ) {
                uint8_t cmd = Port::pkt.hdr->cmd;
                Router::get_instance()->route( Port::pkt );
                if ( cmd == cmd_ack ) {
                    ret_val = wtc_success;
                }
                else {
                    ret_val = wtc_err_comms;
                }
            }
            else {
                ret_val = wtc_err_comms;
            }
        }
    }
    return ret_val;
}

void Network::join( void ) {
    if ( p_current_controller->join() == wtc_success ) {
        state = comm_state_attached;
    }
    else {
        state = comm_state_idle;
    }
}

void Network::check_attach( void ) {
    if ( p_current_controller->check_attach() == wtc_success ) {
        p_current_controller->connect( Conn_config::conn_id_default, server_ip_address, server_port, server_conn_type );
        state = comm_state_connecting;
    }
    else {
        state = comm_state_idle;
    }
}

void Network::check_connect( void ) {
    uint8_t status = 0;
    p_current_controller->conn_status( Conn_config::conn_id_default, status );
    if ( status == Conn_status::cipstatus_code_connected ) {
        state = comm_state_connected;
    }
    else if ( status == Conn_status::cipstatus_code_connecting ) {
        state = comm_state_connecting;
    }
    else if ( status == Conn_status::cipstatus_code_closed ) {
        state = comm_state_attached;
    }
    else {
        state = comm_state_idle;
    }
}

Wtc_err_t Network::send( void ) {
    create_buffer_pkts_to_send();
    for ( int i = 0; i < num_controllers; i++ ) {
        set_controller( p_controller_arr[i] );
        if ( send( p_current_controller ) == wtc_success ) {
            len_buffer_pkts = max_len;
            return wtc_success;
        }
    }
    len_buffer_pkts = max_len;
    len_pkt_offline = 0;
    return wtc_err_comms;
}

void Network::create_buffer_pkts_to_send( void ) {
    save_pkt();
    Pkt pkt_route( Pkt::get_pkt_overhead() + sizeof( uint16_t ) );
    uint8_t payload[sizeof( uint16_t )];
    if ( Port::pkt.hdr->cmd == cmd_sensor_data ) {
        while ( send_offline ) {
            memcpy( payload, &len_buffer_pkts, sizeof( uint16_t ) );
            pkt_route.build( Router::addr_localhost, Router::addr_localhost, cmd_request_pkt, 0, payload, sizeof( uint16_t ) );
            Router::get_instance()->route( pkt_route );
            if ( Port::pkt.hdr->cmd == cmd_stop_request_pkt ) {
                break;
            }
        }
    }
}

Wtc_err_t Network::send( Network_controller_interface* p_controller_0 ) {
    Wtc_err_t send_pkt   = wtc_err_comms;
    p_current_controller = p_controller_0;
    p_current_controller->listen();
    check_connect();
    uint32_t send_timeout = uC_delay.get_time_since_init() + timeout_connect;
    while ( state != comm_state_connected && send_timeout > uC_delay.get_time_since_init() ) {
        step();
    }

    if ( ( state == comm_state_connected ) && ( p_current_controller->send( Conn_config::conn_id_default, pkts_offline, len_pkt_offline ) == wtc_success ) ) {
        Port::pkt.hdr->len = 0;
        send_pkt           = wtc_success;
        len_pkt_offline    = 0;
    }
    return send_pkt;
}

void Network::set_controller( Network_controller_interface* p_controller_0 ) {
    p_current_controller = p_controller_0;
}

void Network::set_network_params( const Pkt& pkt0 ) {
    Payload_mgr payload_mgr( pkt0.msg, pkt0.hdr->len );
    uint8_t id_model = payload_mgr.get_uint8();
    // Comprobamos si corresponde con el modelo de configuracion
    if ( id_model != model_id_config_network_params ) {
        return;
    }
    if ( ( pkt0.hdr->len == Config_network_params::get_size() ) && check_pkt( pkt0 ) ) {
        config_network_params.from_pkt_payload( &payload_mgr );
        server_port       = config_network_params.config_port;
        server_ip_address = (char*)config_network_params.config_address;
        serialController.serial_print( "Port: " );
        serialController.serial_println( config_network_params.config_port );
        serialController.serial_print( "Address: " );
        serialController.serial_println( config_network_params.config_address );
        // Se envia pkt para comprobar que ha llegado la configuracion
        generate_ack( pkt0 );
    }
}

void Network::set_lora_params( const Pkt& pkt0 ) {
    Payload_mgr payload_mgr( pkt0.msg, pkt0.hdr->len );
    uint8_t id_model = payload_mgr.get_uint8();
    // Comprobamos si corresponde con el modelo de configuracion
    if ( id_model != model_id_config_lora_params ) {
        return;
    }
    if ( ( pkt0.hdr->len == Config_lora_params::get_size() ) && check_pkt( pkt0 ) ) {
        for ( int i = 0; i < num_controllers; i++ ) {
            if ( p_controller_arr[i]->get_id_controller() == id_controller_murata93 || p_controller_arr[i]->get_id_controller() == id_controller_imxx ||
                 p_controller_arr[i]->get_id_controller() == id_controller_rhf76 ) {
                config_lora_params.from_pkt_payload( &payload_mgr );
                char aux_appeui[2 * config_lora_params.max_len_eui + 1];
                char aux_appkey[2 * config_lora_params.max_len_key + 1];
                char* p_eui_str = aux_appeui;
                char* p_key_str = aux_appkey;
                for ( uint8_t j = 0; j < config_lora_params.max_len_eui; j++ ) {
                    p_eui_str += sprintf( p_eui_str, "%02X", config_lora_params.config_app_eui[j] );
                }
                *p_eui_str = 0;
                for ( uint8_t j = 0; j < config_lora_params.max_len_key; j++ ) {
                    p_key_str += sprintf( p_key_str, "%02X", config_lora_params.config_app_key[j] );
                }
                *p_key_str = 0;
                serialController.serial_print( "App_eui: " );
                serialController.serial_println( aux_appeui );
                serialController.serial_print( "App_key: " );
                serialController.serial_println( aux_appkey );
                if ( p_controller_arr[i]->set_params( aux_appeui, aux_appkey ) != wtc_success ) {
                    serialController.serial_println( "Error cambio eui y key" );
                }
            }
        }
        // Se envia pkt para comprobar que ha llegado la configuracion
        generate_ack( pkt0 );
    }
}

void Network::parse( const Pkt& pkt0 ) {
    for ( uint8_t i = 0; i < ( sizeof( cmd_array ) / sizeof( network_command_received ) ); i++ ) {
        if ( cmd_array[i].cmd_to_process == pkt0.hdr->cmd ) {
            ( this->*cmd_array[i].function_ptr )( pkt0 );
            return;
        }
    }
}

void Network::save_config_network_params( void ) {
    Pkt pkt( config_network_params.get_size() + 1 + Pkt::get_pkt_overhead() );
    Payload_mgr payload_mgr( pkt.msg, pkt.get_msg_len() );
    payload_mgr.reset();
    config_network_params.to_pkt_payload( &payload_mgr );
    pkt.hdr->len = payload_mgr.get_used_size();
    pkt.rebuild( Router::addr_localhost, Router::addr_localhost, cmd_network_params_conf, 0 );
    Config_mgr::get_instance()->save_config( Router::get_instance(), pkt );
}

void Network::save_config_lora_params( void ) {
    Pkt pkt( config_lora_params.get_size() + 1 + Pkt::get_pkt_overhead() );
    Payload_mgr payload_mgr( pkt.msg, pkt.get_msg_len() );
    payload_mgr.reset();
    config_lora_params.to_pkt_payload( &payload_mgr );
    pkt.hdr->len = payload_mgr.get_used_size();
    pkt.rebuild( Router::addr_localhost, Router::addr_localhost, cmd_lora_params_conf, 0 );
    Config_mgr::get_instance()->save_config( Router::get_instance(), pkt );
}

void Network::save_pkt( void ) {
    memcpy( &pkts_offline[len_pkt_offline], Port::pkt.bytes(), Port::pkt.get_size() );
    len_pkt_offline += Port::pkt.get_size();
    if ( len_buffer_pkts >= Port::pkt.get_size() ) {
        len_buffer_pkts -= Port::pkt.get_size();
    }
}

void Network::generate_ack( const Pkt& pkt0 ) {
    if ( pkt0.hdr->src != Router::addr_localhost ) {
        Config_mgr::get_instance()->save_config( Router::get_instance(), pkt0 );
        Router::get_instance()->echo_ack( pkt0 );
    }
}