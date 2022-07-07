#pragma once

#include "stdint.h"
#include "uC_interface.h"

/*
  \class Controlador del sensor mcp-4000
  \brief Utiliza protocolo hardware serial
  \author Antonio Garcia
*/
class Mp_4000_Controller {
  public:
    /**
      \brief Constructor de la clase
      \param baudrate_controller baudrate con el que comunicara con el controlador
      \param timeout_sensor_0 timeout para la respuesta
    */
    Mp_4000_Controller( uC_DelayGenerator& delayGenerator_0, uC_SerialController& debugPrinter_0, uC_wdt_interface& watchdog_0, uint16_t timeout_sensor_0 );

    /**
      \brief Destructor de la clase
    */
    ~Mp_4000_Controller( void );

    /**
      \see Sensor#init
      \param baudrate_controller baudrate del controlador
    */
    void init( uint16_t baudrate_controller );

    /**
      \brief Envia cmd sensor read out y espera respuesta
    */
    bool set_sensor_read_out( void );

    /**
      \brief Envia cmd control read out y espera respuesta
    */
    bool set_control_read_out( void );

    /**
      \brief Obtiene la variable deseada
      \param supply1_air_temperature copia de la variable
    */
    void get_supply1_air_temperature( int16_t& supply1_air_temperature ) {
      supply1_air_temperature = reply_params.reply_sensor.supply_air1_temperature;
    };

    /**
      \brief Obtiene la variable deseada
      \param return_air_temperature copia de la variable
    */
    void get_return_air_temperature( int16_t& return_air_temperature ) {
      return_air_temperature = reply_params.reply_sensor.return_air_temperature;
    };

    /**
      \brief Obtiene la variable deseada
      \param temperature_set_point copia de la variable
    */
    void get_temperature_set_point( int16_t& temperature_set_point ) {
      temperature_set_point = reply_params.reply_control.temperature_set_point;
    };

    /**
      \brief Obtiene la variable deseada
      \param power_state copia de la variable
    */
    void get_power_state( uint8_t& power_state ) {
      power_state = reply_params.reply_control.power_state;
    };

    typedef enum {
        idle,
        wait_start_reply,
        wait_reply_end
    } Parse_reply_state_t;

  protected:
    typedef enum {
      supply_air1_temperature_L = 0,
      supply_air1_temperature_H = 1,
      return_air_temperature_L = 4,
      return_air_temperature_H = 5
    } Num_byte_reply_sensor;

    typedef enum {
      temperature_set_point_L = 2,
      temperature_set_point_H = 3,
      power_state = 6
    } Num_byte_reply_control;

    typedef struct {
      int16_t supply_air1_temperature;
      int16_t return_air_temperature;
    } Reply_params_sensor;

    typedef struct {
      int16_t temperature_set_point;
      uint8_t power_state;
    } Reply_params_control;

    typedef struct {
        bool got_reply;
        Reply_params_sensor reply_sensor;
        Reply_params_control reply_control;
    } Reply_params;

    Reply_params reply_params;

    /**
      \brief Parsea cada uno de los bytes
      \param data Dato a ser parseado
    */
    void parse_reply_byte( uint8_t byte );

    Parse_reply_state_t state;

    static const uint8_t reply_vector_lenght = 100;
    uint8_t reply_vector[ reply_vector_lenght ];
    uint8_t reply_vector_i;

    const uint8_t sensor_read_out_len = 84;
    const uint8_t control_read_out_len = 36;

    uint16_t timeout_sensor;
    /**
      \brief Captura respuesta del controlador
      \param time_out tiempo de espera de respuesta
    */
    bool wait_reply_controller( uint16_t time_out );

  private:
    //////////////////////////////////////Frame cmd example//////////////////////////////////////
    // ------------------------------------------------------------------------------------------
    // |     START     |  TYPE |   ADDRESS     |   DATA            |   CRC CHECK   |   END       |
    // ------------------------------------------------------------------------------------------
    // | 1byte | 1byte | 1byte | 1byte | 1byte | 0 up to 245 bytes | 1byte | 1byte | 1byte 1byte |
    // ------------------------------------------------------------------------------------------
    // |  ESC  |  SOF  |  FT   |    TA |   FA  |    DATA (0..n)    | CRCHi | CRCLo | ESC | EOF   |
    // ------------------------------------------------------------------------------------------
    // |  1B   |  02   |  04   |    00 |   00  |      02 01        |   FF  |  45   |  1B |  04   |
    // ------------------------------------------------------------------------------------------

    // Cmd params
    static const uint8_t start_esc  = 0x1B;
    static const uint8_t start_sof  = 0x02;
    static const uint8_t type = 0x04;
    static const uint8_t address_l = 0x00;
    static const uint8_t address_h = 0x00;
    static const uint8_t end_esc = 0x1B;
    static const uint8_t end_eof = 0x04;
    // Cmd data params
    static const uint8_t format_data = 0x82;
    static const uint8_t cmd_data  = 0xA7;
    static const uint8_t sub_cmd_data = 0x10;
    static const uint8_t byte_stuffed = 0x1B;
    // Cmd_params_sensor
    static const uint8_t request_cmd_data_sensor = 0x00;
    // Cmd_params_control
    static const uint8_t request_cmd_data_control = 0x01;

    // First position data
    static const uint8_t first_num_byte_data_sensor = 8;
    static const uint8_t first_num_byte_data_control = 8;
    uC_DelayGenerator& delayGenerator;
    uC_SerialController& serialController;
    uC_wdt_interface& watchdog;
};