#pragma once

#include "stdint.h"   // Tipos uint8_t etc
#include <netdb.h>    // addrinfo
#include <sys/poll.h> // poll
#include <list>
#include "pthread.h"
#include "fifo_pkt.h"

class Lora_tcp_server {

    public:

        /**
          \brief Constructor de la clase
          \param fifo_lora_input_0 Pila para almacenar pkt's
          \param max_pkt_size_0 Longitud maxima del pkt
        */
        Lora_tcp_server( Fifo_pkt& fifo_lora_input_0, uint16_t max_pkt_size_0 );

        /**
          \brief Destructor de la clase
        */
        ~Lora_tcp_server();

        /**
          \brief Inicializa los parámetros del servidor
          \param tcp_port Puerto listen del servidor
          \return Error de inicializacion
        */
        int8_t init( char* tcp_port );

        /**
          \brief Arranca el servidor
        */
        void run( void );

        /**
          \brief Funcion estatica callback del thread
          \param Lora_tcp_server_void_ptr puntero al objeto que crea el thread
        */
        static void* thread_fcn( void* Lora_tcp_server_void_ptr );


    private:

        Fifo_pkt& fifo_lora_input;
        pthread_t lora_server_thread;                     ///< Thread de recepcion y envio
        pthread_mutex_t fifo_lock;                        ///< Mutex para manipulacion de la fifo
        uint16_t max_pkt_size;
        int listen_sd;                                    ///< Descriptor socket abierto para listen
        struct pollfd fds[1];
        static const int timeout = 60000;                 ///< [ms]
        static const uint16_t max_time_no_comm = 60;

};


