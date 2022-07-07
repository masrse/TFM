#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "wtc_base.h"
#include "sim8xx_controller.h"
#include "murata93_controller.h"
#include "wtc_network.h"
#include "time_util.h"
#include "fifo.h"
#include "Mockfifo.h"
#include "pkt.h"
#include "payload_mgr.h"
#include "model_ids.h"
#include "commands_ids.h"
#include "router.h"
#include "model_config_network_params.h"
#include "model_config_lora_params.h"
#include "Mock_uC_interface.h"

// #define HW_RTLS_V4_0_X
#define HW_RTLS_V4_1_X
#include "boards.h"

#define PAYLOAD_LENGHT 100
#define ATTEMPTS_ACK   2

using testing::An;
using testing::AnyNumber;
using testing::AtLeast;
using testing::ElementsAreArray;
using testing::Exactly;
using testing::InSequence;
using ::testing::Mock;
using testing::Return;

uint8_t d_register_network, d_port_network = 1;

// Mocks globales usados en los diferentes tests
extern MockuC_DelayGenerator* mock_uC_DelayGenerator_obj;
extern MockuC_SerialController* mock_uC_SerialController_obj;

class Network_controller_interface_dummy: public Network_controller_interface {
  public:
    Network_controller_interface_dummy() {
        memset( appeui, 0, sizeof( appeui ) );
        memset( appkey, 0, sizeof( appkey ) );
        memset( send_buffer, 0, sizeof( send_buffer ) );
        memset( receive_buffer, 0, sizeof( receive_buffer ) );
        memset( ip, 0, sizeof( ip ) );
        controller_id           = id_controller_sim8xx;
        on_counter              = 0;
        off_counter             = 0;
        disconnect_flag         = 0;
        index_conn_status_dummy = 0;
        run_task_is_called = 0;
        listen_is_called = 0;
    }

    void on( void ) {
        on_counter++;
    }

    uint8_t get_on_counter( void ) {
        return on_counter;
    }

    void off( void ) {
        off_counter++;
    }

    uint8_t get_off_counter( void ) {
        return off_counter;
    }

    void run_task( void ) {
        run_task_is_called = true;
    }

    bool get_run_task_is_called( void ) {
        return run_task_is_called;
    }

    Wtc_err_t check_attach( void ) {
        return wtc_success;
    }

    Wtc_err_t join( void ) {
        return wtc_success;
    }

    Wtc_err_t conn_status( Conn_config::Conn_id_t conn_id, uint8_t& status ) {
        status = conn_status_dummy[index_conn_status_dummy++];
        return wtc_success;
    }

    void set_conn_status( uint8_t conn_status_0, uint8_t conn_status_1 ) {
        conn_status_dummy[0] = conn_status_0;
        conn_status_dummy[1] = conn_status_1;
    }

    Wtc_err_t send( const Conn_config::Conn_id_t conn_id, const uint8_t buffer[], const uint16_t len ) {
        memcpy( send_buffer, buffer, len );
        send_len = len;
        return wtc_success;
    }

    uint8_t* get_send_buffer( void ) {
        return send_buffer;
    }

    uint8_t get_send_len( void ) {
        return send_len;
    }

    Wtc_err_t connect( const Conn_config::Conn_id_t conn_id, const char* ip_0, const uint16_t port_0, const Conn_config::Conn_type_t type_0 ) {
        strcpy( ip, ip_0 );
        port            = port_0;
        connection_type = type_0;
        return wtc_success;
    }

    char* get_ip( void ) {
        return ip;
    }

    uint16_t get_port( void ) {
        return port;
    }

    Conn_config::Conn_type_t get_connection_type( void ) {
        return connection_type;
    }

    Wtc_err_t disconnect( const Conn_config::Conn_id_t conn_id ) {
        disconnect_flag = true;
        return wtc_success;
    }

    bool get_disconnect_flag( void ) {
        return disconnect_flag;
    }

