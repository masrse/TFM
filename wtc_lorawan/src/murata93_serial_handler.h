#pragma once

#include "wtc_serial_interface.h"
#include "fifo.h"
#include "uC_interface.h"
#include "wtc_base.h"
#include "conn_config.h"
#include <stdio.h>
#include <avr/pgmspace.h>

namespace Murata93_ATCmd_evts {

    static const uint8_t resp_msg_len_max               = 110;
    static const uint8_t resp_msg_unknown_num           = 3;
    static const char resp_msg_ok[] PROGMEM             = "+OK";
    static const char resp_msg_ok_with_params[] PROGMEM = "+OK=";
    static const char resp_msg_error[] PROGMEM          = "+ERR";
    static const char resp_msg_ack[] PROGMEM            = "+ACK";
    static const char resp_msg_noack[] PROGMEM          = "+NOACK";
    static const char resp_msg_conection[] PROGMEM      = "+EVENT";
    static const char resp_msg_received[] PROGMEM       = "+RECV";
    static const char resp_fw_version[] PROGMEM         = "+OK=1.1.03";

    typedef enum
    {
        atcmd_resp_evt_code_ok = 0,
        atcmd_resp_evt_code_ok_with_params,
        atcmd_resp_evt_code_error,
        atcmd_resp_evt_code_ack,
        atcmd_resp_evt_code_noack,
        atcmd_resp_evt_code_fw_version,
        atcmd_resp_evt_code_event,
        atcmd_resp_evt_code_received_data,
        atcmd_resp_evt_code_unknown_resp
    } ATCmd_resp_evt_code_t;

    typedef enum
    {
        atcmd_resp_error_code_cmd_unknown = 1,
        atcmd_resp_error_code_num_parameter_invalid,
        atcmd_resp_error_code_content_parameter_invalid,
        atcmd_resp_error_code_restore_to_factory,
        atcmd_resp_error_code_device_not_in_network,
        atcmd_resp_error_code_device_is_in_network,
        atcmd_resp_error_code_mac_busy,
        atcmd_resp_error_code_same_firmware_version,
        atcmd_resp_error_code_information_not_set,
        atcmd_resp_error_code_memory,
        atcmd_resp_error_code_update_firmware,
        atcmd_resp_error_code_payload_size,
        atcmd_resp_error_code_only_abp,
        atcmd_resp_error_code_only_ota,
        atcmd_resp_error_code_band,
        atcmd_resp_error_code_power_value,
        atcmd_resp_error_code_cmd_unusable_under_band,
        atcmd_resp_error_code_duty_cycle_limits,
        atcmd_resp_error_code_no_channel_available
    } ATCmd_resp_error_code_t;

    typedef struct {
        bool version_ok;
    } ATCmd_resp_evt_version_t;

    typedef struct {
        uint8_t* eui;
    } ATCmd_resp_evt_deveui_t;

    typedef struct {
        uint8_t band;
    } ATCmd_resp_evt_band_t;

    typedef struct {
        uint16_t baudrate;
    } ATCmd_resp_evt_baudrate_t;

    typedef struct {
        uint8_t rssi;
        uint8_t snr;
    } ATCmd_resp_evt_rf_parameters_t;

    typedef struct {
        bool ack;
    } ATCmd_resp_evt_ack_response_t;

    typedef struct {
        uint8_t type;
        uint8_t event_number;
        uint8_t status;
    } ATCmd_resp_evt_params_device_status_t;

    typedef struct {
        uint8_t port;
        uint8_t length;
        uint8_t* response;
    } ATCmd_resp_evt_params_received_data_t;

    typedef struct {
        uint8_t error;
    } ATCmd_resp_evt_params_error_type_t;

    typedef struct {
        uint8_t* p_resp;
        uint16_t resp_len;
    } ATCmd_resp_evt_params_unknown_response_t;

    typedef struct {
        ATCmd_resp_evt_code_t code;
        union {
            ATCmd_resp_evt_version_t version;
            ATCmd_resp_evt_deveui_t deveui;
            ATCmd_resp_evt_band_t type_band;
            ATCmd_resp_evt_baudrate_t baud_rate;
            ATCmd_resp_evt_rf_parameters_t rf_parameters;
            ATCmd_resp_evt_ack_response_t ack_response;
            ATCmd_resp_evt_params_device_status_t device_status;
            ATCmd_resp_evt_params_received_data_t received_data;
            ATCmd_resp_evt_params_error_type_t error_type;
            ATCmd_resp_evt_params_unknown_response_t unknown_response;
        } params;
    } ATCmd_resp_evt_t;

};

class Murata93_serial_handler {
  public:
    Murata93_serial_handler( Wtc_serial_interface& wtc_serial_murata93, Fifo<uint8_t>& fifo_rx, Fifo<uint8_t>& fifo_tx, uC_DelayGenerator& delayGenerator_0,
                          uC_SerialController& debugPrinter_0 );
    ~Murata93_serial_handler();

