#pragma once

#include "stdint.h"   // Tipos uint8_t etc
#include <netdb.h>    // addrinfo
#include <list>
#include "pthread.h"
#include "fifo_pkt.h"
#include "base64.h"
#include "lora_udp_client.h"

/**
  \class Lora_udp_server
  \brief Servidor udp para almacenar los pkt's en la FiFo
  \author Jesus Palacios
  \date 07/01/2021
*/

class Lora_udp_server {

    private:

        Fifo_pkt& fifo_lora_input;
        pthread_t lora_server_thread;   ///< Thread de recepcion y envio
        pthread_mutex_t fifo_lock;      ///< Mutex para manipulacion de la fifo
        uint16_t max_pkt_size;
        int listen_sd;                  ///< Descriptor socket abierto para listen
        Base64 base64;                  ///< Codificador/decodificador base64

        static const uint16_t  max_len = 1024;
        static const uint8_t prefix_len = 31;

        typedef struct {
            uint16_t len;
            uint8_t data[max_len];
            char prefix[prefix_len + 1];
        } Lora_data;

        Lora_udp_client lora_udp_client;
        uint16_t down_port;

    public:

        /**
          \brief Constructor de la clase
          \param fifo_lora_input_0 Pila para almacenar pkt's
          \param max_pkt_size_0 Longitud maxima del pkt
        */
        Lora_udp_server( Fifo_pkt& fifo_lora_input_0, uint16_t max_pkt_size_0 );

        /**
          \brief Destructor de la clase
        */
        ~Lora_udp_server();

        /**
          \brief Inicializa los parámetros del servidor
          \param up_port_0 Puerto listen del servidor
          \param down_port_0 Puerto send del cliente
          \return Error de inicializacion
        */
        int8_t init( uint16_t up_port_0, uint16_t down_port_0 );

        /**
          \brief Arranca el servidor
        */
        void run( void );

        /**
          \brief Detecta si es una trama de datos valida
          \param buffer Buffer de entrada con los datos en bruto
          \param lora_data Estructura con los datos de salida
          \return Deteccion correcta de la trama
        */
        bool frame_parser( char* buffer, Lora_data& lora_data );

        /**
          \brief Funcion estatica callback del thread
          \param Lora_udp_server_void_ptr puntero al objeto que crea el thread
        */
        static void* thread_fcn( void* Lora_udp_server_void_ptr );



};


