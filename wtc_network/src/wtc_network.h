#pragma once

#include "wtc_base.h"
#include "port.h"
#include "conn_config.h"
#include "network_controller_interface.h"
#include "irx.h"
#include "model_config_network_params.h"
#include "model_config_lora_params.h"
#include "conn_status_code.h"
#include "wtc_network_interface.h"
#include "uC_interface.h"

/**
  \class Network
  \brief Módulo Cellular para la arquitectura WiTraC
*/

class Network: public Port, public Irx, public Wtc_network_interface {
  public:
    /**
      \brief Constructor. Inicializa parámetros
      \param in_size_0 tamaño del buffer de entrada
      \param times_nok_limit_0 Numero de comprobaciones de estado erroneas antes
      de resetear
    */
    Network( uC_SerialController& debugPrinter_0, uC_DelayGenerator& uC_delay_0, const uint16_t in_size_0, const uint8_t times_nok_limit_0 );

    /**
      \brief Destructor
    */
    ~Network();

    void add( Network_controller_interface* p_controller_arr_0 );

    /**
      \brief Inicializa las variables de ip y puerto del model_config_network_params
    */
    void setup( const char ip[], const uint16_t port, uint8_t eui_0[], uint8_t key_0[] );

    /**
      \brief Maquina de estados
    */
    void step( void );
    /**
      \brief Connectar a la red
    */
    void join( void );

    /**
      \brief Comprueba si existe conexion a la red
    */
    void check_attach( void );

    /**
      \brief Comprueba si estamos conectados
    */
    void check_connect( void );

    /**
      \brief Guarda las variables necesarias para conectar con servidor remoto
      \param ip Dirección IP del servidor
      \param port Puerto del servidor
      \param type Tipo de conexión, TCP o UDP
    */
    void setup_connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type );

    /**
      \brief Guarda las variables para conectar con servidor remoto y se conecta
      \param ip Dirección IP del servidor
      \param port Puerto del servidor
      \param type Tipo de conexión, TCP o UDP

      \note Este método asume el usuario utilizará el módulo en modo conexión
      única. Para utilizar más de una conexión se debe utilizar el otro connect
    */
    Wtc_err_t connect( const char ip[], const uint16_t port, const Conn_config::Conn_type_t type );

    /**
      \brief Cierra la conexión con servidor remoto
    */
    Wtc_err_t disconnect( void );

    /**
      \brief Envia trama al servidor remoto
      \param buffer Array de datos a enviar
      \param len Longitud del buffer
      \return Wtc_err_t Devuelve wtc_success si el envío ha sido correcto
    */
    Wtc_err_t send( const uint8_t buffer[], const uint16_t len );

    /**
      \brief Realiza un cast del buffer. Necesario para la compatibilidad con ATmega1284
      \param buffer Array de datos a enviar
      \param len Longitud del buffer
      \return Wtc_err_t Devuelve wtc_success si el envío ha sido correcto
    */
    Wtc_err_t send( const char* buffer, const uint16_t len );

    /**
      \brief Recibe trama de servidor remoto
      \param buffer Buffer dónde copiar el mensaje que se ha recibido
      \param p_len Longitud del mensaje que se ha recibido
      \param max_len Longitud total del buffer del usuario. Si el mensaje
      recibido es más largo, no se copiará.
      \param timeout Máximo tiempo de espera (ms)
    */
    Wtc_err_t receive( uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout = 5000 );

    /**
      \brief Tarea accessible para el scheduler
    */
    void run_task( void );

    /**
      \brief Enciende el modulo
    */
    void on( void );

    /**
      \brief Apaga el modulo
    */
    void off( void );

    /**
     * \brief Recepcion paquete
     */
    Wtc_err_t receive( void );

    /**
      \brief Ejecuta el envio de la cola
      \return Wtc_err_t Devuelve wtc_success si el envío ha sido correcto
    */
    Wtc_err_t send( void );

    /**
      \brief Ejecuta el envio de la cola
      \param p_controller_0 Punerto al controlador
      \return Wtc_err_t Devuelve wtc_success si el envío ha sido correcto
    */
    Wtc_err_t send( Network_controller_interface* p_controller_0 );

    /**
      \see Irx::parse
    */
    void parse( const Pkt& pkt0 );

    void set_network_params( const Pkt& pkt0 );

    void set_lora_params( const Pkt& pkt0 );

    /**
      \brief Guarda en eeprom address y port
    */
    void save_config_network_params( void );

    /**
      \brief Guarda en eeprom los parametros de conexion de lora
    */
    void save_config_lora_params( void );

    /**
      \brief Guarda en el buffer el pkt a enviar
    */
    void save_pkt( void );

    /**
      \brief Metodo para obtener el estado de la conexión
      \return el estado de la conexión
    */
    Comm_state get_state( void );

    void reset_pkt_len( void );

    uint16_t len_pkt_offline = 0;

  protected:
    virtual void create_buffer_pkts_to_send( void );

    static const uint16_t max_len = 1024;
    uint8_t pkts_offline[max_len];
    uint16_t len_buffer_pkts = max_len;

    bool send_offline = true;
    char* server_ip_address = NULL;
    uint16_t server_port    = 0;

  private:
    uC_SerialController& serialController;
    uC_DelayGenerator& uC_delay;

    void generate_ack( const Pkt& pkt0 );

    static const uint8_t controllers_max     = 3;
    Network_controller_interface* p_current_controller = nullptr;
    Network_controller_interface* p_controller_arr[controllers_max];

    uint8_t num_controllers = 0;

    static const int ip_len_max = 32;

    Conn_config::Conn_type_t server_conn_type;

    Comm_state state;
    const uint32_t timeout_connect = 15000;
    Config_network_params config_network_params;
    Config_lora_params config_lora_params;


    void set_controller( Network_controller_interface* p_controller_0 );

    void init_cmd_array( void );

    typedef enum
    {
        network_params,
        lora_params,
        num_network_commands
    } network_commands;

    struct network_command_received {
        uint16_t cmd_to_process;
        void ( Network::*function_ptr )( const Pkt& pkt0 );
    };

    network_command_received cmd_array[num_network_commands];
};
