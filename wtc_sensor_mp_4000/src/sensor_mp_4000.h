#pragma once

#include "sensor.h"
#include "model_mp_4000.h"
#include "mp_4000_controller.h"
#include "low_power_trigger.h"
#include "wtc_serial_interface.h"
#include "fifo.h"
#include "uC_interface.h"

class Sensor_mp_4000: public Sensor, public Low_power_trigger {

  typedef enum {
    idle,
    go_to_sleep,
    waiting_to_get_out_off_sleep
  } Trigger_state;

  public:
    /**
      \brief Constructor de la clase
    */
    Sensor_mp_4000( Mp_4000_Controller* mp_4000_0, uint16_t baudrate_mp_4000_0, Wtc_serial_interface* wtc_serial_0, Fifo<uint8_t>& fifo_rx_0, uC_SerialController& debguPrinter_0 );

    /**
      \brief Destructor de la clase
    */
    ~Sensor_mp_4000( void );

    /**
      \see Sensor#init
    */
    void init( void );

    /**
      \see Sensor#measure
    */
    void measure( void );

    /**
      \see Sensor#append
    */
    void append( Payload_mgr* p_payload_mgr, const uint32_t timestamp );

    /**
      \see Sensor#on_ack
    */
    void on_ack( void );

    /**
      \brief Obtiene supply1 air temperature del sensor
      \return Devuelve supply1 air temperature del sensor en int
    */
    int16_t get_supply1_air_temperature( void ) {
      return supply1_air_temperature;
    }
    /**
      \brief Obtiene air temperature del sensor
      \return Devuelve air temperature del sensor en int
    */
    int16_t get_return_air_temperature( void ) {
      return return_air_temperature;
    }
    /**
      \brief Obtiene temperature set point del control
      \return Devuelve temperature set point del control en int
    */
    int16_t get_temperature_set_point( void ) {
      return temperature_set_point;
    }
    /**
      \brief Obtiene power state del control
      \return Devuelve power state del control
    */
    uint8_t get_power_state( void ) {
      return power_state;
    }

    /**
      \see Low_power_trigger#run_trigger
    */
    uint8_t run_trigger( void );

  protected:
    static const uint16_t measure_time = 1;
    Mp_4000 model_mp_4000;
    Mp_4000_Controller* mp_4000;

  private:
    // Sensor readout
    int16_t supply1_air_temperature;
    int16_t return_air_temperature;
    // Control readout
    int16_t temperature_set_point;
    uint8_t power_state;
    // Variable de estado para run_trigger
    Trigger_state trigger_state;
    // Baudrate sensor
    uint16_t baudrate_mp_4000;

    // Software serial para habilitación de PCINT
    Fifo<uint8_t>& fifo_rx;
    Wtc_serial_interface* w_serial;

    // Resultado de respuesta a comando
    bool reply_sensor_read_out = false;
    bool reply_control_read_out = false;
    uC_SerialController& serialController;
};