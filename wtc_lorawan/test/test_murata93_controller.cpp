/**
  \brief Test para la clase Murata93_controller
*/

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "wtc_base.h"
#include "murata93_controller.h"
#include "time_util.h"
#include "Mockfifo.h"
#include "conn_status_code.h"
#include "Mock_uC_interface.h"

// Selección del hardware a utilizar
// #define HW_RTLS_V4_0_X
#define HW_RTLS_V4_1_X
#include "boards.h"

#define lora_reset_dummy 5

using testing::An;
using testing::AtLeast;
using testing::ElementsAreArray;
using testing::Exactly;
using testing::InSequence;
using ::testing::Mock;
using testing::Return;


static const uint8_t len_fifo_test = 150;
Fifo<uint8_t> murata93_fifo_rx( len_fifo_test );
Fifo<uint8_t> murata93_fifo_tx( len_fifo_test );


uint8_t pkt_buffer_with_reefer_powered_batery_and_location[] = { 44,  232, 233, 101, 2,  123, 0,  0, 0, 2, 9,  80,  225, 19, 96, 21, 43,  0, 237, 0, 211, 11, 184, 1, 106, 132, 54,
                                                                 129, 56,  1,   96,  25, 177, 64, 0, 0, 0, 21, 117, 42,  1,  87, 82, 160, 0, 0,   0, 0,   0,  0,   0, 0,   251 };

class Given_a_murata93_controller: public ::testing::Test {
  protected:
    void SetUp() override {
        mock_uC_PinController_obj    = uC_PinController_Instance();
        mock_uC_DelayGenerator_obj   = uC_DelayGenerator_Instance();
        mock_uC_SerialController_obj = uC_SerialController_Instance();
        mock_uC_Watchdog_obj         = uC_Watchdog_Instance();

        murata93_controller = new Murata93_controller( wtc_serial_for_murata93_test,
                                                       murata93_fifo_rx,
                                                       murata93_fifo_tx,
                                                       lora_reset_dummy,
                                                       (baud_rate_t)murata93_baudrate,
                                                       *mock_uC_SerialController_obj,
                                                       *mock_uC_PinController_obj,
                                                       *mock_uC_DelayGenerator_obj,
                                                       *mock_uC_Watchdog_obj );
    }

    void TearDown() override {
        release_uC_PinController();
        release_uC_DelayGenerator();
        release_uC_SerialController();
        release_uC_Watchdog();
        murata93_fifo_rx.reset();
        murata93_fifo_tx.reset();
    }

    MockuC_DelayGenerator* mock_uC_DelayGenerator_obj;
    MockuC_Watchdog* mock_uC_Watchdog_obj;
    MockuC_PinController* mock_uC_PinController_obj;
    MockuC_SerialController* mock_uC_SerialController_obj;
    Murata93_controller* murata93_controller;
    uC_Pseudo_Wtc_serial_interface wtc_serial_for_murata93_test;
};

class Given_a_murata93_controller_autoack: public ::testing::Test {
  protected:
    void SetUp() override {
        mock_uC_PinController_obj    = uC_PinController_Instance();
        mock_uC_DelayGenerator_obj   = uC_DelayGenerator_Instance();
        mock_uC_SerialController_obj = uC_SerialController_Instance();
        mock_uC_Watchdog_obj         = uC_Watchdog_Instance();

        murata93_controller_autoack = new Murata93_controller_autoack( wtc_serial_for_murata93_test,
                                                                       murata93_fifo_rx,
                                                                       murata93_fifo_tx,
                                                                       lora_reset_dummy,
                                                                       (baud_rate_t)murata93_baudrate,
                                                                       *mock_uC_SerialController_obj,
                                                                       *mock_uC_PinController_obj,
                                                                       *mock_uC_DelayGenerator_obj,
                                                                       *mock_uC_Watchdog_obj );
    }

    void TearDown() override {
        release_uC_PinController();
        release_uC_DelayGenerator();
        release_uC_SerialController();
        release_uC_Watchdog();
        murata93_fifo_rx.reset();
        murata93_fifo_tx.reset();
    }

    MockuC_DelayGenerator* mock_uC_DelayGenerator_obj;
    MockuC_Watchdog* mock_uC_Watchdog_obj;
    MockuC_PinController* mock_uC_PinController_obj;
    MockuC_SerialController* mock_uC_SerialController_obj;
    Murata93_controller_autoack* murata93_controller_autoack;
    uC_Pseudo_Wtc_serial_interface wtc_serial_for_murata93_test;
};