    Wtc_err_t receive( const Conn_config::Conn_id_t conn_id, uint8_t buffer[], uint16_t& len, const uint16_t max_len, const uint32_t timeout = 5000 ) {
        len = receive_len;
        memcpy( buffer, receive_buffer, len );
        return wtc_success;
    }

    void set_receive_buffer( uint8_t* receive_buffer_0, uint8_t len ) {
        memcpy( receive_buffer, receive_buffer_0, len );
    }

    void set_receive_len( uint8_t receive_len_0 ) {
        receive_len = receive_len_0;
    }

    void listen( void ) {
        listen_is_called = true;
    }

    bool get_listen_is_called( void ) {
        return listen_is_called;
    }

    Id_controller_t get_id_controller( void ) {
        return controller_id;
    }

    void set_controller_id( Id_controller_t controller_id_0 ) {
        controller_id = controller_id_0;
    }

    Wtc_err_t set_params( char* appeui_0, char* appkey_0 ) {
        strcpy( appeui, appeui_0 );
        strcpy( appkey, appkey_0 );
        return wtc_success;
    }
    char* get_appeui( void ) {
        return appeui;
    }
    char* get_appkey( void ) {
        return appkey;
    }

    Wtc_err_t get_cell_info( uint16_t& mcc, uint16_t& mnc, uint32_t& cid, uint16_t& lac ) {
        return wtc_success;
    }

    uint32_t get_timestamp_from_nmea_frame( void ) {
        return 0U;
    }
  private:
    char appeui[2 * Config_lora_params::max_len_eui + 1];
    char appkey[2 * Config_lora_params::max_len_key + 1];
    Id_controller_t controller_id;
    uint8_t on_counter, off_counter;
    bool disconnect_flag;
    uint8_t send_buffer[100], receive_buffer[100], send_len, receive_len;
    uint8_t conn_status_dummy[2], index_conn_status_dummy;
    char ip[20];
    uint16_t port;
    Conn_config::Conn_type_t connection_type;
    bool run_task_is_called, listen_is_called;
};

class Network_tester: public Network {
  public:
    Network_tester( uC_SerialController& debugPrinter_0, uC_DelayGenerator& uC_delay_0, const uint16_t in_size_0, const uint8_t times_nok_limit_0 ) :
        Network( debugPrinter_0, uC_delay_0, in_size_0, times_nok_limit_0 ) {
        create_buffer_pkts_to_send_is_called = false;
    };

    uint16_t get_server_port( void ) {
        return server_port;
    }

    char* get_server_ip_address( void ) {
        return server_ip_address;
    }

    bool get_send_offline() {
        return send_offline;
    }

    void create_buffer_pkts_to_send( void ) {
        create_buffer_pkts_to_send_is_called = true;
    }

    bool get_create_buffer_pkts_to_send_flag( void ) {
        return create_buffer_pkts_to_send_is_called;
    }

    void set_buffer_pkts_offline( uint8_t* pkts_offline_0, int16_t len_buffer_pkts_0 ) {
        len_buffer_pkts = len_buffer_pkts_0;
        memcpy( pkts_offline, pkts_offline_0, len_buffer_pkts );
    }

    uint8_t* get_buffer_pkts_offline( void ) {
        return pkts_offline;
    }

  private:
    bool create_buffer_pkts_to_send_is_called;
};

class GivenAWtcNetwork: public testing::Test {
  public:
    void SetUp() override {
        // ARRANGE
        mock_uC_SerialController_obj = uC_SerialController_Instance();
        mock_uC_DelayGenerator_obj   = uC_DelayGenerator_Instance();
        network_controller_interface_dummy     = new Network_controller_interface_dummy();

        network_tester = new Network_tester( *mock_uC_SerialController_obj, *mock_uC_DelayGenerator_obj, PAYLOAD_LENGHT, ATTEMPTS_ACK );
    }
    void TearDown( void ) {
        release_uC_SerialController();
        release_uC_DelayGenerator();
    }
    Network_controller_interface_dummy* network_controller_interface_dummy;
    Network_tester* network_tester;
};