    /**
      \brief Metodos sobrecargados para la escritura de bytes por puerto serie software
    */
    Murata93_serial_handler& operator<<( uint8_t data ) {
        char cast_data_vector[4];
        uint8_t num_char;

        num_char = snprintf( cast_data_vector, sizeof( cast_data_vector ), "%d", data );

        for ( uint8_t i = 0; i < num_char; i++ ) {
            fifo_tx.put( cast_data_vector[i] );
            serialController.serial_write( cast_data_vector[i] );
        }

        write_to_serial_port();

        return *this;
    }
    Murata93_serial_handler& operator<<( uint16_t data ) {
        char cast_data_vector[6];
        uint8_t num_char;

        num_char = snprintf( cast_data_vector, sizeof( cast_data_vector ), "%d", data );

        for ( uint8_t i = 0; i < num_char; i++ ) {
            fifo_tx.put( cast_data_vector[i] );
            serialController.serial_write( cast_data_vector[i] );
        }

        write_to_serial_port();

        return *this;
    }
    Murata93_serial_handler& operator<<( uint32_t data ) {
        char cast_data_vector[11];
        uint8_t num_char;

        num_char = snprintf( cast_data_vector, sizeof( cast_data_vector ), "%u", data );

        for ( uint8_t i = 0; i < num_char; i++ ) {
            fifo_tx.put( cast_data_vector[i] );
            serialController.serial_write( cast_data_vector[i] );
        }

        write_to_serial_port();

        return *this;
    }
    Murata93_serial_handler& operator<<( const char* data ) {
        uint8_t c;
        while ( ( c = pgm_read_byte_near( data++ ) ) != 0 ) {
            fifo_tx.put( c );
            serialController.serial_write( c );
        }
        write_to_serial_port();
        return *this;
    }
    Murata93_serial_handler& operator<<( char* data ) {
        uint8_t i = 0;
        while ( ( (uint8_t)data[i] ) != 0 ) {
            fifo_tx.put( (uint8_t)data[i] );
            serialController.serial_write( (uint8_t)data[i]);
            i++;
        }
        write_to_serial_port();
        return *this;
    }

    /**
      \brief Escribe un byte en formato hexadecimal
      \param data byte a escribir
    */
    void write_byte_to_serial_in_hex( uint8_t data ) {
        char cast_data_vector[4];
        uint8_t num_char;

        num_char = snprintf( cast_data_vector, sizeof( cast_data_vector ), "%0X", data );

        for ( uint8_t i = 0; i < num_char; i++ ) {
            fifo_tx.put( cast_data_vector[i] );
            serialController.serial_write( cast_data_vector[i] );
        }

        write_to_serial_port();
    }

    typedef enum
    {
        idle,
        wait_recv,
        wait_resp_msg,
        wait_resp_end,
        save_data
    } Parse_resp_state_t;

    /**
      \brief Parsea cada uno de los bytes
      \param data Dato a ser parseado
    */
    void parse_rx_byte( uint8_t data );

    /**
      \brief Devuelve el numero de elementos disponibles
    */
    uint8_t resp_evt_available( void );

    /**
      \brief Obtiene el siguiente elemento de la fifo
    */
    bool resp_evt_get( Murata93_ATCmd_evts::ATCmd_resp_evt_t& resp_evt );

    /**
      \brief Obtiene un elemento concreto de la fifo
    */
    Murata93_ATCmd_evts::ATCmd_resp_evt_t resp_evt_peek( uint8_t idx );

    /**
      \brief Resetea la fifo y la maquina de estados
    */
    void reset( void );

    /**
      \brief LLama a parse_rx_byte siempre que tengamos datos en la fifo
    */
    void run_task( void );

    /**
      \brief Vacia el buffer del serial
    */
    void flush( void );

  protected:
    Parse_resp_state_t state;

  private:
    uint8_t resp_buffer[Murata93_ATCmd_evts::resp_msg_len_max];
    uint8_t aux_buffer[Murata93_ATCmd_evts::resp_msg_len_max];
    uint16_t resp_buffer_i;

    static const uint8_t char_idle_to_prepare_recv  = '+';
    static const uint8_t char_any_to_wait_resp_end  = '\r';
    static const uint8_t char_wait_resp_end_to_idle = '\n';
    static const uint8_t end_response_size          = 4;

    uC_DelayGenerator& delayGenerator;
    uC_SerialController& serialController;
    Wtc_serial_interface& wtc_serial_murata93;
    Fifo<uint8_t>& fifo_rx;
    Fifo<uint8_t>& fifo_tx;

    static const uint8_t resp_evt_queue_max = 10;
    Fifo<Murata93_ATCmd_evts::ATCmd_resp_evt_t> resp_evt_queue;

    /**
      \brief Escribe por el puerto serie software los bytes disponibles
    */
    void write_to_serial_port( void ) {
#ifndef TEST_FLAG
        while ( fifo_tx.available() ) {
            wtc_serial_murata93.run();
        }
#endif
    }
};