static void put_fifo_rx_test_buffer( const uint8_t* data, uint8_t size_data ) {
    for ( uint8_t i = 0; i < size_data; i++ ) {
        murata93_fifo_rx.put( data[i] );
    }
}

TEST_F( Given_a_murata93_controller, When_the_data_format_command_is_sent_and_reponse_is_correct_Then_data_format_command_is_executed_correctly ) {
    // ARRANGE
    const uint8_t resp_command[]   = "+OK\r\n\r\n";
    const uint8_t len_resp_command = sizeof( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_format( 1 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_data_format_command_is_sent_and_reponse_is_incorrect_Then_data_format_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_format( 1 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_data_format_command_is_sent_and_format_is_incorrect_Then_data_format_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_format( 3 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_data_format_command_is_sent_and_reponse_is_unknown_Then_data_format_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_format( 1 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_duty_cycle_command_is_sent_and_reponse_is_correct_Then_duty_cycle_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_duty_cycle( 1 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_duty_cycle_command_is_sent_and_reponse_is_incorrect_Then_duty_cycle_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_duty_cycle( 1 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_duty_cycle_command_is_sent_and_configuration_is_incorrect_Then_duty_cycle_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_duty_cycle( 3 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_duty_cycle_command_is_sent_and_reponse_is_unknown_Then_duty_cycle_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_duty_cycle( 1 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_rf_power_command_is_sent_and_reponse_is_correct_Then_rf_power_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_rfpower( 1, 0 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_rf_power_command_is_sent_and_reponse_is_incorrect_Then_rf_power_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_rfpower( 1, 0 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_rf_power_command_is_sent_and_command_is_incorrect_Then_rf_power_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_rfpower( 3, 0 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_rf_power_command_is_sent_and_reponse_is_unknown_Then_rf_power_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_rfpower( 1, 0 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_data_rate_command_is_sent_and_reponse_is_correct_Then_data_rate_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_rate( 6 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_data_rate_command_is_sent_and_reponse_is_incorrect_Then_data_rate_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_rate( 5 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_data_rate_command_is_sent_and_command_is_incorrect_Then_data_rate_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_rate( 20 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_data_rate_command_is_sent_and_reponse_is_unknown_Then_data_rate_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_data_rate( 5 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_activation_mode_command_is_sent_and_reponse_is_correct_Then_activation_mode_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_activation_mode( 0 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_activation_mode_command_is_sent_and_reponse_is_incorrect_Then_activation_mode_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_activation_mode( 1 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_activation_mode_command_is_sent_and_command_is_incorrect_Then_activation_mode_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_activation_mode( 20 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_activation_mode_command_is_sent_and_reponse_is_unknown_Then_activation_mode_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_activation_mode( 5 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_set_appeui_command_is_sent_and_reponse_is_correct_Then_set_appeui_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_appeui();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_appeui_command_is_sent_and_reponse_is_incorrect_Then_appeui_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_appeui();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_appeui_command_is_sent_and_reponse_is_unknown_Then_appeui_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_appeui();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_appkey_command_is_sent_and_reponse_is_correct_Then_appkey_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_appkey();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_appkey_command_is_sent_and_reponse_is_incorrect_Then_appkey_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_appkey();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_appkey_command_is_sent_and_reponse_is_unknown_Then_appkey_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_appkey();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_set_baudrate_command_is_sent_and_reponse_is_correct_Then_set_baudrate_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_baudrate( (baud_rate_t)murata93_baudrate );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_set_baudrate_command_is_sent_and_reponse_is_incorrect_Then_set_baudrate_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_baudrate( (baud_rate_t)murata93_baudrate );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_set_baudrate_command_is_sent_and_reponse_is_unknown_Then_set_baudrate_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->set_baudrate( (baud_rate_t)murata93_baudrate );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_get_baudrate_command_is_sent_and_reponse_is_correct_Then_get_baudrate_command_is_executed_correctly ) {
    // ARRANGE
    baud_rate_t config_baudrate;
    char resp_command[]            = "+OK=19200,8,1,0,0\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->get_baudrate( config_baudrate );

    // ASSERT
    EXPECT_EQ( config_baudrate, 19200 );
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_get_baudrate_command_is_sent_and_reponse_is_incorrect_Then_get_baudrate_command_is_executed_incorrectly ) {
    // ARRANGE
    baud_rate_t config_baudrate;
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->get_baudrate( config_baudrate );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_get_baudrate_command_is_sent_and_reponse_is_unknown_Then_get_baudrate_command_is_not_executed ) {
    // ARRANGE
    baud_rate_t config_baudrate;
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->get_baudrate( config_baudrate );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_send_command_is_sent_and_reponse_is_correct_Then_send_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->send( 0, pkt_buffer_with_reefer_powered_batery_and_location, sizeof( pkt_buffer_with_reefer_powered_batery_and_location ) );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_send_command_is_sent_and_reponse_is_incorrect_Then_send_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->send( 0, pkt_buffer_with_reefer_powered_batery_and_location, sizeof( pkt_buffer_with_reefer_powered_batery_and_location ) );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_send_command_is_sent_and_reponse_is_unknown_Then_send_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->send( 0, pkt_buffer_with_reefer_powered_batery_and_location, sizeof( pkt_buffer_with_reefer_powered_batery_and_location ) );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_reboot_command_is_sent_and_reponse_is_correct_Then_reboot_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n+EVENT=0,0\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->reboot();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}


TEST_F( Given_a_murata93_controller, When_the_reboot_command_is_sent_and_reponse_is_incorrect_Then_reboot_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n+E*ENl=0,0\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->reboot();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_reboot_command_is_sent_and_reponse_is_wrong_Then_reboot_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->reboot();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_reboot_command_is_sent_and_reponse_is_unknown_Then_reboot_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->reboot();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_get_status_connection_command_is_sent_and_reponse_is_connection_lost_Then_get_status_connection_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n+EVENT=2,0\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t status    = 1;
    ret_val           = murata93_controller->get_status_connection( status );

    // ASSERT
    EXPECT_EQ( status, Conn_status::cipstatus_code_unattached );
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_get_status_connection_command_is_sent_and_reponse_is_ok_connection_Then_get_status_connection_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n+EVENT=2,1\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t status    = 0;
    ret_val           = murata93_controller->get_status_connection( status );

    // ASSERT
    EXPECT_EQ( status, Conn_status::cipstatus_code_connected );
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_get_status_connection_command_is_sent_and_reponse_is_incorrect_Then_get_status_connection_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t status    = 1;
    ret_val           = murata93_controller->get_status_connection( status );

    // ASSERT
    EXPECT_EQ( status, Conn_status::cipstatus_code_unattached );
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_get_status_connection_command_is_sent_and_reponse_is_unknown_Then_get_status_connection_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t status    = 1;
    ret_val           = murata93_controller->get_status_connection( status );

    // ASSERT
    EXPECT_EQ( status, Conn_status::cipstatus_code_unattached );
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_join_command_is_sent_and_reponse_is_join_accepted_Then_join_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n+EVENT=1,1\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->join();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_join_command_is_sent_and_reponse_is_join_rejected_Then_join_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n+EVENT=1,0\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->join();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_join_command_is_sent_and_reponse_is_incorrect_Then_join_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->join();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_join_command_is_sent_and_reponse_is_unknown_Then_join_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller->join();

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_check_version_command_is_sent_and_reponse_is_correct_Then_check_version_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK=1.1.03,Dec 20 2018 15:00:56\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    bool version      = false;
    ret_val           = murata93_controller->check_version( version );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
    EXPECT_EQ( version, true );
}

TEST_F( Given_a_murata93_controller, When_the_check_version_command_is_sent_and_reponse_is_incorrect_Then_check_version_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    bool version      = false;
    ret_val           = murata93_controller->check_version( version );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
    EXPECT_EQ( version, false );
}

TEST_F( Given_a_murata93_controller, When_the_check_version_command_is_sent_and_reponse_is_unknown_Then_check_version_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    bool version      = false;
    ret_val           = murata93_controller->check_version( version );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
    EXPECT_EQ( version, false );
}

TEST_F( Given_a_murata93_controller, When_the_get_deveui_command_is_sent_and_reponse_is_correct_Then_get_deveui_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK=01020304AABBCCDD\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t deveui[16];
    memset( deveui, 0, 16 );
    ret_val = murata93_controller->get_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
    EXPECT_THAT( std::vector<uint8_t>( &deveui[0], &deveui[16] ), ElementsAreArray( { '0', '1', '0', '2', '0', '3', '0', '4', 'A', 'A', 'B', 'B', 'C', 'C', 'D', 'D' } ) );
}

TEST_F( Given_a_murata93_controller, When_the_get_deveui_command_is_sent_and_reponse_is_incorrect_Then_get_deveui_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t deveui[16];
    memset( deveui, 0, 16 );
    ret_val = murata93_controller->get_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_get_deveui_command_is_sent_and_reponse_is_long_Then_get_deveui_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+OK=01024AABBCCDDFFEE\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t deveui[16];
    memset( deveui, 0, 16 );
    ret_val = murata93_controller->get_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_timeout );
}

TEST_F( Given_a_murata93_controller, When_the_get_deveui_command_is_sent_and_reponse_is_shorter_Then_get_deveui_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+OK=01024AABBCCDD\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t deveui[16];
    memset( deveui, 0, 16 );
    ret_val = murata93_controller->get_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_timeout );
}

TEST_F( Given_a_murata93_controller, When_the_get_deveui_command_is_sent_and_reponse_is_unknown_Then_get_deveui_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t deveui[16];
    memset( deveui, 0, 16 );
    ret_val = murata93_controller->get_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_set_deveui_command_is_sent_and_reponse_is_correct_Then_set_deveui_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint32_t deveui   = 0;
    ret_val           = murata93_controller->set_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_set_deveui_command_is_sent_and_reponse_is_wrong_Then_set_deveui_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t deveui    = 0;
    ret_val           = murata93_controller->set_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_set_deveui_command_is_sent_and_reponse_is_unknown_Then_set_deveui_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t deveui    = 0;
    ret_val           = murata93_controller->set_deveui( deveui );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_get_band_command_is_sent_and_reponse_is_correct_Then_get_band_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK=5\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t band      = 0;
    ret_val           = murata93_controller->get_band( band );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
    EXPECT_EQ( band, 5 );
}

TEST_F( Given_a_murata93_controller, When_the_get_band_command_is_sent_and_reponse_is_wrong_Then_get_band_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t band      = 0;
    ret_val           = murata93_controller->get_band( band );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
    EXPECT_EQ( band, 0 );
}

TEST_F( Given_a_murata93_controller, When_the_get_band_command_is_sent_and_reponse_is_unknown_Then_get_band_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t band      = 0;
    ret_val           = murata93_controller->get_band( band );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_set_band_command_is_sent_and_reponse_is_correct_Then_set_band_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t band      = 0;
    ret_val           = murata93_controller->set_band( band );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller, When_the_set_band_command_is_sent_and_reponse_is_wrong_Then_set_band_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t band      = 0;
    ret_val           = murata93_controller->set_band( band );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller, When_the_set_band_command_is_sent_and_reponse_is_unknown_Then_set_band_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t band      = 0;
    ret_val           = murata93_controller->set_band( band );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_get_rssi_command_is_sent_and_reponse_is_correct_Then_get_rssi_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK=-90,6\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t rssi      = 0;
    uint8_t snr       = 0;
    ret_val           = murata93_controller->get_rssi( rssi, snr );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
    EXPECT_EQ( rssi, 90 );
    EXPECT_EQ( snr, 6 );
}

TEST_F( Given_a_murata93_controller, When_the_get_rssi_command_is_sent_and_reponse_is_wrong_Then_get_rssi_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t rssi      = 0;
    uint8_t snr       = 0;
    ret_val           = murata93_controller->get_rssi( rssi, snr );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
    EXPECT_EQ( rssi, 0 );
    EXPECT_EQ( snr, 0 );
}

TEST_F( Given_a_murata93_controller, When_the_get_rssi_command_is_sent_and_reponse_is_unknown_Then_get_rssi_command_is_not_executed ) {
    // ARRANGE
    char resp_command[]            = "+unknown\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    uint8_t rssi      = 0;
    uint8_t snr       = 0;
    ret_val           = murata93_controller->get_rssi( rssi, snr );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_comms );
}

TEST_F( Given_a_murata93_controller, When_the_receive_command_is_sent_and_reponse_is_correct_Then_the_message_is_received ) {
    // ARRANGE
    char resp_command[]            = "+RECV=1,11\r\n\r\n68656C6C6F20776F726C64";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_fatal;
    uint16_t len      = 0;
    uint8_t port      = 1;
    uint8_t buffer[20];
    memset( buffer, 0, 20 );
    ret_val = murata93_controller->receive( 1, buffer, len, 20 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
    EXPECT_EQ( port, 1 );
    EXPECT_EQ( len, 11 );
    EXPECT_THAT( std::vector<uint8_t>( &buffer[0], &buffer[len] ), ElementsAreArray( { 'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd' } ) );
}

TEST_F( Given_a_murata93_controller, When_the_receive_command_is_sent_and_reponse_is_wrong_size_len_Then_the_message_is_not_received ) {
    // ARRANGE
    char resp_command[]            = "+RECV=1,10\r\n\r\n68656C6C6F20776F726C";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_success;
    uint16_t len      = 0;
    uint8_t buffer[20];
    memset( buffer, 0, 20 );
    ret_val = murata93_controller->receive( 1, buffer, len, 5 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_params );
}

TEST_F( Given_a_murata93_controller, When_the_set_params_command_is_sent_and_reponse_is_correct_Then_the_parameters_are_saved ) {
    // ARRANGE
    char eui[] = "008000000000E19C";
    char key[] = "531BD9C5EC5D8BA5EF3B262CEBFB3E66";
    char aux_buffer_eui[17];
    char aux_buffer_key[33];

    // ACT
    murata93_controller->set_params( eui, key );
    murata93_controller->get_appeui( aux_buffer_eui );
    murata93_controller->get_appkey( aux_buffer_key );

    // ASSERT
    uint8_t len_eui = strlen( aux_buffer_eui );
    uint8_t len_key = strlen( aux_buffer_key );
    EXPECT_EQ( strlen( aux_buffer_eui ), strlen( eui ) );
    EXPECT_THAT( std::vector<uint8_t>( &aux_buffer_eui[0], &aux_buffer_eui[len_eui] ),
                 ElementsAreArray( { '0', '0', '8', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'E', '1', '9', 'C' } ) );
    EXPECT_EQ( strlen( aux_buffer_key ), strlen( key ) );
    EXPECT_THAT( std::vector<uint8_t>( &aux_buffer_key[0], &aux_buffer_key[len_key] ),
                 ElementsAreArray( { '5', '3', '1', 'B', 'D', '9', 'C', '5', 'E', 'C', '5', 'D', '8', 'B', 'A', '5',
                                     'E', 'F', '3', 'B', '2', '6', '2', 'C', 'E', 'B', 'F', 'B', '3', 'E', '6', '6' } ) );
}

TEST_F( Given_a_murata93_controller_autoack, When_the_send_command_is_sent_and_reponse_is_correct_Then_send_command_is_executed_correctly ) {
    // ARRANGE
    char resp_command[]            = "+OK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller_autoack->send( 0, pkt_buffer_with_reefer_powered_batery_and_location, sizeof( pkt_buffer_with_reefer_powered_batery_and_location ) );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
}

TEST_F( Given_a_murata93_controller_autoack, When_the_send_command_is_sent_and_reponse_is_incorrect_Then_send_command_is_executed_incorrectly ) {
    // ARRANGE
    char resp_command[]            = "+ERR=3\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_init;
    ret_val           = murata93_controller_autoack->send( 0, pkt_buffer_with_reefer_powered_batery_and_location, sizeof( pkt_buffer_with_reefer_powered_batery_and_location ) );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_command );
}

TEST_F( Given_a_murata93_controller_autoack, When_the_receive_command_is_sent_and_reponse_is_correct_Then_the_message_is_received ) {
    // ARRANGE
    uint8_t ack_pkt[]              = { 44, 254, 0, 0, 0, 254, 0, 0, 0, 1, 0, 0, 0, 0, 0, 18, 0 };
    char resp_command[]            = "+ACK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_err_fatal;
    uint16_t len      = 0;
    uint8_t buffer[20];
    memset( buffer, 0, 20 );
    ret_val = murata93_controller_autoack->receive( 1, buffer, len, 20 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_success );
    EXPECT_EQ( len, sizeof( ack_pkt ) );
    EXPECT_THAT( std::vector<uint8_t>( &buffer[0], &buffer[sizeof( ack_pkt )] ), ElementsAreArray( { 44, 254, 0, 0, 0, 254, 0, 0, 0, 1, 0, 0, 0, 0, 0, 18, 0 } ) );
}

TEST_F( Given_a_murata93_controller_autoack, When_the_receive_command_is_sent_and_reponse_is_wrong_size_len_Then_the_message_is_not_received ) {
    // ARRANGE
    char resp_command[]            = "+ACK\r\n\r\n";
    const uint8_t len_resp_command = strlen( resp_command );
    put_fifo_rx_test_buffer( (uint8_t*)resp_command, len_resp_command );

    // ACT
    Wtc_err_t ret_val = wtc_success;
    uint16_t len      = 0;
    uint8_t buffer[20];
    memset( buffer, 0, 20 );
    ret_val = murata93_controller_autoack->receive( 1, buffer, len, 3 );

    // ASSERT
    EXPECT_EQ( ret_val, wtc_err_params );
}