TEST_F( GivenAWtcNetwork, WhenTheControllerIdSetAllowsOfflineSending_ThenOfflineSendingIsOn ) {
    // ACT
    network_controller_interface_dummy->set_controller_id( id_controller_sim8xx );
    network_tester->add( network_controller_interface_dummy );

    // ASSERT
    ASSERT_EQ( true, network_tester->get_send_offline() );
};

TEST_F( GivenAWtcNetwork, WhenTheControllerIdSetAllowsOfflineSending_ThenOfflineSendingIsOff ) {
    // ACT
    network_controller_interface_dummy->set_controller_id( id_controller_murata93 );
    network_tester->add( network_controller_interface_dummy );

    // ASSERT
    ASSERT_EQ( false, network_tester->get_send_offline() );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsAddedAndItActivates_ThenTheControllerIsActivated ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );

    // ACT
    network_tester->on();

    // ASSERT
    ASSERT_EQ( 1, network_controller_interface_dummy->get_on_counter() );
};

TEST_F( GivenAWtcNetwork, WhenThreeCotrollerAreAddedAndTheseAreDesactivated_ThenTheControllersAreDesactivated ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );
    network_tester->add( network_controller_interface_dummy );
    network_tester->add( network_controller_interface_dummy );

    // ACT
    network_tester->off();

    // ASSERT
    ASSERT_EQ( 3, network_controller_interface_dummy->get_off_counter() );
};

