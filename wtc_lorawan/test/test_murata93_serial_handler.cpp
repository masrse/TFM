/**
  \brief Test para la clase murata93_handler
*/

#include "gtest/gtest.h"
#include "gmock/gmock.h"
using testing::An;
using testing::AtLeast;
using testing::Exactly;
using testing::InSequence;
using testing::Return;

// Selección del hardware a utilizar
// #define HW_RTLS_V4_0_X
#define HW_RTLS_V4_1_X
#include "boards.h"

#include "murata93_serial_handler.h"
#include "conn_status_code.h"
#include "Mock_uC_interface.h"
#include <stdlib.h>

static const uint8_t len_fifo_rx_test = 110;
static const uint8_t len_fifo_tx_test = 110;
Fifo<uint8_t> murata93_fifo_rx_test( len_fifo_rx_test );
Fifo<uint8_t> murata93_fifo_tx_test( len_fifo_tx_test );

// Mocks globales usados en los diferentes tests
extern MockuC_DelayGenerator* mock_uC_DelayGenerator_obj;
extern MockuC_SerialController* mock_uC_SerialController_obj;

/**
  \class Tester_murata93_serial_handler
*/
class Tester_murata93_serial_handler: public Murata93_serial_handler {
  public:
    Tester_murata93_serial_handler( Wtc_serial_interface& wtc_serial_murata93, Fifo<uint8_t>& fifo_rx, Fifo<uint8_t>& fifo_tx, MockuC_DelayGenerator& delayGenerator,
                                    MockuC_SerialController& wtc_serial_for_debug ) :
        Murata93_serial_handler( wtc_serial_murata93, fifo_rx, fifo_tx, delayGenerator, wtc_serial_for_debug ){};
    Parse_resp_state_t get_state() const {
        return state;
    }

    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event1;
};

/**
    Fixture class para los diferentes test
*/
class Fixture_murata93_serial_handler: public ::testing::Test {
  protected:
    void SetUp() override {
        mock_uC_DelayGenerator_obj   = uC_DelayGenerator_Instance();
        mock_uC_SerialController_obj = uC_SerialController_Instance();

        murata93_serial_handler =
            new Tester_murata93_serial_handler( wtc_serial_murata93_test, murata93_fifo_rx_test, murata93_fifo_tx_test, *mock_uC_DelayGenerator_obj, *mock_uC_SerialController_obj );
    }

    void TearDown() override {
        release_uC_DelayGenerator();
        release_uC_SerialController();
        murata93_serial_handler->flush();
        murata93_serial_handler->reset();
    }

    uC_Pseudo_Wtc_serial_interface wtc_serial_murata93_test;
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event;
    Murata93_ATCmd_evts::ATCmd_resp_evt_t event1;
    Tester_murata93_serial_handler* murata93_serial_handler;
};

static void put_fifo_rx_test_buffer( const uint8_t* data, uint8_t size_data ) {
    for ( uint8_t i = 0; i < size_data; i++ ) {
        murata93_fifo_rx_test.put( data[i] );
    }
}

