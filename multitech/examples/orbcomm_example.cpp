#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"
#include "OrbcommST2100_controller.h"

main() {
    OrbcommST2100_controller controller;
    Wtc_err_t result = wtc_success;
    printf( "-----Inicialización módulo-----\n" );
    result =  controller.init();
    printf( "Result: %d\n", result) ;

    printf( "-----Prueba echo0-----\n" );
    result = controller.echo(0 );
    printf( "Result: %d\n", result);

    printf( "-----Prueba comando AT-----\n" );
    result = controller.at_check();
    printf( "Result: %d\n", result );

    printf( "-----Prueba comando conn_status-----\n" );
    printf( "Status active = 10\n" );
    printf( "Status Waiting for GNSS = 1 \n" );
    uint8_t status = 0;
    result = controller.conn_status( status );
    printf( "Status: %d\n", status );
    printf( "Result: %d\n", result );

    printf( "-----Prueba envio-----\n" );

    uint8_t priority = 1;
    uint8_t data_format = 2;
    uint8_t sin = 128;
    char msg_name[10] = "005";
    uint8_t msg[] = { 48, 21, 23, 23 };
    Pkt pkt( 50 );
    pkt.build( 2, 123, 1, 0, msg, sizeof( msg ) );
    uint16_t size_pkt = pkt.get_size();
    printf( "Pkt size: %d\n", size_pkt );
    printf( "Pkt a enviar:\n" );
    for ( uint8_t i = 0; i < size_pkt; i++ ) {
        printf( "%d-", pkt[i] );
    }
    printf( "\n" );

    result = controller.send( msg_name, priority, sin, data_format, pkt, size_pkt );
    printf( "Result: %d\n", result );
    uint8_t attempts = 0;
    uint8_t max_attempts = 3;
    printf( "-----Status envio-----\n" );
    do {
        result = controller.send_status( msg_name, status );
        printf( "Status: %d\n", status );
        printf( "Result: %d\n", result );
        attempts++;
    } while( attempts < max_attempts );

}