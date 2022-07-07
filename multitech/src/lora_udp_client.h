#pragma once

#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <stdint.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include "base64.h"
#include "pkt.h"
#include "lossy.h"
#include "no_lossy.h"

/**
  \class Lora_udp_client
  \brief Clente udp para enviar pkt's via lora
  \author Jesus Palacios
  \date 08/01/2021
*/


class Lora_udp_client {

    public:

        /**
          \brief Constructor de la clase
        */
        Lora_udp_client();

        /**
          \brief Destructor de la clase
        */
        ~Lora_udp_client();

        /**
          \brief Inicializa el puerto de comunicacion
        */
        void init( uint16_t port_0 );

        /**
          \brief Modifica el tipo de formato de los datos
          \param sync_byte Byte que determina el formato a utilizar
          \return booleano True si el paramatreo de entrada es valido, false en caso contrario
        */
        bool set_formatter( uint8_t sync_byte );

        /**
          \brief Condificacion de la respuesta y envio de datos
          \param prefix_up Prefijo del paquete recibido
          \param pkt Datos a enviar
          \return Confirmacion del envio
        */
        bool send( char* prefix_up, Pkt& pkt );

    private:
        No_lossy no_lossy;
        Lossy lossy;
        uint16_t port;
        Base64 base64;
        const uint32_t offset_days = 20;
};