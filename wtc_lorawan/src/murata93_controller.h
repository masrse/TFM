#pragma once

#include "wtc_base.h"
#include "wtc_serial_interface.h"
#include "murata93_serial_handler.h"
#include "lora_controller.h"
#include "uC_interface.h"

namespace Murata93_ATCmd_cmds {
    static const char at[] PROGMEM   = "AT";
    static const char endl[] PROGMEM = "\r";
};

class Murata93_base_controller: public lora_controller {
  protected:
    /**
      \brief Constructor. Inicializa parámetros
      \param lora_serial Puntero a clase Wtc_serial_arduino para comunicar con
                            el módulo LoRa
      \param fifo_rx_0     Referencia al bufer rx de la fifo
      \param fifo_tx_0     Referencia al bufer tx de la fifo
      \param reset_pin_0   Pin reset del murata93
      \param module_serial_baudrate_0 baud_rate del dispositivo
    */
    Murata93_base_controller( Wtc_serial_interface& lora_serial, Fifo<uint8_t>& fifo_rx_0, Fifo<uint8_t>& fifo_tx_0, const uint8_t reset_pin_0,
                              const baud_rate_t module_serial_baudrate_0, uC_SerialController& debugPrinter_0, uC_PinController& pinController_0, uC_DelayGenerator& uC_delay_0,
                              uC_wdt_interface& watchdog_0 );

    virtual void command_send( uint16_t size_pkt )                                                                          = 0;
    virtual Wtc_err_t wait_receive_command( Murata93_ATCmd_evts::ATCmd_resp_evt_t& event, const uint32_t timeout )          = 0;
    virtual void copy_receive_msg( Murata93_ATCmd_evts::ATCmd_resp_evt_t event, uint8_t buffer[], uint16_t& length_buffer ) = 0;

    Murata93_serial_handler* p_handler_murata93;

  public:
    /* Timeouts para respuestas a comandos AT del controlador */
    static const uint32_t atresp_timeout_default_ms  = 1000;  ///< Para la mayoría de comandos AT
    static const uint32_t atresp_timeout_net_oper_ms = 10000; ///< Network operations

    /**
      \brief Destructor
    */
    ~Murata93_base_controller( void );

    /**
      \brief Función que establece el formato del mensaje
      \param format 0: text
                    1: hex
      \return Wtc_err_t
    */
    Wtc_err_t set_data_format( const uint8_t format );

    /**
      \brief Función que habilita el duty cycle necesario para la banda 868MHz
      \param state 0: disable
                   1: enable
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_duty_cycle( const uint8_t state );

    /**
      \brief Función que configura la potencia de la RF del módulo
      \param index 0-15
      \param mode 0: RFO mode
                  1: PABOOST mode
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_rfpower( const uint8_t mode, const uint8_t index );

    /**
      \brief Función que configura el data_rate
      \param datarate 0-15 #Ver datasheet apartado 5.4.3
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_data_rate( const uint8_t datarate );

    /**
      \brief Función que configura el modo de activación del módulo
      \param activation_mode 0: ABP
                             1: OTAA
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_activation_mode( const uint8_t activation_mode );

    /**
      \brief Función que establece el identificador de la aplicación
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_appeui( void );

    /**
      \brief Función que devuelve el identificador de la aplicación(para test)
    */
    void get_appeui( char* buffer );

    /**
      \brief Función que establece la clave de la aplicación
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_appkey( void );

    /**
      \brief Función que devuelve la clave de la aplicación(para test)
    */
    void get_appkey( char* buffer );

    /**
      \brief Función para cambiar el baudrate del dispositivo
      \param baud_rate baud_rate al que se quiere poner el dispositivo, por defecto a 19200
    */
    Wtc_err_t set_baudrate( baud_rate_t baud_rate );

    /**
      \brief Función para obtener el baudrate del dispositivo
      \param baud_rate copia baudrate al que está configurado el dispositivo
    */
    Wtc_err_t get_baudrate( baud_rate_t& baud_rate );

    /**
      \brief Función para setear el baudrate deseado al dispositivo
      \param new_baud_rate baudrate al que debe de configurarse el dispositivo
    */
    bool change_baud_rate( const baud_rate_t new_baud_rate );

    /**
      \brief Reboot del módulo
      \return Wtc_err_t code_error
    */
    Wtc_err_t reboot( void );

    /**
      \brief Función que envía los datos al gateway
      \param conn_id en este caso no existe id de conexión
      \param buffer mensaje a enviar
      \param len tamaño del mensaje
      \return Wtc_err_t code_error
    */
    Wtc_err_t send( const Conn_config::Conn_id_t conn_id, const uint8_t buffer[], const uint16_t len );

    /**
      \brief Función que devuelve el estado de la conexión LoRaWAN
      \param status 0: The link between modem and gateway is lost.
                    1: The link between modem and gateway is connected.
                    2: The modem doesn’t receive ACK for confirmed uplink message and will retransmission.
      \return Wtc_err_t code_error
    */
    Wtc_err_t get_status_connection( uint8_t& status );

    /**
      \brief Conexión a la red en modo OTAA
      \return Wtc_err_t code_error
    */
    Wtc_err_t join( void );

