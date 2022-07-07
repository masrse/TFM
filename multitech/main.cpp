#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "log.h"
#include "lora_udp_server.h"
#include "fifo_pkt.h"
#include "orbcommST2100_controller.h"
#include "pkt_filter.h"
#include "comm_mgr.h"
#include "model_location.h"
#include "commands_ids.h"
#include "imei_list.h"

#include "pkt.h"
#include "lossy.h"
#include "payload_mgr.h"

constexpr uint16_t max_elements = 100;
constexpr uint16_t max_pkt_size = 200;
Lossy lossy;
Data_formatter_interface* Pkt_byte_sync::data_formatter_interface = &lossy;
Data_formatter_interface* Payload_formatter::data_formatter_impl  = &lossy;

Fifo_pkt fifo_lora_input( max_elements, max_pkt_size );
Fifo_pkt fifo_cloud_output( max_elements, max_pkt_size );
Fifo_pkt fifo_local_output( max_elements, max_pkt_size );

char url_cloud[100] = { "https://api.witrac.es/api/gateway_lora" };
char url_local[100] = { "http://192.168.2.100:8080/api/sensors/gateway/lora" };

Lora_udp_server lora_udp_server( fifo_lora_input, max_pkt_size );
Comm_mgr comm_cloud( max_pkt_size, url_cloud );
Comm_mgr comm_local( max_pkt_size, url_local );
OrbcommST2100_controller controller;

Imei_list imei_list;
constexpr uint32_t send_imei_time_s_max = 86400; // 24H

constexpr uint8_t priority    = 1;                // param priority prioridad del mensaje
constexpr uint8_t data_format = 2;                // formato del mensaje: ASCII-Hex
constexpr uint8_t sin         = 128;              // identificador del mensaje de datos
uint32_t gateway_imei         = 0;                // imei del gateway
char msg_name[8]              = "0";              // nombre del mensaje
char mobile_id[16];                               // id del orbcom
float latitud                             = 0;    // latitud obtenida del orbcom
float longitud                            = 0;    // longitud obtenida del orbcom
constexpr uint32_t send_position_time_max = 3600; // tiempo maximo hasta volver a enviar la posicion
uint32_t send_position_time               = 0;    // tiempo desde la ultima vez que se envio la posicion

void sleep_seconds( uint32_t seconds ) {
    uint32_t now = time( 0 );
    while ( time( 0 ) < now + seconds ) {};
}

static void lora_receive( void ) {
    if ( fifo_lora_input.available() > 0 ) {
        Pkt pkt( max_pkt_size );
        fifo_lora_input.get_pkt( pkt );
        log( pkt.hdr->src, "Frame->" );
        for ( uint_fast16_t i = 0; i < pkt.get_size(); i++ ) {
            printf( " %d", pkt[i] );
        }
        printf( "\n" );
        fifo_cloud_output.put_pkt( pkt );
        fifo_local_output.put_pkt( pkt );
    }
}

static void send_cloud_by_satellite( Pkt& pkt ) {
    uint8_t msg_status = 0;
    Wtc_err_t result   = controller.send_status( msg_name, msg_status );

    if ( result == wtc_err_command ) {
        Pkt_filter pkt_filter( pkt );
        pkt_filter.filter_pkt( model_id_mp_4000, model_id_location );
        controller.send( msg_name, priority, sin, data_format, pkt_filter.pkt_filtered, pkt_filter.pkt_filtered.get_size() );
    }
    else if ( result == wtc_success && msg_status == OrbcommST2100_msg_status::completed ) {
        fifo_cloud_output.get_pkt( pkt );
        imei_list.update_imei_timestamp( pkt.hdr->src, time( 0 ) );
        log( pkt.hdr->src, "Pkt successfully sent to cloud API by satellite\n" );
    }
    else {
        // El mensaje no se ha enviado
    }
}

