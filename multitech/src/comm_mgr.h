#pragma once

#include "pkt.h"

/**
  \class Comm_mgr
  \brief Interfaz para parsear y enviar pkt a la api
  \author Javier Barahona <javier.barahona@witrac.es>
  \date 27/11/2020
*/

// estructura para receive_callback
typedef struct {
    char* const payload;
    size_t size; // bytes recibidos
    const size_t max_size;
} receive_params_t;

class Comm_mgr {

    public:
      /**
        \brief Constructor de la clase
        \param pkt_size Tamanyo maximo de pkt que se puede almacenar
        \param url_post_0 Url a utilizar
      */
      Comm_mgr( uint16_t pkt_size, char* url_post_0 );

      /**
        \brief Destructor de la clase
      */
      ~Comm_mgr( void );

      /**
        \brief Envia un pkt a la API con el formato correcto
        \param pkt_0 Pkt a enviar
        \param mobile_id id del módulo orbcomm
      */
      bool send( Pkt& pkt_0, char* mobile_id );

    private:

      /**
        \brief Genera los datos a enviar
        \param send_data Cadena de datos a enviar
        \param pkt_0 Pkt a enviar
        \param mobile_id id del módulo orbcomm
      */
      void create_post_data( char* send_data, Pkt& pkt_0, char* mobile_id );

      /**
        \brief Funcion post para enviar un string, guarda la respuesta del servidor
        \param url Array de char con la url
        \param send_buff Array de char a enviar
        \param response_buff Buffer para recibir la respuesta
        \param response_len Longitud del buffer de respuesta
        \param max_response_len Longitud máxima del buffer
        \return true si OK, CREATED o ACEPTED
      */
      bool post( const char* url, const char* send_buff, char* response_buff,
                        uint16_t& response_len, const uint16_t max_response_len );

      Pkt pkt;
      typedef enum : uint16_t {
        response_code_ok           = 200,
        response_code_created      = 201,
        response_code_accepted     = 202,
        response_code_no_content   = 204,
        response_code_unauthorized = 401
      } Response_t;

      static const uint16_t lib_max_len = 16384; // 1024*16
      char lib_buffer[lib_max_len];
      receive_params_t receive_params;
      static const uint16_t data_max_len = 500;
      static const uint16_t url_max_len = 500;
      char url_post[url_max_len] = {""};
};

