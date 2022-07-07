#include "lorawan_factory_setup.h"

lorawan_factory_setup::lorawan_factory_setup( lora_controller& lora_controller, uC_SerialController& serialController, baud_rate_t lora_baud_rate, uint32_t unique_id ) :
    lora_controller_( lora_controller ),
    debugPrinter_( serialController ),
    lora_baud_rate_( lora_baud_rate ),
    unique_id_( unique_id ) {}

lorawan_factory_setup::~lorawan_factory_setup() {}

bool lorawan_factory_setup::setup( void ) {
    lora_controller_.on();
    if ( !change_baud_rate() ) {
        return false;
    }
    if ( !validate_version() ) {
        return false;
    }
    if ( !change_deveui() ) {
        return false;
    }
    return true;
}

bool lorawan_factory_setup::change_baud_rate( void ) {
    bool baud_rate_change_ok = false;

    debugPrinter_.serial_println( "\nCambiando baudrate LORA" );

    for ( uint8_t i = 0; !baud_rate_change_ok && i < baud_config_attemps; i++ ) {
        baud_rate_change_ok = lora_controller_.change_baud_rate( lora_baud_rate_ );
    }

    if ( baud_rate_change_ok ) {
        debugPrinter_.serial_println( "-- EXITO BAUDRATE --\n" );
    }
    else {
        debugPrinter_.serial_println( "-- ERROR BAUDRATE --\n" );
    }
    return baud_rate_change_ok;
}

bool lorawan_factory_setup::validate_version( void ) {
    bool version_ok = false;

    debugPrinter_.serial_println( "\nValidando Version LORA" );

    for ( uint8_t i = 0; !version_ok && i < version_check_attemps; i++ ) {
        lora_controller_.check_version( version_ok );
    }

    if ( version_ok ) {
        debugPrinter_.serial_println( "-- EXITO VERSION FW --\n" );
    }
    else {
        debugPrinter_.serial_println( "-- ERROR VERSION FW --\n" );
    }

    return version_ok;
}

bool lorawan_factory_setup::change_deveui( void ) {
    bool deveui_changed_ok = false;

    debugPrinter_.serial_println( "\nCambiando DevEui LORA" );

    for ( uint8_t i = 0; !deveui_changed_ok && i < deveui_set_attemps; i++ ) {
        deveui_changed_ok = ( lora_controller_.set_deveui( unique_id_ ) == wtc_success );
    }

    if ( deveui_changed_ok ) {
        debugPrinter_.serial_println( "-- EXITO SET DEVEUI --\n" );
    }
    else {
        debugPrinter_.serial_println( "-- ERROR SET DEVEUI --\n" );
    }

    return deveui_changed_ok;
}
