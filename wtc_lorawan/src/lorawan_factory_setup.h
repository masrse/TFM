#pragma once

#include <stdint.h>
#include "lora_controller.h"

#include "uC_interface.h"

class lorawan_factory_setup {
  public:
    lorawan_factory_setup( lora_controller& lora_controller, uC_SerialController& serialController, baud_rate_t lora_baud_rate, uint32_t unique_id = 0 );
    ~lorawan_factory_setup();

    bool setup( void );

  private:
    bool change_baud_rate( void );
    bool validate_version( void );
    bool change_deveui( void );
    uint32_t get_id( void );
    lora_controller& lora_controller_;
    uC_SerialController& debugPrinter_;
    baud_rate_t lora_baud_rate_;
    uint32_t unique_id_;

    static const uint8_t baud_config_attemps   = 5;
    static const uint8_t version_check_attemps = 3;
    static const uint8_t deveui_set_attemps    = 3;
};