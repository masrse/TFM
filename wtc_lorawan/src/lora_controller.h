#pragma once

#include <stdint.h>
#include "wtc_base.h"
#include "network_controller_interface.h"

class lora_controller: public Network_controller_interface {
  public:
    virtual ~lora_controller() {}
    virtual bool change_baud_rate( const baud_rate_t new_baud_rate ) = 0;
    virtual Wtc_err_t check_version( bool& version )                 = 0;
    virtual Wtc_err_t set_deveui( uint32_t deveui ) {
        return wtc_success;
    }
};

class PseudoLoraController: public lora_controller {
  public:
    virtual void on( void ) {}

    virtual void off( void ) {}

    virtual void listen( void ) {}

    virtual Wtc_err_t join( void ) {
        return wtc_success;
    }

    virtual void run_task( void ) {}

    virtual Wtc_err_t check_attach( void ) {
        return wtc_success;
    }

    virtual Wtc_err_t conn_status( Conn_config::Conn_id_t conn_id, uint8_t& status ) {
        return wtc_success;
    }

    virtual Wtc_err_t send( const Conn_config::Conn_id_t conn_id, const uint8_t buffer[], const uint16_t len ) {
        return wtc_success;
    }

    virtual Wtc_err_t connect( const Conn_config::Conn_id_t conn_id, const char* ip, const uint16_t port, const Conn_config::Conn_type_t type ) {
        return wtc_success;
    }

    virtual Wtc_err_t disconnect( const Conn_config::Conn_id_t conn_id ) {
        return wtc_success;
    }

    virtual Wtc_err_t receive( const Conn_config::Conn_id_t conn_id, uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout = 5000 ) {
        return wtc_success;
    }

    virtual Id_controller_t get_id_controller( void ) {
        return id_controller_sim8xx;
    }

    virtual Wtc_err_t set_params( char* arg1, char* arg2 ) {
        return wtc_success;
    }
    virtual bool change_baud_rate( const baud_rate_t new_baud_rate ) {
        return true;
    }
    virtual Wtc_err_t check_version( bool& version ) {
        return wtc_success;
    }
};
