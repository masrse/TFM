#pragma once
#include <stdint.h>
#include "network_controller_interface.h"
#include "wtc_base.h"

class Wtc_network_interface {
  public:
    virtual void setup( const char ip[], const uint16_t port, uint8_t eui_0[], uint8_t key_0[] )                        = 0;
    virtual void step( void )                                                                                           = 0;
    virtual void join( void )                                                                                           = 0;
    virtual void check_attach( void )                                                                                   = 0;
    virtual void check_connect( void )                                                                                  = 0;
    virtual void setup_connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type )             = 0;
    virtual Wtc_err_t connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type )              = 0;
    virtual Wtc_err_t disconnect( void )                                                                                = 0;
    virtual Wtc_err_t send( const uint8_t buffer[], const uint16_t len )                                                = 0;
    virtual Wtc_err_t send( const char* buffer, const uint16_t len )                                                    = 0;
    virtual Wtc_err_t receive( uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout = 5000 ) = 0;
    virtual void run_task( void )                                                                                       = 0;
    virtual void on( void )                                                                                             = 0;
    virtual void off( void )                                                                                            = 0;
    virtual void set_controller( Network_controller_interface* p_controller_0 )                                         = 0;
    virtual void parse( const Pkt& pkt0 )                                                                               = 0;
    virtual void save_config_network_params( void )                                                                     = 0;
    virtual void save_config_lora_params( void )                                                                        = 0;
    virtual void save_pkt( void )                                                                                       = 0;
    virtual Comm_state get_state( void )                                                                                = 0;
    virtual Wtc_err_t receive( void )                                                                                   = 0;
    virtual Wtc_err_t send( void )                                                                                      = 0;
    virtual Wtc_err_t send( Network_controller_interface* p_controller_0 )                                              = 0;
    virtual void reset_pkt_len( void )                                                                                  = 0;
};

class PseudoWtc_network_interface: public Wtc_network_interface {
  protected:
    virtual void setup( const char ip[], const uint16_t port, uint8_t eui_0[], uint8_t key_0[] ) {}
    virtual void step( void ) {}
    virtual void join( void ) {}
    virtual void check_attach( void ) {}
    virtual void check_connect( void ) {}
    virtual void setup_connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type ) {}
    virtual Wtc_err_t connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type ) {
        return wtc_success;
    }
    virtual Wtc_err_t disconnect( void ) {
        return wtc_success;
    }
    virtual Wtc_err_t send( const uint8_t buffer[], const uint16_t len ) {
        return wtc_success;
    }
    virtual Wtc_err_t send( const char* buffer, const uint16_t len ) {
        return wtc_success;
    }
    virtual Wtc_err_t receive( uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout = 5000 ) {
        return wtc_success;
    }
    virtual void run_task( void ) {}
    virtual void on( void ) {}
    virtual void off( void ) {}
    virtual void set_controller( Network_controller_interface* p_controller_0 ) {}
    virtual void parse( const Pkt& pkt0 ) {}
    virtual void save_config_network_params( void ) {}
    virtual void save_config_lora_params( void ) {}
    virtual void save_pkt( void ) {}
    virtual Comm_state get_state( void ) {
        return comm_state_connected;
    }

    virtual Wtc_err_t receive( void ) {
        return wtc_success;
    }

    virtual Wtc_err_t send( void ) {
        return wtc_success;
    }

    virtual Wtc_err_t send( Network_controller_interface* p_controller_0 ) {
        return wtc_success;
    }

    virtual void reset_pkt_len( void ) {}
};
