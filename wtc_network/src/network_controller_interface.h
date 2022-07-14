#pragma once

#include "conn_config.h"

class Network_controller_interface {
  public:
    /**
      \brief Enciende el modulo
    */
    virtual void on( void ) = 0;

    /**
      \brief Apaga el modulo
    */
    virtual void off( void ) = 0;

    /**
      \brief Tarea accessible para el scheduler
    */
    virtual void run_task( void ) = 0;

    /**
      \brief Método encargado de comprobar si exite conexión a la red
      \return wtc_success si la conexión está abierta
              wtc_err_comms en caso contrario
    */
    virtual Wtc_err_t check_attach( void ) = 0;

    /**
      \brief Inicializa la conexion
    */
    virtual Wtc_err_t join( void ) = 0;

    /**
      \brief Devuelve el estado de la conexión conn_id
      \param conn_id conexion de la que se quiere saber el estado
    */
    virtual Wtc_err_t conn_status( Conn_config::Conn_id_t conn_id, uint8_t& status ) = 0;

    /**
      \brief Envia trama al servidor remoto
      \param conn_id Identificador de la conexión
      \param buffer Array de datos a enviar
      \param len Longitud del buffer
    */
    virtual Wtc_err_t send( const Conn_config::Conn_id_t conn_id, const uint8_t buffer[], const uint16_t len ) = 0;

    /**
      \brief Conecta con servidor remoto
      \param conn_id Identificador de la conexión
      \param ip Dirección IP del servidor
      \param port Puerto del servidor
      \param type Tipo de conexión, TCP o UDP
    */
    virtual Wtc_err_t connect( const Conn_config::Conn_id_t conn_id, const char* ip, const uint16_t port, const Conn_config::Conn_type_t type ) = 0;

    /**
      \brief Cierra la conexión con servidor remoto
      \param conn_id Identificador de la conexión
    */
    virtual Wtc_err_t disconnect( const Conn_config::Conn_id_t conn_id ) = 0;

    /**
      \brief Recibe trama de servidor remoto
      \param conn_id Identificador de la conexión
      \param buffer Buffer dónde copiar el mensaje que se ha recibido
      \param len Longitud del mensaje que se ha recibido
      \param max_len Longitud total del buffer del usuario. Si el mensaje
            recibido es más largo, no se copiará.
      \param timeout Máximo tiempo de espera (ms)
    */
    virtual Wtc_err_t receive( const Conn_config::Conn_id_t conn_id, uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout = 200 ) = 0;

    virtual void listen( void ) = 0;

    virtual Id_controller_t get_id_controller( void ) = 0;

    /**
      \brief Funcion para modificar los parametros
      \param arg1 Primer parametro a cambiar
      \param arg2 Segundo parametro a cambiar
    */
    virtual Wtc_err_t set_params( char* arg1, char* arg2 ) = 0;

};

class PseudoNetwork_controller_interface : public Network_controller_interface {
    public:
        void on( void ) {}

        void off( void ) {}

        void listen( void ) {}

        Wtc_err_t join( void ) {
            return wtc_success;
        }

        void run_task( void ) {}

        Wtc_err_t check_attach( void ) {
             return wtc_success;
        }

        Wtc_err_t conn_status( Conn_config::Conn_id_t conn_id, uint8_t &status ) {
            return wtc_success;
        }

        Wtc_err_t send( const Conn_config::Conn_id_t conn_id, const uint8_t buffer[], const uint16_t len ) {
            return wtc_success;
        }

        Wtc_err_t connect( const Conn_config::Conn_id_t conn_id, const char* ip, const uint16_t port,
                        const Conn_config::Conn_type_t type ) {
            return wtc_success;
        }

        Wtc_err_t disconnect( const Conn_config::Conn_id_t conn_id ) {
            return wtc_success;
        }

        Wtc_err_t receive( const Conn_config::Conn_id_t conn_id, uint8_t buffer[], uint16_t& len, const uint16_t max_len,
                        const uint32_t timeout = 5000 ) {
            return wtc_success;
        }

        Id_controller_t get_id_controller( void ) {
            return id_controller_sim8xx;
        }

        Wtc_err_t set_params( char* arg1, char* arg2 ) {
            return wtc_success;
        }
};