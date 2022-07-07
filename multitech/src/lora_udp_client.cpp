#include "lora_udp_client.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "time.h"
#include "commands_ids.h"
#include "model_config_time.h"
#include "log.h"
#include "payload_mgr.h"

Lora_udp_client::Lora_udp_client() {}

Lora_udp_client::~Lora_udp_client() {}

void Lora_udp_client::init( uint16_t port_0 ) {
    port = port_0;
}

bool Lora_udp_client::set_formatter( uint8_t sync_byte ) {
    switch ( sync_byte ) {
        case 0x2C:
            Pkt_byte_sync::data_formatter_interface = &lossy;
            Payload_formatter::data_formatter_impl  = &lossy;
            break;
        case 0x2D:
            Pkt_byte_sync::data_formatter_interface = &no_lossy;
            Payload_formatter::data_formatter_impl  = &no_lossy;
            break;
        default:
            return false;
            break;
    }
    return true;
}

bool Lora_udp_client::send( char* prefix_up, Pkt& pkt_data ) {
    set_formatter( pkt_data.hdr->sync );
    Pkt pkt( pkt_data.get_size() );
    // Generamos el pkt de respuesta
    if ( pkt_data.hdr->cmd == cmd_sensor_data && ( abs( (int32_t)( pkt_data.hdr->timestamp - (uint32_t)time( NULL ) ) ) > ( offset_days * 24UL * 60UL * 60UL ) ) ) {
        Config_time config_time;
        Payload_mgr payload_mgr( Config_time::get_size() );
        config_time.time_from_server = (uint32_t)time( NULL );
        config_time.to_pkt_payload( &payload_mgr );
        pkt.build( pkt_data.hdr->dst, pkt_data.hdr->src, cmd_time_conf, pkt_data.hdr->timestamp, payload_mgr.get_bytes(), payload_mgr.get_used_size() );
    }
    else {
        pkt.build( pkt_data.hdr->dst, pkt_data.hdr->src, cmd_ack, pkt_data.hdr->timestamp, nullptr, 0 );
    }
    // Codificamos el pkt a Base64
    size_t coded_data_len = base64.encoded_size( pkt.get_size() );
    char out[coded_data_len];
    if ( !base64.encode( (unsigned char*)pkt.bytes(), out, pkt.get_size() ) ) {
        return false;
    }

    // Modificar prefix up a down
    uint8_t prefix_len = 34;
    char prefix_down[prefix_len];
    char* pch = strtok( prefix_up, "u" );
    if ( pch == nullptr ) {
        return false;
    }

    strcpy( prefix_down, pch );
    strcat( prefix_down, "down" );
    for ( uint_fast16_t i = 0; i < strlen( prefix_down ); i++ ) {
        if ( prefix_down[i] == '-' ) {
            prefix_down[i] = ':';
        }
    }

    // Creamos la cadena de salida con el prefijo y el json de datos codificados
    // lora/00:80:00:00:00:00:6a:1a/down {"data":"aGVsbG8gd29ybGQ="}
    char response[strlen( prefix_down ) + strlen( out ) + strlen( " {\"data\":\"\"}" )];
    strcpy( response, prefix_down );
    strcat( response, " {\"data\":\"" );
    strcat( response, out );
    strcat( response, "\"}" );

    // Envio de datos
    int sockfd;
    struct sockaddr_in servaddr;

    if ( ( sockfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
        log( (uint32_t)0, "Client socket creation failed" );
        return false;
    }

    log( pkt.hdr->dst, "Respuesta:%s\n", response );
    memset( &servaddr, 0, sizeof( servaddr ) );

    servaddr.sin_family      = AF_INET;
    servaddr.sin_port        = htons( port );
    servaddr.sin_addr.s_addr = INADDR_ANY;

    sendto( sockfd, (const char*)response, strlen( response ), MSG_CONFIRM, (const struct sockaddr*)&servaddr, sizeof( servaddr ) );

    close( sockfd );

    return true;
}