TEST_F( GivenAWtcNetwork, WhenTheSchedulerTaskIsCalled_ThenTheAcctionsAreCalled ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );

    // ACT
    network_tester->run_task();

    // ASSERT
    ASSERT_EQ( true, network_controller_interface_dummy->get_run_task_is_called() );
    ASSERT_EQ( true, network_controller_interface_dummy->get_listen_is_called() );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsConnectedWithDiferentNetworkParameters_ThenTheNetworksParametersAreSavedAndTheControllerIsConnected ) {
    // ARRANGE
    Wtc_err_t connect_status;
    char ip[]{ 'd', 'e', 'v', 'e', 'l', 'o', 'p', '.', 'w', 'i', 't', 'r', 'a', 'c', '.', 'e', 's' };
    uint16_t port = 9600;
    network_tester->add( network_controller_interface_dummy );

    // ACT
    connect_status = network_tester->connect( ip, port, Conn_config::Conn_type_t::conn_type_tcp );

    // ASSERT
    ASSERT_EQ( wtc_success, connect_status );
    ASSERT_EQ( Conn_config::Conn_type_t::conn_type_tcp, network_controller_interface_dummy->get_connection_type() );
    ASSERT_EQ( port, network_controller_interface_dummy->get_port() );
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_ip(), network_controller_interface_dummy->get_ip() + sizeof( ip ) ),
                 ElementsAreArray( { 'd', 'e', 'v', 'e', 'l', 'o', 'p', '.', 'w', 'i', 't', 'r', 'a', 'c', '.', 'e', 's' } ) );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsConnectedAndABufferIsSent_ThenTheBufferIsSent ) {
    // ARRANGE
    Wtc_err_t send_status;
    uint8_t buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_connected, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send( buffer, sizeof( buffer ) );

    // ASSERT
    ASSERT_EQ( wtc_success, send_status );
    ASSERT_EQ( sizeof( buffer ), network_controller_interface_dummy->get_send_len() );
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_send_buffer(), network_controller_interface_dummy->get_send_buffer() + sizeof( buffer ) ),
                 ElementsAreArray( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } ) );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsConnectedAndACharBufferIsSent_ThenTheBufferIsSentAndCasted ) {
    // ARRANGE
    Wtc_err_t send_status;
    char buffer[] = { 'h', 'e', 'l', 'l', 'o' };
    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_connected, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send( buffer, sizeof( buffer ) );

    // ASSERT
    ASSERT_EQ( wtc_success, send_status );
    ASSERT_EQ( sizeof( buffer ), network_controller_interface_dummy->get_send_len() );
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_send_buffer(), network_controller_interface_dummy->get_send_buffer() + sizeof( buffer ) ),
                 ElementsAreArray( { 'h', 'e', 'l', 'l', 'o' } ) );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsConnectingAndABufferIsSent_ThenTheControllerConnectsAndTheBufferIsSent ) {
    // ARRANGE
    Wtc_err_t send_status;
    char ip[]{ 'd', 'e', 'v', 'e', 'l', 'o', 'p', '.', 'w', 'i', 't', 'r', 'a', 'c', '.', 'e', 's' };
    uint16_t port = 9600;
    network_tester->setup_connect( ip, port, Conn_config::Conn_type_t::conn_type_tcp );
    uint8_t buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_connecting, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send( buffer, sizeof( buffer ) );

    // ASSERT
    ASSERT_EQ( wtc_success, send_status );
    ASSERT_EQ( sizeof( buffer ), network_controller_interface_dummy->get_send_len() );
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_send_buffer(), network_controller_interface_dummy->get_send_buffer() + sizeof( buffer ) ),
                 ElementsAreArray( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } ) );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsClosedAndABufferIsSent_ThenTheControllerConnectsAndTheBufferIsSent ) {
    // ARRANGE
    Wtc_err_t send_status;
    char ip[]{ 'd', 'e', 'v', 'e', 'l', 'o', 'p', '.', 'w', 'i', 't', 'r', 'a', 'c', '.', 'e', 's' };
    uint16_t port = 9600;
    network_tester->setup_connect( ip, port, Conn_config::Conn_type_t::conn_type_tcp );
    uint8_t buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_closed, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send( buffer, sizeof( buffer ) );

    // ASSERT
    ASSERT_EQ( wtc_success, send_status );
    ASSERT_EQ( sizeof( buffer ), network_controller_interface_dummy->get_send_len() );
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_send_buffer(), network_controller_interface_dummy->get_send_buffer() + sizeof( buffer ) ),
                 ElementsAreArray( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } ) );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsDisconnectedAndABufferIsSent_ThenTheControllerConnectsAndTheBufferIsSent ) {
    // ARRANGE
    Wtc_err_t send_status;
    char ip[]{ 'd', 'e', 'v', 'e', 'l', 'o', 'p', '.', 'w', 'i', 't', 'r', 'a', 'c', '.', 'e', 's' };
    uint16_t port = 9600;
    network_tester->setup_connect( ip, port, Conn_config::Conn_type_t::conn_type_tcp );
    uint8_t buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_unattached, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send( buffer, sizeof( buffer ) );

    // ASSERT
    ASSERT_EQ( wtc_success, send_status );
    ASSERT_EQ( sizeof( buffer ), network_controller_interface_dummy->get_send_len() );
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_send_buffer(), network_controller_interface_dummy->get_send_buffer() + sizeof( buffer ) ),
                 ElementsAreArray( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } ) );
};

TEST_F( GivenAWtcNetwork, WhenNoControllerIsAddedAndAOfflineMessageIsSent_ThenTheOfflineMessageIsNotSent ) {
    // ARRANGE
    Wtc_err_t send_status;
    uint8_t offline_buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

    network_tester->set_buffer_pkts_offline( offline_buffer, sizeof( offline_buffer ) );

    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_connected, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send();

    // ASSERT
    ASSERT_EQ( wtc_err_comms, send_status );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsConnectedAndAOfflineMessageIsSent_ThenTheOfflineMessageIsSent ) {
    // ARRANGE
    Wtc_err_t send_status;
    uint8_t offline_buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    network_tester->set_buffer_pkts_offline( offline_buffer, sizeof( offline_buffer ) );

    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_connected, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send();

    // ASSERT
    ASSERT_EQ( wtc_success, send_status );

    EXPECT_THAT( std::vector<uint8_t>( network_tester->get_buffer_pkts_offline(), network_tester->get_buffer_pkts_offline() + sizeof( offline_buffer ) ),
                 ElementsAreArray( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 } ) );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsDisconnectedAndAOfflineMessageIsSent_ThenTheOfflineMessageIsSent ) {
    // ARRANGE
    Wtc_err_t send_status;

    char ip[]{ 'd', 'e', 'v', 'e', 'l', 'o', 'p', '.', 'w', 'i', 't', 'r', 'a', 'c', '.', 'e', 's' };
    uint16_t port = 9600;
    network_tester->setup_connect( ip, port, Conn_config::Conn_type_t::conn_type_tcp );

    uint8_t offline_buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 };
    network_tester->set_buffer_pkts_offline( offline_buffer, sizeof( offline_buffer ) );

    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_conn_status( Conn_status::cipstatus_code_unattached, Conn_status::cipstatus_code_connected );

    // ACT
    send_status = network_tester->send();

    // ASSERT
    ASSERT_EQ( wtc_success, send_status );

    EXPECT_THAT( std::vector<uint8_t>( network_tester->get_buffer_pkts_offline(), network_tester->get_buffer_pkts_offline() + sizeof( offline_buffer ) ),
                 ElementsAreArray( { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20 } ) );
};