    /**
      \brief Función que comprueba la versión del módulo
      \param version true si coincide la version con la esperada
      \return Wtc_err_t code_error
    */
    Wtc_err_t check_version( bool& version );

    /**
      \brief Función que devuelve el identificador del módulo
      \param deveui identificador del módulo
      \return Wtc_err_t code_error
    */
    Wtc_err_t get_deveui( uint8_t deveui[] );

    /**
      \brief Función que establece el identificador del módulo
      \param deveui identificador del módulo
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_deveui( uint32_t deveui );

    /**
      \brief Función que devuelve la frecuencia de comunicación
      \param band ver band_channel_t
            band_channel_AS923        = 0,
            band_channel_AU915        = 1,
            band_channel_RFU          = 2,3 y 4
            band_channel_EU868        = 5,
            band_channel_KR920        = 6,
            band_channel_IN865        = 7,
            band_channel_US915        = 8,
            band_channel_US915_HYBRID = 9,
      \return Wtc_err_t code_error
    */
    Wtc_err_t get_band( uint8_t& band );

    /**
      \brief Función que establece la frecuencia de comunicación
      \param band ver band_channel_t
            band_channel_AS923        = 0,
            band_channel_AU915        = 1,
            band_channel_RFU          = 2,3 y 4
            band_channel_EU868        = 5,
            band_channel_KR920        = 6,
            band_channel_IN865        = 7,
            band_channel_US915        = 8,
            band_channel_US915_HYBRID = 9,
      \return Wtc_err_t code_error
    */
    Wtc_err_t set_band( const uint8_t band );


    /**
      \brief Función que devuelve rssi y snr del último mensaje recibido
      \param rssi
      \param snr
      \return Wtc_err_t code_error
    */
    Wtc_err_t get_rssi( uint8_t& rssi, uint8_t& snr );

    /**
      \brief Método encargado de comprobar si exite conexión a la red
      \return wtc_success si la conexión está abierta
              wtc_err_comms en caso contrario
    */
    Wtc_err_t check_attach( void );

    /**
      \brief Devuelve el estado de la conexión
      \param conn_id
      \param status
      \return Wtc_err_t
    */
    Wtc_err_t conn_status( Conn_config::Conn_id_t conn_id, uint8_t& status );

    /**
      \brief Inicializa las claves de la aplicación
      \param appeui Nombre del APN
      \param appkey Identificador para el APN
      \return Wtc_err_t
    */
    template<typename T, typename... Ts> Wtc_err_t setup_network( T appeui_0, T appkey_0, Ts... args ) {
        Wtc_err_t result_appeui = wtc_err_comms;
        Wtc_err_t result_appkey = wtc_err_comms;

        memcpy( appeui, appeui_0, strlen( appeui_0 ) );
        memcpy( appkey, appkey_0, strlen( appkey_0 ) );

        result_appeui = set_appeui();
        result_appkey = set_appkey();

        if ( ( result_appeui == wtc_success ) && ( result_appkey == wtc_success ) ) {
            return wtc_success;
        }
        return wtc_err_comms;
    }

    /**
      \brief Conecta con servidor remoto
      \param conn_id Identificador de la conexión
      \param ip Dirección IP del servidor
      \param port Puerto del servidor
      \param type Tipo de conexión, TCP o UDP
      \return Wtc_err_t code_error
    */
    Wtc_err_t connect( const Conn_config::Conn_id_t conn_id, const char* ip, const uint16_t port, const Conn_config::Conn_type_t type );

    /**
      \brief Funcion para modificar los parametros
      \param arg1 Primer parametro a cambiar
      \param arg2 Segundo parametro a cambiar
    */
    Wtc_err_t set_params( char* arg1, char* arg2 );

    /**
      \brief Cierra la conexión con servidor remoto
      \param conn_id Identificador de la conexión
      \return Wtc_err_t code_error
    */
    Wtc_err_t disconnect( const Conn_config::Conn_id_t conn_id );

    /**
      \brief Recibe trama de servidor remoto
      \param conn_id Identificador de la conexión
      \param buffer Buffer dónde copiar el mensaje que se ha recibido
      \param len Longitud del mensaje que se ha recibido
      \param max_len Longitud total del buffer del usuario. Si el mensaje
            recibido es más largo, no se copiará.
      \param timeout Máximo tiempo de espera (ms)
      \return Wtc_err_t code_error
    */
    Wtc_err_t receive( const Conn_config::Conn_id_t conn_id, uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout = 5000 );

    /**
      \brief Setup serial
    */
    void on( void );

    /**
      \brief Se crea por compatibilidad con wtc_network
    */
    void off( void );

    /**
      \brief Tarea accessible para el scheduler
    */
    void run_task( void );

    /**
      \brief Espera hasta timeout_ms milisegundos a que llegue
            cualquier evento en el serial_handler
      \param resp_evt Estructura donde guardar el evento
      \param timeout_ms Tiempo maximo de espera
      \return wtc_success si se recibe cualquier evento en el tiempo
              acordado, wtc_err_timeout si se sobrepasa este tiempo
    */
    Wtc_err_t resp_evt_wait_next( Murata93_ATCmd_evts::ATCmd_resp_evt_t& resp_evt, const uint32_t timeout_ms );

