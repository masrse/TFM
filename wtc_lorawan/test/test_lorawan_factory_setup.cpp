#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "lorawan_factory_setup.h"
#include "uC_interface.h"

#define ATTEMPTS_FAIL          100
#define BAUD_RATE_ATTEMPTS     5
#define VERSION_CHECK_ATTEMPTS 3
#define SET_EUI_ATTEMPTS       3
#define ID_TEST                12345U

class lora_controller_for_test: public PseudoLoraController {
  public:
    lora_controller_for_test( uint8_t baud_rate_attempts, uint8_t version_attempts, uint8_t eui_attempts ) :
        baud_rate_correct_attempt( baud_rate_attempts ),
        baud_rate_config_attempts( 0 ),
        version_correct_attempt( version_attempts ),
        version_check_attempts( 0 ),
        set_eui_correct_attempt( eui_attempts ),
        set_eui_attempts( 0 ),
        _swicthed_on( false ),
        baud_rate_configured( baud_rate_0 ),
        version_checked( false ) {}

    void on( void ) {
        _swicthed_on = true;
    }

    bool change_baud_rate( const baud_rate_t new_baud_rate ) {
        baud_rate_configured = new_baud_rate;
        return baud_rate_config_attempts++ == baud_rate_correct_attempt - 1;
    }

    Wtc_err_t check_version( bool& version ) {
        version_checked = true;
        version         = version_check_attempts++ == version_correct_attempt - 1;
        return wtc_success;
    }

    Wtc_err_t set_deveui( uint32_t deveui ) {
        id_set = deveui;
        if ( set_eui_attempts++ == set_eui_correct_attempt - 1 ) {
            return wtc_success;
        }

        return wtc_err_comms;
    }
    uint8_t baud_rate_correct_attempt;
    uint8_t baud_rate_config_attempts;

    uint8_t version_correct_attempt;
    uint8_t version_check_attempts;

    uint8_t set_eui_correct_attempt;
    uint8_t set_eui_attempts;

    bool _swicthed_on;
    baud_rate_t baud_rate_configured;
    bool version_checked;
    uint32_t id_set;
};

class GivenALorawanFactorySetup: public testing::Test {
  public:
    void SetUp( void ) override {
        // ARRANGE
        controller_test = new lora_controller_for_test( BAUD_RATE_ATTEMPTS, VERSION_CHECK_ATTEMPTS, SET_EUI_ATTEMPTS );
        serial_test     = new uC_PseudoSerialController();
        lora_config     = new lorawan_factory_setup( *controller_test, *serial_test, baud_rate_19200, ID_TEST );
    }

    void TearDown( void ) {
        delete ( controller_test );
        delete ( serial_test );
        delete ( lora_config );
    }

    lora_controller_for_test* controller_test;
    uC_PseudoSerialController* serial_test;
    lorawan_factory_setup* lora_config;
};


TEST_F( GivenALorawanFactorySetup, WhenLoraIsSetup_ThenTheLoraControllerIsSwitchedOn ) {
    // ACT
    lora_config->setup();

    // ASSERT
    ASSERT_TRUE( controller_test->_swicthed_on );
}

TEST_F( GivenALorawanFactorySetup, WhenLoraIsSetup_ThenTheLoraControllerBaudRateIsConfigured ) {
    // ACT
    lora_config->setup();

    // ASSERT
    ASSERT_EQ( baud_rate_19200, controller_test->baud_rate_configured );
}

TEST_F( GivenALorawanFactorySetup, WhenLoraIsSetupAndEveryConfigurationWorks_ThenNoErrorIsObtained ) {
    // ACT, ASSERT
    ASSERT_TRUE( lora_config->setup() );
}

TEST_F( GivenALorawanFactorySetup, WhenTheBaudRateIsCorrectlyConfigured_ThenTheVersionIsCheckedInTheController ) {
    // ACT
    lora_config->setup();

    // ASSERT
    ASSERT_TRUE( controller_test->version_checked );
}


TEST_F( GivenALorawanFactorySetup, WhenTheBaudRateIsCorrectlyConfiguredAndtheVersionIsChecked_ThenTheIdIsSet ) {
    // ACT
    lora_config->setup();

    // ASSERT
    ASSERT_EQ( ID_TEST, controller_test->id_set );
}


TEST( GivenALorawanFactorySetupThatFailsInTheBaudRateConfiguration, WhenLoraIsSetup_ThenAnErrorIsObtained ) {
    // ARRANGE
    lora_controller_for_test controller_test( ATTEMPTS_FAIL, VERSION_CHECK_ATTEMPTS, SET_EUI_ATTEMPTS );
    uC_PseudoSerialController serial_test;

    lorawan_factory_setup lora_config( controller_test, serial_test, baud_rate_19200 );

    // ACT, ASSERT
    ASSERT_FALSE( lora_config.setup() );
}


TEST( GivenALorawanFactorySetupThatFailsInTheVersionChecking, WhenLoraIsSetup_ThenNoErrorIsObtained ) {
    // ARRANGE
    lora_controller_for_test controller_test( BAUD_RATE_ATTEMPTS, ATTEMPTS_FAIL, SET_EUI_ATTEMPTS );
    uC_PseudoSerialController serial_test;

    lorawan_factory_setup lora_config( controller_test, serial_test, baud_rate_19200 );

    // ACT, ASSERT
    ASSERT_FALSE( lora_config.setup() );
}

TEST( GivenALorawanFactorySetupThatFailsInTheUniqueIdSet, WhenLoraIsSetup_ThenNoErrorIsObtained ) {
    // ARRANGE
    lora_controller_for_test controller_test( BAUD_RATE_ATTEMPTS, VERSION_CHECK_ATTEMPTS, ATTEMPTS_FAIL );
    uC_PseudoSerialController serial_test;

    lorawan_factory_setup lora_config( controller_test, serial_test, baud_rate_19200 );

    // ACT, ASSERT
    ASSERT_FALSE( lora_config.setup() );
}