TEST_F( GivenAWtcNetwork, WhenAControllerReceivesAIncorrectPkt_ThenThePktIsNotReceived ) {
    // ARRANGE
    Wtc_err_t receive_status;
    uint8_t buffer[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_receive_len( sizeof( buffer ) );
    network_controller_interface_dummy->set_receive_buffer( buffer, sizeof( buffer ) );

    // ACT
    receive_status = network_tester->receive();

    // ASSERT
    ASSERT_EQ( wtc_err_comms, receive_status );
};

TEST_F( GivenAWtcNetwork, WhenAControllerReceivesACorrectPkt_ThenThePktIsReceived ) {
    // ARRANGE
    Wtc_err_t receive_status;
    uint8_t buffer[] = { 44, 123, 0, 0, 0, 204, 39, 233, 2, 1, 0, 196, 240, 8, 97, 158, 0 };
    network_controller_interface_dummy->set_receive_len( sizeof( buffer ) );
    network_tester->add( network_controller_interface_dummy );
    network_controller_interface_dummy->set_receive_buffer( buffer, sizeof( buffer ) );

    // ACT
    receive_status = network_tester->receive();

    // ASSERT
    ASSERT_EQ( wtc_success, receive_status );
};

TEST_F( GivenAWtcNetwork, WhenAControllerIsDisconnect_ThenTheControllerIsDisconnected ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );

    // ACT
    network_tester->disconnect();

    // ASSERT
    ASSERT_EQ( 1, network_controller_interface_dummy->get_disconnect_flag() );
};

namespace Network_params_conf_build_pkt {
#define CONFIG_PORT 2600
    const char* config_address = "s16.witrac.es";

    const uint16_t size_pkt = 150;
    Pkt pkt_test( size_pkt );
    uint8_t payload_len_MD = 0;
    const uint8_t max_len  = 100;
    uint8_t payload_MD[max_len];

    void create_config_network_params_pkt_test_correct( void ) {
        Payload_mgr payload_lossy_MD( size_pkt );
        payload_lossy_MD.reset();
        payload_lossy_MD.put_uint8( model_id_config_network_params );
        payload_lossy_MD.put_uint16( CONFIG_PORT );
        for ( uint8_t i = 0; i < Config_network_params::max_address; i++ ) {
            payload_lossy_MD.put_uint8( config_address[i] );
        }
        memcpy( &payload_MD[payload_len_MD], payload_lossy_MD.get_bytes(), payload_lossy_MD.get_used_size() );
        payload_len_MD += payload_lossy_MD.get_used_size();
        pkt_test.build( 123, 123, cmd_network_params_conf, 1624350379, payload_MD, payload_len_MD );
        memset( payload_MD, '\0', sizeof( payload_MD ) );
        payload_len_MD = 0;
    }