TEST_F( Fixture_murata93_serial_handler, If_fiforx_is_empty_Then_there_are_no_events ) {
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 0 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_does_not_have_plus_Then_there_are_no_events ) {
    static const char resp_msg_test[] = "OK\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_test, sizeof( resp_msg_test ) - 1 );
    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 0 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_does_not_have_terminators_bytes_Then_there_are_no_events ) {
    static const char resp_msg_test[] = "+OK01234567890123456789012345678901234567890123456789012345678901234567 \
            8901234567890123456789012345678901234567890123456789012345678901234567890123456789";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_test, sizeof( resp_msg_test ) - 1 );
    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 0 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_ok_Then_event_ok_is_generated ) {
    static const char resp_msg_ok_test[] = "+OK\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ok_test, sizeof( resp_msg_ok_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}


TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_ok_and_band_parameter_Then_event_ok_with_param_is_generated_and_band_is_stored ) {
    static const char resp_msg_ok_with_params_test[] = "+OK=5\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ok_with_params_test, sizeof( resp_msg_ok_with_params_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params );
    EXPECT_EQ( event.params.type_band.band, 5 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_ok_and_baudrate_parameter_Then_event_ok_with_param_is_generated_and_baudrate_is_stored ) {
    static const char resp_msg_ok_with_params_test[] = "+OK=19200,8,1,0,0\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ok_with_params_test, sizeof( resp_msg_ok_with_params_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params );
    EXPECT_EQ( event.params.baud_rate.baudrate, 19200 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_ok_and_rssi_and_snr_parameters_Then_event_ok_with_param_is_generated_and_rssy_and_snr_are_stored ) {
    static const char resp_msg_ok_with_params_test[] = "+OK=-93,25\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ok_with_params_test, sizeof( resp_msg_ok_with_params_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params );
    EXPECT_EQ( event.params.rf_parameters.rssi, 93 );
    EXPECT_EQ( event.params.rf_parameters.snr, 25 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_ok_and_eui_parameter_Then_event_ok_with_param_is_generated_and_eui_is_stored ) {
    static const char resp_msg_ok_with_params_test[] = "+OK=01020304AABBCCDD\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ok_with_params_test, sizeof( resp_msg_ok_with_params_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok_with_params );
    EXPECT_STREQ( (char*)event.params.deveui.eui, "01020304AABBCCDD" );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_error_Then_event_error_is_generated ) {
    static const char resp_msg_error_test[] = "+ERR=1\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_error_test, sizeof( resp_msg_error_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_error );
    EXPECT_EQ( event.params.error_type.error, Murata93_ATCmd_evts::atcmd_resp_error_code_cmd_unknown );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_ack_Then_event_ack_is_generated ) {
    static const char resp_msg_ack_test[] = "+ACK\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ack_test, sizeof( resp_msg_ack_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ack );
    EXPECT_EQ( event.params.ack_response.ack, 1 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_noack_Then_event_noack_is_generated ) {
    static const char resp_msg_noack_test[] = "+NOACK\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_noack_test, sizeof( resp_msg_noack_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_noack );
    EXPECT_EQ( event.params.ack_response.ack, 0 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_fw_version_Then_event_fw_version_is_generated ) {
    static const char resp_msg_fw_version_test[] = "+OK=1.1.03,Dec 20 2018 15:00:56\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_fw_version_test, sizeof( resp_msg_fw_version_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_fw_version );
    EXPECT_EQ( event.params.version.version_ok, true );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_attached_conection_Then_event_attached_conection_is_generated ) {
    static const char resp_msg_attached_conection_test[] = "+EVENT=1,1\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_attached_conection_test, sizeof( resp_msg_attached_conection_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_event );
    EXPECT_EQ( event.params.device_status.type, 1 );
    EXPECT_EQ( event.params.device_status.event_number, 1 );
    EXPECT_EQ( event.params.device_status.status, Conn_status::cipstatus_code_attached );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_connected_conection_Then_event_connected_conection_is_generated ) {
    static const char resp_msg_connected_conection_test[] = "+EVENT=2,1\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_connected_conection_test, sizeof( resp_msg_connected_conection_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_event );
    EXPECT_EQ( event.params.device_status.type, 2 );
    EXPECT_EQ( event.params.device_status.event_number, 1 );
    EXPECT_EQ( event.params.device_status.status, Conn_status::cipstatus_code_connected );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_closed_conection_Then_event_closed_conection_is_generated ) {
    static const char resp_msg_closed_conection_test[] = "+EVENT=2,0\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_closed_conection_test, sizeof( resp_msg_closed_conection_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_event );
    EXPECT_EQ( event.params.device_status.type, 2 );
    EXPECT_EQ( event.params.device_status.event_number, 0 );
    EXPECT_EQ( event.params.device_status.status, Conn_status::cipstatus_code_unattached );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_unattached_conection_Then_event_unattached_conection_is_generated ) {
    static const char resp_msg_unattached_conection_test[] = "+EVENT=0,0\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_unattached_conection_test, sizeof( resp_msg_unattached_conection_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_event );
    EXPECT_EQ( event.params.device_status.type, 0 );
    EXPECT_EQ( event.params.device_status.event_number, 0 );
    EXPECT_EQ( event.params.device_status.status, Conn_status::cipstatus_code_unattached );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_unattached_conection_without_iqual_character_Then_no_event_is_generated ) {
    static const char resp_msg_unattached_conection_test[] = "+EVENT0,0\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_unattached_conection_test, sizeof( resp_msg_unattached_conection_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 0 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_received_msg_with_length_10_Then_event_received_msg_is_generated_and_received_data ) {
    static const char resp_received_msg_test[] = "+RECV=1,10\r\n\r\n68656C6C6F20776F726C";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_received_msg_test, sizeof( resp_received_msg_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data );
    EXPECT_EQ( event.params.received_data.port, 1 );
    EXPECT_EQ( event.params.received_data.length, 10 );
    EXPECT_STREQ( (char*)event.params.received_data.response, "hello worl" );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_received_msg_with_length_40_Then_event_received_msg_is_generated_and_received_data ) {
    static const char resp_received_msg_test[] = "+RECV=1,40\r\n\r\n68656C6C6F20776F726C68656C6C6F20776F726C68656C6C6F20776F726C68656C6C6F20776F726C";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_received_msg_test, sizeof( resp_received_msg_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data );
    EXPECT_EQ( event.params.received_data.port, 1 );
    EXPECT_EQ( event.params.received_data.length, 40 );
    EXPECT_STREQ( (char*)event.params.received_data.response, "hello worlhello worlhello worlhello worl" );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_msg_ok_and_msg_attached_conection_Then_event_ok_and_attached_conection_are_generated ) {
    static const char resp_msg_ok_test[]                 = "+OK\r\n\r\n";
    static const char resp_msg_attached_conection_test[] = "+EVENT=1,1\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ok_test, sizeof( resp_msg_ok_test ) - 1 );
    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_attached_conection_test, sizeof( resp_msg_attached_conection_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 2 );
    murata93_serial_handler->resp_evt_get( event );
    murata93_serial_handler->resp_evt_get( event1 );

    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok );

    EXPECT_EQ( event1.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_event );
    EXPECT_EQ( event1.params.device_status.type, 1 );
    EXPECT_EQ( event1.params.device_status.event_number, 1 );
    EXPECT_EQ( event1.params.device_status.status, Conn_status::cipstatus_code_attached );

    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_msg_ok_and_received_msg_Then_event_ok_and_received_are_generated ) {
    static const char resp_msg_ok_test[]       = "+OK\r\n\r\n";
    static const char resp_received_msg_test[] = "+RECV=1,10\r\n\r\n68656C6C6F20776F726C";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_msg_ok_test, sizeof( resp_msg_ok_test ) - 1 );
    put_fifo_rx_test_buffer( (const uint8_t*)resp_received_msg_test, sizeof( resp_received_msg_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 2 );
    murata93_serial_handler->resp_evt_get( event );
    murata93_serial_handler->resp_evt_get( event1 );

    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok );

    EXPECT_EQ( event1.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data );
    EXPECT_EQ( event1.params.received_data.port, 1 );
    EXPECT_EQ( event1.params.received_data.length, 10 );
    EXPECT_STREQ( (char*)event1.params.received_data.response, "hello worl" );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_three_unknown_msg_Then_three_event_unknown_msg_are_generated ) {
    static const char resp_unknown_msg_one_test[]   = "+one\r\n\r\n";
    static const char resp_unknown_msg_two_test[]   = "+two\r\n\r\n";
    static const char resp_unknown_msg_three_test[] = "+three\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_unknown_msg_one_test, sizeof( resp_unknown_msg_one_test ) - 1 );
    put_fifo_rx_test_buffer( (const uint8_t*)resp_unknown_msg_two_test, sizeof( resp_unknown_msg_two_test ) - 1 );
    put_fifo_rx_test_buffer( (const uint8_t*)resp_unknown_msg_three_test, sizeof( resp_unknown_msg_three_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 3 );

    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp );
    EXPECT_EQ( event.params.unknown_response.resp_len, 4 );
    EXPECT_STREQ( (char*)event.params.unknown_response.p_resp, "+one" );

    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp );
    EXPECT_EQ( event.params.unknown_response.resp_len, 4 );
    EXPECT_STREQ( (char*)event.params.unknown_response.p_resp, "+two" );

    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp );
    EXPECT_EQ( event.params.unknown_response.resp_len, 6 );
    EXPECT_STREQ( (char*)event.params.unknown_response.p_resp, "+three" );
}

TEST_F( Fixture_murata93_serial_handler, If_the_answer_is_three_unknown_msg_Then_only_the_last_three_event_unknown_msg_are_generated ) {
    static const char resp_unknown_msg_lost_test[]  = "+msg lost\r\n\r\n";
    static const char resp_unknown_msg_one_test[]   = "+one\r\n\r\n";
    static const char resp_unknown_msg_two_test[]   = "+two\r\n\r\n";
    static const char resp_unknown_msg_three_test[] = "+three\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_unknown_msg_lost_test, sizeof( resp_unknown_msg_lost_test ) - 1 );
    put_fifo_rx_test_buffer( (const uint8_t*)resp_unknown_msg_one_test, sizeof( resp_unknown_msg_one_test ) - 1 );
    put_fifo_rx_test_buffer( (const uint8_t*)resp_unknown_msg_two_test, sizeof( resp_unknown_msg_two_test ) - 1 );
    put_fifo_rx_test_buffer( (const uint8_t*)resp_unknown_msg_three_test, sizeof( resp_unknown_msg_three_test ) - 1 );

    murata93_serial_handler->run_task();

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 4 );

    murata93_serial_handler->resp_evt_get( event );
    // This message is lost, len is 9 ("msg lost") while content is three
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp );
    EXPECT_EQ( event.params.unknown_response.resp_len, 9 );
    EXPECT_STREQ( (char*)event.params.unknown_response.p_resp, "+three" );

    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp );
    EXPECT_EQ( event.params.unknown_response.resp_len, 4 );
    EXPECT_STREQ( (char*)event.params.unknown_response.p_resp, "+one" );

    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp );
    EXPECT_EQ( event.params.unknown_response.resp_len, 4 );
    EXPECT_STREQ( (char*)event.params.unknown_response.p_resp, "+two" );

    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_unknown_resp );
    EXPECT_EQ( event.params.unknown_response.resp_len, 6 );
    EXPECT_STREQ( (char*)event.params.unknown_response.p_resp, "+three" );
}

