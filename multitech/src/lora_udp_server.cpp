#include "lora_udp_server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h> // inet_ntop()

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h> // sigint
#include "log.h"

static void die( const char* msg ) {
    printf( "%s", msg );
    fflush( stdout );
    fflush( stderr );
    exit( EXIT_FAILURE );
}

Lora_udp_server::Lora_udp_server( Fifo_pkt& fifo_lora_input_0, uint16_t max_pkt_size_0 ) : fifo_lora_input( fifo_lora_input_0 ), max_pkt_size( max_pkt_size_0 ) {}

Lora_udp_server::~Lora_udp_server() {
    pthread_mutex_destroy( &fifo_lock );
}

int8_t Lora_udp_server::init( uint16_t up_port_0, uint16_t down_port_0 ) {
    // Inicio la estructura hints
    struct addrinfo hints;
    (void)memset( &hints, '\0', sizeof( struct addrinfo ) );
    hints.ai_family   = AF_INET;                     // ipv4
    hints.ai_socktype = SOCK_DGRAM;                  // Datagrama
    hints.ai_protocol = 0;                           // Cualquier protocolo
    hints.ai_flags    = AI_PASSIVE | AI_NUMERICSERV; // Recibo de cualquier address

    addrinfo* result; // Resultado de getaddrinfo
    addrinfo* rp;     // Iterador por los addresses devueltos

    down_port = down_port_0;
    char server_port[5];
    snprintf( server_port, sizeof( server_port ), "%d", up_port_0 );
    // Cojo los addresses disponibles
    // INFO: http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    if ( 0 != ( getaddrinfo( NULL, server_port, &hints, &result ) ) ) {
        die( "getaddrinfo()\n" );
    }

    // Recorro los disponibles, hasta que consigo un bind
    for ( rp = result; rp != NULL; rp = rp->ai_next ) {
        listen_sd = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol );

        if ( listen_sd == -1 ) {
            continue;
        }

        int option_value = 1;
        if ( setsockopt( listen_sd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof( int ) ) == -1 ) {
            perror( "Setsockopt" );
            exit( 1 );
        }

        if ( bind( listen_sd, rp->ai_addr, rp->ai_addrlen ) == 0 ) {
            break; // Success
        }

        close( listen_sd );
    }

    // Si no he conseguido ninguno, salgo
    if ( rp == NULL ) {
        size_t err_len = snprintf( NULL, 0, "Could not bind on port %s\n", server_port );
        char* err_str  = (char*)malloc( err_len + 1 );
        snprintf( err_str, err_len + 1, "Could not bind on port %s\n", server_port );
        die( err_str );
        free( err_str );
    }

    freeaddrinfo( result ); // Libero resultados, ya no se necesitan

    // Fija la condicion de nonblock al socket
    // NOTA: se podria poner async y usar signals
    // INFO: http://man7.org/linux/man-pages/man2/fcntl.2.html
    if ( -1 == ( fcntl( listen_sd, F_SETFD, O_NONBLOCK ) ) ) {
        die( "fcntl()" );
    }

    if ( pthread_mutex_init( &fifo_lock, NULL ) != 0 ) {
        printf( "Mutex init failed\n" );
        return 0;
    }

    if ( pthread_create( &lora_server_thread, NULL, thread_fcn, (void*)this ) ) {
        printf( "Error creating thread\n" );
        return 0;
    }

    log( (uint32_t)0, "LoRa server thread created\n" );
    return 1;
}

void Lora_udp_server::run( void ) {
    uint8_t buffer[max_len];
    Pkt pkt( max_pkt_size );
    Lora_data lora_data;

    struct sockaddr_in cliaddr;
    socklen_t socket_len;
    socket_len = sizeof( cliaddr );

    lora_udp_client.init( down_port );

    while ( 1 ) {
        int n = recvfrom( listen_sd, buffer, max_len, MSG_WAITALL, (struct sockaddr*)&cliaddr, &socket_len );

        buffer[n] = '\0';
        memset( lora_data.prefix, 0, prefix_len + 1 );
        memset( lora_data.data, 0, max_len );
        lora_data.len = 0;

        if ( frame_parser( (char*)buffer, lora_data ) ) {
            for ( uint_fast16_t i = 0; i < lora_data.len; i++ ) {
                if ( pkt.parse( lora_data.data[i] ) ) {
                    pthread_mutex_lock( &fifo_lock );
                    bool pkt_intput_is_saved = fifo_lora_input.put_pkt( pkt );
                    pthread_mutex_unlock( &fifo_lock );
                    if ( pkt_intput_is_saved ) {
                        lora_udp_client.send( lora_data.prefix, pkt );
                    }
                }
            }
        }
    }
}

bool Lora_udp_server::frame_parser( char* buffer, Lora_data& lora_data ) {
    char* pch;
    const uint16_t encoded_data_len = 500;
    char encoded_data[encoded_data_len];

    // |------------prefix-----------| |----------------------------data in Base64-------------------------| lora/00-00-00-00-02-65-e9-e8/up
    // {"tmst":3734965956,"time":"2021-01-05T20:42:44.962524Z","tmms":1293914582962,"chan":2,"rfch":0,"freq":868.5,"stat":1,"modu":"LORA","datr":"SF9BW125","codr":"4/5","lsnr":10.0,"rssi":-47,"opts":"","size":51,"fcnt":18,"cls":0,"port":2,"mhdr":"801dce3001801200","data":"LOjpZQJ7AAAAgggEz/RfxSsAAAAAAAAAaYukIWAYKUNMXFDe3m/BVp8cAkAgAAJAgIAZ","appeui":"00-80-00-00-00-00-e1-9c","deveui":"00-00-00-00-02-65-e9-e8","ack":false,"adr":true,"gweui":"00-80-00-00-a0-00-69-3f","seqn":18}
    pch = strtok( buffer, " " );
    if ( pch == nullptr ) {
        return false;
    }

    if ( strlen( pch ) != prefix_len || strstr( pch, "lora/" ) == NULL || strstr( pch, "/up" ) == NULL ) {
        return false;
    }

    strncpy( lora_data.prefix, pch, strlen( pch ) );

    pch = strtok( NULL, "\0" );
    if ( pch == NULL ) {
        return false;
    }

    char* sub_pch = strstr( pch, "\"data\":\"" );
    pch           = strtok( sub_pch, ":" );
    if ( pch != nullptr ) {
        pch = strtok( NULL, "\"" );
        if ( pch != nullptr && strlen( pch ) < encoded_data_len ) {
            strcpy( encoded_data, pch );
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }

    size_t decoded_data_len = base64.decoded_size( encoded_data );
    uint8_t decoded_data[decoded_data_len];
    if ( !base64.decode( encoded_data, (unsigned char*)decoded_data, decoded_data_len ) ) {
        return false;
    }
    lora_data.len = decoded_data_len;
    memcpy( lora_data.data, decoded_data, decoded_data_len );

    return true;
}

void* Lora_udp_server::thread_fcn( void* Lora_udp_server_void_ptr ) {
    ( (Lora_udp_server*)Lora_udp_server_void_ptr )->run();
    return NULL;
}