    /**
      \brief Espera hasta timeout_ms milisegundos a que llegue
            un evento especifico en el Murata93_serial_handler
      \param resp_evt Estructura donde guardar el evento
      \param timeout_ms Tiempo maximo de espera
      \param expected_code Tipo de evento esperado
      \param ignore_others Flag para ignorar eventos inesperados
      \return - Con ignore_others: wtc_success si se recibe el evento indicado
              en el tiempo acordado, wtc_err_timeout si se sobrepasa este tiempo,
              otros eventos inesperados se procesan con handle_unexpected_event
              - Sin ignore_others wtc_success si se recibe el evento indicado
              en el tiempo acordado, wtc_err_timeout si se recibe cualquier otro
              evento en este tiempo o si se sobrepasa este tiempo
    */
    Wtc_err_t resp_evt_wait_specific( Murata93_ATCmd_evts::ATCmd_resp_evt_t& resp_evt, const uint32_t timeout_ms, const Murata93_ATCmd_evts::ATCmd_resp_evt_code_t expected_code,
                                      const bool ignore_others = false );

    /**
      \brief Vacía la cola de enventos del serial_handler
    */
    void handle_all_queued_events( void );

    /**
      \brief Trata el evento no esperado
      \param event Evento no contemplado
    */
    void handle_unexpected_event( const Murata93_ATCmd_evts::ATCmd_resp_evt_t& event );

    /**
      \brief Devuelve si hay datos en la cola para la conexión
      \param conn_id Identificador de la conexión
      \return bool false si no hay nada pendiente
    */
    bool is_data_pending( const Conn_config::Conn_id_t conn_id );

    /**
      \brief Método para hacer un listen del Wtc_serial_arduino cuando
      se trabaja con dos Wtc_serial_arduino distintos
    */
    void listen( void );

    /**
      \brief Método que devuelve el id asociado al controlador
    */
    Id_controller_t get_id_controller( void );

    Wtc_err_t init( void );

  private:
    Wtc_serial_interface& serial_murata93;
    Fifo<uint8_t>& fifo_rx;
    Fifo<uint8_t>& fifo_tx;

    uint8_t reset_pin;
    baud_rate_t module_serial_baudrate; // baudrate de comunicación con el módulo

    static const uint8_t lenght_appeui = 17; // lenght = 16 bytes + /0
    static const uint8_t lenght_appkey = 33; // lenght = 32 bytes + /0
    char appeui[lenght_appeui];
    char appkey[lenght_appkey];

    uC_SerialController& serialController;
    uC_PinController& pinController;
    uC_DelayGenerator& uC_delay;
    uC_wdt_interface& watchdog;

    /**
      \brief Función para validar si el baudrate introducido como parametro es el mismo que el baudrate configurado en el dispositivo
      \param baud_rate baudrate a comprobar
    */
    bool check_baud_rate( baud_rate_t baud_rate );

    /**
      \brief Función que almacena el baudrate al que esta configurado el dispositivo
      \param new_baud_rate almacena el baudrate al que esta configurado el dispositivo
    */
    bool detect_baud_rate( baud_rate_t& detected_baud_rate );
};

class Murata93_controller: public Murata93_base_controller {
  public:
    Murata93_controller( Wtc_serial_interface& lora_serial, Fifo<uint8_t>& fifo_rx_0, Fifo<uint8_t>& fifo_tx_0, const uint8_t reset_pin_0,
                         const baud_rate_t module_serial_baudrate_0, uC_SerialController& debugPrinter_0, uC_PinController& pinController_0, uC_DelayGenerator& uC_delay_0,
                         uC_wdt_interface& watchdog_0 );

    ~Murata93_controller();

    void command_send( uint16_t size_pkt );
    Wtc_err_t wait_receive_command( Murata93_ATCmd_evts::ATCmd_resp_evt_t& event, const uint32_t timeout );
    void copy_receive_msg( Murata93_ATCmd_evts::ATCmd_resp_evt_t event, uint8_t buffer[], uint16_t& length_buffer );
};

class Murata93_controller_autoack: public Murata93_base_controller {
  public:
    Murata93_controller_autoack( Wtc_serial_interface& lora_serial, Fifo<uint8_t>& fifo_rx_0, Fifo<uint8_t>& fifo_tx_0, const uint8_t reset_pin_0,
                                 const baud_rate_t module_serial_baudrate_0, uC_SerialController& debugPrinter_0, uC_PinController& pinController_0, uC_DelayGenerator& uC_delay_0,
                                 uC_wdt_interface& watchdog_0 );

    ~Murata93_controller_autoack();

    void command_send( uint16_t size_pkt );
    Wtc_err_t wait_receive_command( Murata93_ATCmd_evts::ATCmd_resp_evt_t& event, const uint32_t timeout );
    void copy_receive_msg( Murata93_ATCmd_evts::ATCmd_resp_evt_t event, uint8_t buffer[], uint16_t& length_buffer );
};