static void send_cloud( void ) {
    if ( fifo_cloud_output.available() > 0 ) {
        Pkt pkt( max_pkt_size );
        fifo_cloud_output.copy_pkt( pkt );

        if ( comm_cloud.send( pkt, mobile_id ) ) {
            log( pkt.hdr->src, "Pkt successfully sent to cloud API by cellular\n" );
            fifo_cloud_output.get_pkt( pkt );
        }
        else {
            log( pkt.hdr->src, "Error sending pkt to cloud API by cellular\n" );
            if ( imei_list.check_send_pkt_by_satellite( pkt.hdr->src, time( 0 ), send_imei_time_s_max ) ) {
                log( pkt.hdr->src, "Sending pkt by satellite \n" );
                send_cloud_by_satellite( pkt );
            }
            else {
                fifo_cloud_output.get_pkt( pkt );
            }
        }
    }
}

static void send_local( void ) {
    if ( fifo_local_output.available() > 0 ) {
        Pkt pkt( max_pkt_size );
        fifo_local_output.copy_pkt( pkt );

        if ( comm_local.send( pkt, mobile_id ) ) {
            log( pkt.hdr->src, "Pkt successfully sent to local API\n" );
            fifo_local_output.get_pkt( pkt );
        }
        else {
            log( pkt.hdr->src, "Error sending pkt to local API\n" );
        }
    }
}

static bool get_position( void ) {
    if ( ( controller.get_latitud( latitud ) != wtc_success ) || ( controller.get_longitud( longitud ) != wtc_success ) ) {
        return false;
    }
    if ( ( latitud == 0.0 ) || ( longitud == 0.0 ) || ( latitud == longitud ) ) {
        return false;
    }
    return true;
}

static void send_position( void ) {
    if ( time( 0 ) > ( send_position_time + send_position_time_max ) ) {
        if ( get_position() ) {
            log( (uint32_t)0, "Position -> latitud: %f; longitud: %f\n", latitud, longitud );
            send_position_time = time( 0 );
            // Creamos el modelo
            Location location;
            location.latitud   = latitud;
            location.longitud  = longitud;
            location.timestamp = send_position_time;
            // Creamos el pkt con lossy
            Pkt pkt_position( max_pkt_size );
            Payload_mgr payload_mgr( max_pkt_size );
            payload_mgr.reset();
            location.to_pkt_payload( &payload_mgr );

            pkt_position.hdr->len = payload_mgr.get_used_size();
            pkt_position.build( gateway_imei, 123, cmd_sensor_data, send_position_time, payload_mgr.get_bytes(), payload_mgr.get_used_size() );

            fifo_cloud_output.put_pkt( pkt_position );
        }
        else {
            log( (uint32_t)0, "Error getting position\n" );
        }
    }
}

main( void ) {
    int ap_state = system( "mts-io-sysfs store ap2/serial-mode rs232" );
    log( (uint32_t)0, "Configurando AP2->%i\n", ap_state );
    log( (uint32_t)0, "Start\n" );

    // Obtenemos el imei del gateway
    system( "mts-io-sysfs show imei > imei.txt" );
    FILE* imei_file = fopen( "imei.txt", "rb" );
    if ( !imei_file ) {
        log( (uint32_t)0, "Error imei Gateway\n" );
    }
    else {
        char imei[9] = "0";
        fseek( imei_file, sizeof( imei ) - 1, SEEK_SET );
        fread( imei, sizeof( char ), sizeof( imei ) - 1, imei_file );
        gateway_imei = atoi( imei );
    }
    fclose( imei_file );

    sleep_seconds( 5 );

    if ( !lora_udp_server.init( 1784, 1786 ) ) {
        exit( EXIT_FAILURE );
    }

    if ( controller.init() != wtc_success ) {
        exit( EXIT_FAILURE );
    }

    if ( controller.echo( 0 ) != wtc_success ) {
        log( (uint32_t)0, "Error Orbcom init\n" );
        exit( EXIT_FAILURE );
    }
    log( (uint32_t)0, "Orbcom OK\n" );

    memset( mobile_id, 0, sizeof( mobile_id ) );
    controller.get_mobile_id( mobile_id );

    while ( 1 ) {
        send_position();
        lora_receive();
        send_cloud();
        send_local();
        sleep_seconds( 10 );
    }
}