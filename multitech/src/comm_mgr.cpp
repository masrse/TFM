#include "comm_mgr.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "log.h"


// Funcion para la recepción de infromación. GET
// see: https://curl.haxx.se/libcurl/c/postinmemory.html
static size_t receive_callback ( void *contents, size_t size, size_t nmemb, void* user_params ) {
    size_t realsize = size * nmemb;
    receive_params_t* p = (receive_params_t*) user_params;

    if ( p->size + realsize < p->max_size ) {
        // copy contents to buffer
        memcpy( &( p->payload[p->size] ), contents, realsize );

        // set new buffer size
        p->size += realsize;

        // ensure null termination
        p->payload[p->size] = 0;

        return realsize;
    }
    return -1;
}

Comm_mgr::Comm_mgr( uint16_t pkt_size, char* url_post_0 ):
    pkt( pkt_size ),
    receive_params{ lib_buffer, 0, lib_max_len } {
        memset( lib_buffer, 0, lib_max_len );
        strcat( (char*)url_post, url_post_0 );
}

Comm_mgr::~Comm_mgr( void ) {
}

bool Comm_mgr::send( Pkt& pkt_0, char* mobile_id ) {
    char send_data_buffer[data_max_len] = {""};

    create_post_data( send_data_buffer, pkt_0, mobile_id );

    static const uint16_t max_response_len = 5000;
    char response_buff[max_response_len];
    uint16_t response_len = 0;
    log( pkt_0.hdr->src, "URL: %s; Data: %s\n", url_post, send_data_buffer );
    return post( url_post, send_data_buffer, response_buff, response_len, max_response_len );
}

void Comm_mgr::create_post_data( char* send_data, Pkt& pkt_0, char* mobile_id ) {
    uint16_t write_data_pos = 0;

    // MobileID
    strcat( (char*)send_data, "MobileID=" );
    write_data_pos += strlen( "MobileID=" );
    strcat( (char*)send_data, mobile_id );
    write_data_pos += strlen( mobile_id );

    // Project
    strcat( (char*)send_data, "&project=boluda" );
    write_data_pos += strlen( "&project=boluda" );

    // Pkt
    char aux_buffer[5];
    strcat( (char*)send_data, "&pkt=" );
    write_data_pos += strlen( "&pkt=" );
    for ( uint_fast16_t i = 0; i < pkt_0.get_size(); i++ ) {
        sprintf( &aux_buffer[0], "%d,", pkt_0.bytes()[i] );
        strncpy( &send_data[write_data_pos], &aux_buffer[0], strlen( aux_buffer ) );
        write_data_pos += strlen( aux_buffer );
    }
    send_data[write_data_pos-1] = '\0';
}

bool Comm_mgr::post( const char* url, const char* send_buff, char* response_buff,
                        uint16_t& response_len, const uint16_t max_response_len ) {
    CURL* p_curl;
    CURLcode request_code;

    // buffer restart
    memset( lib_buffer, 0, lib_max_len );

    receive_params.size = 0;

    // create curl and check init
    p_curl = curl_easy_init();

    if ( !p_curl ) {
        return false;
    }

    struct curl_slist* p_headers = nullptr;
    p_headers = curl_slist_append( p_headers, "Accept: application/x-www-form-urlencoded" );
    p_headers = curl_slist_append( p_headers, "Content-Type: application/x-www-form-urlencoded" );

    curl_easy_setopt( p_curl, CURLOPT_SSL_VERIFYPEER, 0L );
    curl_easy_setopt( p_curl, CURLOPT_URL, url );
    curl_easy_setopt( p_curl, CURLOPT_WRITEFUNCTION, receive_callback );
    curl_easy_setopt( p_curl, CURLOPT_WRITEDATA, (void*) &receive_params );
    curl_easy_setopt( p_curl, CURLOPT_POST, 0 );
    curl_easy_setopt( p_curl, CURLOPT_POSTFIELDS, send_buff );
    curl_easy_setopt( p_curl, CURLOPT_HTTPHEADER, p_headers );
    curl_easy_setopt( p_curl, CURLOPT_TIMEOUT, 5L );

    // Perform the request, response will get the return code
    request_code = curl_easy_perform( p_curl );
    long response_code;
    curl_easy_getinfo( p_curl, CURLINFO_RESPONSE_CODE, &response_code );
    // always cleanup
    curl_easy_cleanup( p_curl );
    curl_slist_free_all( p_headers );

    if ( request_code == CURLE_OK ) {
        if ( response_code == response_code_ok || response_code == response_code_accepted || response_code == response_code_created ) {
            if ( receive_params.size + response_len <= max_response_len ) {
              memcpy( response_buff, receive_params.payload, receive_params.size );
              response_len += receive_params.size;
              return true;
            }
        }
        else {
            if ( receive_params.size + response_len <= max_response_len ) {
              memcpy( response_buff, receive_params.payload, receive_params.size );
              response_len += receive_params.size;
            }
        }
    }

    return false;
}