    void create_config_network_params_pkt_test_incorrect_model( void ) {
        Payload_mgr payload_lossy_MD( size_pkt );
        payload_lossy_MD.reset();
        payload_lossy_MD.put_uint8( model_id_config_network_params + 1 );
        payload_lossy_MD.put_uint16( CONFIG_PORT );
        for ( uint8_t i = 0; i < Config_network_params::max_address; i++ ) {
            payload_lossy_MD.put_uint8( config_address[i] );
        }
        memcpy( &payload_MD[payload_len_MD], payload_lossy_MD.get_bytes(), payload_lossy_MD.get_used_size() );
        payload_len_MD += payload_lossy_MD.get_used_size();
        pkt_test.build( 123, 123, cmd_network_params_conf, 1624350379, payload_MD, payload_len_MD );
        memset( payload_MD, '\0', sizeof( payload_MD ) );
        payload_len_MD = 0;
    }

    void create_config_network_params_pkt_test_incorrect_command( void ) {
        create_config_network_params_pkt_test_correct();
        pkt_test.hdr->cmd = cmd_network_params_conf + 1;
    }

    void create_config_network_params_pkt_test_incorrect_crc( void ) {
        create_config_network_params_pkt_test_correct();
        pkt_test.msg[1] = 2;
    }

    void create_config_network_params_pkt_test_incorrect_length( void ) {
        create_config_network_params_pkt_test_correct();
        pkt_test.hdr->len = Config_network_params::get_size() + 10;
    }
}

TEST_F( GivenAWtcNetwork, WhenTheNetworkParamsPktIsReceivedWithIncorrectCommandId_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    Network_params_conf_build_pkt::create_config_network_params_pkt_test_incorrect_command();

    // ACT
    network_tester->parse( Network_params_conf_build_pkt::pkt_test );

    // ASSERT
    ASSERT_EQ( 0, network_tester->get_server_port() );
    ASSERT_EQ( NULL, network_tester->get_server_ip_address() );
};

TEST_F( GivenAWtcNetwork, WhenTheNetworkParamsPktIsReceivedWithIncorrectModelId_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    Network_params_conf_build_pkt::create_config_network_params_pkt_test_incorrect_model();

    // ACT
    network_tester->parse( Network_params_conf_build_pkt::pkt_test );

    // ASSERT
    ASSERT_EQ( 0, network_tester->get_server_port() );
    ASSERT_EQ( NULL, network_tester->get_server_ip_address() );
};

TEST_F( GivenAWtcNetwork, WhenTheNetworkParamsPktIsReceivedWithIncorrectCrc_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    Network_params_conf_build_pkt::create_config_network_params_pkt_test_incorrect_crc();

    // ACT
    network_tester->parse( Network_params_conf_build_pkt::pkt_test );

    // ASSERT
    ASSERT_EQ( 0, network_tester->get_server_port() );
    ASSERT_EQ( NULL, network_tester->get_server_ip_address() );
};

TEST_F( GivenAWtcNetwork, WhenTheNetworkParamsPktIsReceivedWithIncorrectLength_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    Network_params_conf_build_pkt::create_config_network_params_pkt_test_incorrect_length();

    // ACT
    network_tester->parse( Network_params_conf_build_pkt::pkt_test );

    // ASSERT
    ASSERT_EQ( 0, network_tester->get_server_port() );
    ASSERT_EQ( NULL, network_tester->get_server_ip_address() );
};

TEST_F( GivenAWtcNetwork, WhenTheNetworkParamsPktIsReceivedCorrectly_ThenTheValuesAreUpdatedAndTheAckIsGenerated ) {
    // ARRANGE
    Network_params_conf_build_pkt::create_config_network_params_pkt_test_correct();

    // ACT
    network_tester->parse( Network_params_conf_build_pkt::pkt_test );

    // ASSERT
    ASSERT_EQ( 2600, network_tester->get_server_port() );
    EXPECT_THAT( std::vector<uint8_t>( network_tester->get_server_ip_address(), network_tester->get_server_ip_address() + strlen( Network_params_conf_build_pkt::config_address ) ),
                 ElementsAreArray( { 's', '1', '6', '.', 'w', 'i', 't', 'r', 'a', 'c', '.', 'e', 's' } ) );
};