TEST_F( Fixture_murata93_serial_handler, if_call_flush_When_fifo_rx_is_available_Then_fifo_rx_will_be_emptied ) {
    static const char resp_received_msg_test[] = "/REC.=1*37\r\n\r\n";

    put_fifo_rx_test_buffer( (const uint8_t*)resp_received_msg_test, sizeof( resp_received_msg_test ) - 1 );

    murata93_serial_handler->flush();

    EXPECT_EQ( murata93_fifo_rx_test.available(), 0 );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
}

TEST_F( Fixture_murata93_serial_handler, if_call_parse_rx_byte_with_ok_answer_Then_state_machine_and_event_generated_are_ok ) {
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
    murata93_serial_handler->parse_rx_byte( '+' );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::wait_resp_msg );
    murata93_serial_handler->parse_rx_byte( 'O' );
    murata93_serial_handler->parse_rx_byte( 'K' );
    murata93_serial_handler->parse_rx_byte( '\r' );
    murata93_serial_handler->parse_rx_byte( '\n' );
    murata93_serial_handler->parse_rx_byte( '\r' );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::wait_resp_end );
    murata93_serial_handler->parse_rx_byte( '\n' );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_ok );
    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 0 );
}

TEST_F( Fixture_murata93_serial_handler, if_call_parse_rx_byte_with_received_answer_Then_state_machine_and_event_generated_are_ok ) {
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );
    murata93_serial_handler->parse_rx_byte( '+' );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::wait_resp_msg );
    murata93_serial_handler->parse_rx_byte( 'R' );
    murata93_serial_handler->parse_rx_byte( 'E' );
    murata93_serial_handler->parse_rx_byte( 'C' );
    murata93_serial_handler->parse_rx_byte( 'V' );
    murata93_serial_handler->parse_rx_byte( '=' );
    murata93_serial_handler->parse_rx_byte( '1' );
    murata93_serial_handler->parse_rx_byte( ',' );
    murata93_serial_handler->parse_rx_byte( '3' );
    murata93_serial_handler->parse_rx_byte( '\r' );
    murata93_serial_handler->parse_rx_byte( '\n' );
    murata93_serial_handler->parse_rx_byte( '\r' );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::wait_resp_end );
    murata93_serial_handler->parse_rx_byte( '\n' );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::save_data );
    murata93_serial_handler->parse_rx_byte( '3' );
    murata93_serial_handler->parse_rx_byte( '2' );
    murata93_serial_handler->parse_rx_byte( '3' );
    murata93_serial_handler->parse_rx_byte( '2' );
    murata93_serial_handler->parse_rx_byte( '3' );
    murata93_serial_handler->parse_rx_byte( '4' );
    EXPECT_EQ( murata93_serial_handler->get_state(), Murata93_serial_handler::idle );

    EXPECT_EQ( murata93_serial_handler->resp_evt_available(), 1 );
    murata93_serial_handler->resp_evt_get( event );
    EXPECT_EQ( event.code, Murata93_ATCmd_evts::atcmd_resp_evt_code_received_data );
    EXPECT_EQ( event.params.received_data.port, 1 );
    EXPECT_EQ( event.params.received_data.length, 3 );
    EXPECT_STREQ( (char*)event.params.received_data.response, "224" );
}