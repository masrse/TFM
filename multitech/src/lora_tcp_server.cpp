#include "lora_tcp_server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>  // inet_ntop()

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>     // sigint
#include "log.h"

static void die( const char* msg ) {
    printf( "%s", msg );
    fflush( stdout );
    fflush( stderr );
    exit( EXIT_FAILURE );
}

Lora_tcp_server::Lora_tcp_server( Fifo_pkt& fifo_lora_input_0, uint16_t max_pkt_size_0 ):
    fifo_lora_input( fifo_lora_input_0 ),
    max_pkt_size( max_pkt_size_0 ) {
}

Lora_tcp_server::~Lora_tcp_server() {
    pthread_mutex_destroy( &fifo_lock );
    close( fds[0].fd );
}

int8_t Lora_tcp_server::init( char* tcp_port ) {

    // Inicio la estructura hints
    struct addrinfo hints;
    ( void )memset( &hints, '\0', sizeof( struct addrinfo ) );
    hints.ai_family = AF_INET;       // ipv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP; // TCP
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;     // Recibo de cualquier address

    addrinfo* result;                // Resultado de getaddrinfo
    addrinfo* rp;                    // Iterador por los addresses devueltos

    // Cojo los addresses disponibles
    // INFO: http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
    if ( 0 != ( getaddrinfo( NULL, tcp_port, &hints, &result ) ) ) {
        die( "getaddrinfo()\n" );
    }

    // Recorro los disponibles, hasta que consigo un bind
    for ( rp = result; rp != NULL; rp = rp->ai_next ) {

        listen_sd = socket( rp->ai_family, rp->ai_socktype,
                            rp->ai_protocol );

        if ( listen_sd == -1 ) {
            continue;
        }

        int option_value = 1;
        if ( setsockopt( listen_sd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof( int ) ) == -1 ) {
            perror( "Setsockopt" );
            exit( 1 );
        }

        if ( bind( listen_sd, rp->ai_addr, rp->ai_addrlen ) == 0 ) {
            break;                  // Success
        }

        close( listen_sd );
    }

    // Si no he conseguido ninguno, salgo
    if ( rp == NULL ) {
        size_t err_len = snprintf( NULL, 0, "Could not bind on port %s\n", tcp_port );
        char* err_str = (char*)malloc( err_len + 1 );
        snprintf( err_str, err_len + 1, "Could not bind on port %s\n", tcp_port );
        die( err_str );
        free( err_str );
    }

    freeaddrinfo( result );         // Libero resultados, ya no se necesitan

    // Habilita el socket binded para aceptar conexiones entrantes
    // INFO: http://man7.org/linux/man-pages/man2/listen.2.html
    if ( -1 == ( listen( listen_sd, 32 ) ) ) {
        die( "listen()" );
    }

    // Fija la condicion de nonblock al socket
    // NOTA: se podria poner async y usar signals
    // INFO: http://man7.org/linux/man-pages/man2/fcntl.2.html
    if ( -1 == ( fcntl( listen_sd, F_SETFD, O_NONBLOCK ) ) ) {
        die( "fcntl()" );
    }

    memset( fds, 0, sizeof( fds ) );
    fds[0].fd = listen_sd;
    fds[0].events = POLLIN;

    if ( pthread_mutex_init( &fifo_lock, NULL ) != 0 ) {
        printf( "Mutex init failed\n" );
        return 0;
    }

    if ( pthread_create( &lora_server_thread, NULL, thread_fcn, ( void* )this ) ) {
        printf( "Error creating thread\n" );
        return 0;
    }

    log( (uint32_t)0, "LoRa server thread created\n" );
    return 1;
}

void Lora_tcp_server::run( void ) {

    while( 1 ) {

        socklen_t addr_len;           // Longitud del address
        struct sockaddr_storage addr; // Estructura con info del address
        int news;                     // Descriptor nuevo socket

        constexpr int max_len = 1024;
        uint8_t data[max_pkt_size];
        Pkt pkt( max_pkt_size );

        addr_len = sizeof( addr );
        // printf( "Waiting for connection...\n", listen_sd );
        news = accept( listen_sd, ( struct sockaddr* )&addr, &addr_len );

        if ( -1 == news  ) {
            if ( EWOULDBLOCK != errno ) {
                die( "accept()" );
            }
        }
        else {
            // printf( "Open connection fd %d\n", listen_sd );
            struct pollfd fds_client;
            fds_client.fd = news; // your socket handler
            fds_client.events = POLLIN;
            struct timespec now;
            struct timespec last_recv;

            while ( 1 ) {

                // Polling
                int nready = poll( &fds_client, 1, timeout );

                if ( clock_gettime( CLOCK_REALTIME, &now ) == -1 ) {
                    // printf( "Error de reloj interno" );
                    break;
                }

                if ( nready < 0  ) {
                    // printf( "Error poll\n" );
                    break;
                }
                else if ( nready == 0 ) {
                }
                else {

                    if ( fds_client.revents & POLLIN ) {
                        fds_client.revents &= ~POLLIN;

                        int read = recv( fds_client.fd, data, max_len, 0 );

                        if ( !read ) {
                            // printf( "Cnnection closed\n" );
                            break;
                        }
                        if ( read < 0 ) {
                            // printf( "Client close connection\n" );
                            break;
                        }
                        if ( read > 0 ) {
                            for ( uint_fast16_t i = 0; i < read; i++ ) {
                                if ( pkt.parse( data[i] ) ) {
                                    pthread_mutex_lock( &fifo_lock );
                                    fifo_lora_input.put_pkt( pkt );
                                    pthread_mutex_unlock( &fifo_lock );
                                }
                            }
                            last_recv = now;
                        }
                    }

                    if ( fds_client.revents & POLLOUT ) {
                        fds_client.revents &= ~POLLOUT;
                        // printf( "pollout\n" );
                    }
                    if ( fds_client.revents ) {
                        // printf( "revents %u\n", fds[0].revents );
                        continue;                               // Evento, pero no de lectura
                    }
                }

                // Compruebo tiempo transcurrido desde ultima recepcion
                if ( now.tv_sec - last_recv.tv_sec > max_time_no_comm ) {
                    // printf( "Timeout, connection closed\n" );
                    close( news );
                    break;
                }
            }
        }

    }

}

void* Lora_tcp_server::thread_fcn( void* Lora_tcp_server_void_ptr ) {
    ( ( Lora_tcp_server* )Lora_tcp_server_void_ptr )->run();
    return NULL;
}