namespace Lora_params_conf_build_pkt {
    // APP_EUI "008000000000E19C";
    uint8_t config_app_eui[] = { 0, 128, 0, 0, 0, 0, 225, 156 };

    // APP_KEY "531BD9C5EC5D8BA5EF3B262CEBFB3E66";
    uint8_t config_app_key[] = { 83, 27, 217, 197, 236, 93, 139, 165, 239, 59, 38, 44, 235, 251, 62, 102 };

    const uint16_t size_pkt = 150;
    Pkt pkt_test( size_pkt );
    uint8_t payload_len_MD = 0;
    const uint8_t max_len  = 100;
    uint8_t payload_MD[max_len];

    void create_config_lora_params_pkt_test_correct( void ) {
        Payload_mgr payload_lossy_MD( size_pkt );
        payload_lossy_MD.reset();
        payload_lossy_MD.put_uint8( model_id_config_lora_params );
        for ( uint8_t i = 0; i < Config_lora_params::max_len_eui; i++ ) {
            payload_lossy_MD.put_uint8( config_app_eui[i] );
        }
        for ( uint8_t i = 0; i < Config_lora_params::max_len_key; i++ ) {
            payload_lossy_MD.put_uint8( config_app_key[i] );
        }
        memcpy( &payload_MD[payload_len_MD], payload_lossy_MD.get_bytes(), payload_lossy_MD.get_used_size() );
        payload_len_MD += payload_lossy_MD.get_used_size();
        pkt_test.build( 123, 123, cmd_lora_params_conf, 1624350379, payload_MD, payload_len_MD );
        memset( payload_MD, '\0', sizeof( payload_MD ) );
        payload_len_MD = 0;
    }

    void create_config_lora_params_pkt_test_incorrect_model( void ) {
        Payload_mgr payload_lossy_MD( size_pkt );
        payload_lossy_MD.reset();
        payload_lossy_MD.put_uint8( model_id_config_lora_params + 1 );
        for ( uint8_t i = 0; i < Config_lora_params::max_len_eui; i++ ) {
            payload_lossy_MD.put_uint8( config_app_eui[i] );
        }
        for ( uint8_t i = 0; i < Config_lora_params::max_len_key; i++ ) {
            payload_lossy_MD.put_uint8( config_app_key[i] );
        }
        memcpy( &payload_MD[payload_len_MD], payload_lossy_MD.get_bytes(), payload_lossy_MD.get_used_size() );
        payload_len_MD += payload_lossy_MD.get_used_size();
        pkt_test.build( 123, 123, cmd_lora_params_conf, 1624350379, payload_MD, payload_len_MD );
        memset( payload_MD, '\0', sizeof( payload_MD ) );
        payload_len_MD = 0;
    }

    void create_config_lora_params_pkt_test_incorrect_command( void ) {
        create_config_lora_params_pkt_test_correct();
        pkt_test.hdr->cmd = cmd_lora_params_conf + 1;
    }

    void create_config_lora_params_pkt_test_incorrect_crc( void ) {
        create_config_lora_params_pkt_test_correct();
        pkt_test.msg[1] = 2;
    }

    void create_config_lora_params_pkt_test_incorrect_length( void ) {
        create_config_lora_params_pkt_test_correct();
        pkt_test.hdr->len = Config_lora_params::get_size() + 10;
    }
}

TEST_F( GivenAWtcNetwork, WhenTheLoraParamsPktIsReceivedWithIncorrectCommandId_ThenTheValuesAreNotUpdatedd ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );
    Lora_params_conf_build_pkt::create_config_lora_params_pkt_test_incorrect_command();

    // ACT
    network_tester->parse( Lora_params_conf_build_pkt::pkt_test );

    // ASSERT
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appeui(), network_controller_interface_dummy->get_appeui() + sizeof( Lora_params_conf_build_pkt::config_app_eui ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );

    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appkey(), network_controller_interface_dummy->get_appkey() + sizeof( Lora_params_conf_build_pkt::config_app_key ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );
};

