#pragma once

#include "stdint.h"
#include "stdlib.h"
#include "string.h"

/**
  \class Imei list
*/

#define IMEI_NOT_FOUND -1

typedef struct {
    uint32_t imei;
    uint32_t timestamp;
} Imei_list_t;

class Imei_list {
  public:
    /**
      \brief Constructor de la clase
    */
    Imei_list();

    /**
      \brief Destructor de la clase
    */
    ~Imei_list();

    /**
      \brief Añade el imei en la lista
      \param imei imei que queremos añadir
      \param timestamp timestamp en el que se ha añadido el imei, por defecto 0
      \return devuelve true si se ha podido añadir y false si no
    */
    bool add_imei( uint32_t new_imei, uint32_t timestamp = 0 );

    /**
      \brief Busca si el imei está en la lista
      \param imei imei que buscamos
      \return devuelve la posición en la lista imei, y si no lo encuentra devuelve IMEI_NOT_FOUND( -1 )
    */
    int found_imei( uint32_t imei );

    /**
      \brief Actaualiza el timesatmp del imei si está en la lista, si no, lo añade
      \param imei imei que buscamos
      \param timestamp timestamp que actualizamos
    */
    void update_imei_timestamp( uint32_t imei, uint32_t timestamp );

    /**
      \brief Comprobar si el imei tiene que ser filtrado por tiempo para enviar el pkt por satelite
      \param imei imei que buscamos
      \param timestamp timestamp del imei
      \param time_filter_s el tiempo para saber si hay que filtrar el imei
      \return
    */
    bool check_send_pkt_by_satellite( uint32_t imei, uint32_t timestamp, uint32_t time_filter_s );

  private:
    static const uint8_t imei_list_max_len = UINT8_MAX - 1;
    Imei_list_t imei_list[imei_list_max_len];
    uint16_t imei_list_len;
};