TEST_F( GivenAWtcNetwork, WhenThePktIsReceivedWithIncorrectModelId_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );
    Lora_params_conf_build_pkt::create_config_lora_params_pkt_test_incorrect_model();

    // ACT
    network_tester->parse( Lora_params_conf_build_pkt::pkt_test );

    // ASSERT
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appeui(), network_controller_interface_dummy->get_appeui() + sizeof( Lora_params_conf_build_pkt::config_app_eui ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );

    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appkey(), network_controller_interface_dummy->get_appkey() + sizeof( Lora_params_conf_build_pkt::config_app_key ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );
};

TEST_F( GivenAWtcNetwork, WhenThePktIsReceivedWithIncorrectCrc_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );
    Lora_params_conf_build_pkt::create_config_lora_params_pkt_test_incorrect_crc();

    // ACT
    network_tester->parse( Lora_params_conf_build_pkt::pkt_test );

    // ASSERT
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appeui(), network_controller_interface_dummy->get_appeui() + sizeof( Lora_params_conf_build_pkt::config_app_eui ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );

    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appkey(), network_controller_interface_dummy->get_appkey() + sizeof( Lora_params_conf_build_pkt::config_app_key ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );
};

TEST_F( GivenAWtcNetwork, WhenThePktIsReceivedWithIncorrectLength_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    network_tester->add( network_controller_interface_dummy );
    Lora_params_conf_build_pkt::create_config_lora_params_pkt_test_incorrect_length();

    // ACT
    network_tester->parse( Lora_params_conf_build_pkt::pkt_test );

    // ASSERT
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appeui(), network_controller_interface_dummy->get_appeui() + sizeof( Lora_params_conf_build_pkt::config_app_eui ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );

    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appkey(), network_controller_interface_dummy->get_appkey() + sizeof( Lora_params_conf_build_pkt::config_app_key ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );
};

TEST_F( GivenAWtcNetwork, WhenThePktIsReceivedWithControllerIdIncorrect_ThenTheValuesAreNotUpdated ) {
    // ARRANGE
    network_controller_interface_dummy->set_controller_id( id_controller_sim8xx );
    network_tester->add( network_controller_interface_dummy );
    Lora_params_conf_build_pkt::create_config_lora_params_pkt_test_correct();

    // ACT
    network_tester->parse( Lora_params_conf_build_pkt::pkt_test );

    // ASSERT
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appeui(), network_controller_interface_dummy->get_appeui() + sizeof( Lora_params_conf_build_pkt::config_app_eui ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );

    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appkey(), network_controller_interface_dummy->get_appkey() + sizeof( Lora_params_conf_build_pkt::config_app_key ) * 2 ),
                 ElementsAreArray( { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ) );
};

TEST_F( GivenAWtcNetwork, WhenThePktIsReceivedCorrectly_ThenTheValuesAreUpdatedAndTheAckIsGenerated ) {
    // ARRANGE
    network_controller_interface_dummy->set_controller_id( id_controller_murata93 );
    network_tester->add( network_controller_interface_dummy );
    Lora_params_conf_build_pkt::create_config_lora_params_pkt_test_correct();

    // ACT
    network_tester->parse( Lora_params_conf_build_pkt::pkt_test );

    // ASSERT
    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appeui(), network_controller_interface_dummy->get_appeui() + sizeof( Lora_params_conf_build_pkt::config_app_eui ) * 2 ),
                 ElementsAreArray( { '0', '0', '8', '0', '0', '0', '0', '0', '0', '0', '0', '0', 'E', '1', '9', 'C' } ) );

    EXPECT_THAT( std::vector<uint8_t>( network_controller_interface_dummy->get_appkey(), network_controller_interface_dummy->get_appkey() + sizeof( Lora_params_conf_build_pkt::config_app_key ) * 2 ),
                 ElementsAreArray( { '5', '3', '1', 'B', 'D', '9', 'C', '5', 'E', 'C', '5', 'D', '8', 'B', 'A', '5',
                                     'E', 'F', '3', 'B', '2', '6', '2', 'C', 'E', 'B', 'F', 'B', '3', 'E', '6', '6' } ) );
};
