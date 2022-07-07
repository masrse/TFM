#pragma once

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdint.h>
#include <sys/types.h> 

/**
  \class Base64
  \brief Clase para codificar y decodificar tramas en Base64
  \author Jesus Palacios
  \date 07/01/2021
*/


class Base64 {
    
    public:

        /**
          \brief Constructor de la clase
        */
        Base64();

        /**
          \brief Destructor de la clase
        */
        ~Base64();

        /**
          \brief Devuelve el tamanyo de los datos codificados
          \param inlen Tamanyo de los datos sin codificar
          \return Tamanyo de los datos codificados
        */
        size_t encoded_size( size_t inlen );

        /**
          \brief Condifica datos en Base64
          \param in Buffer de datos sin codificar
          \param out Buffer de datos codificados
          \param outlen Tamanyo de los datos codificados
          \return Resultado de la codificacion
        */
        bool encode( unsigned char* in, char* out, size_t outlen );

        /**
          \brief Devuelve el tamanyo de los datos codificados
          \param inlen Tamanyo de los datos codificados
          \return Tamanyo de los datos decodificados
        */
        size_t decoded_size( const char* in );

        /**
          \brief Comprueba si es un caracter valido para la codificacion/decodificacion
          \param c Caracter a comprobar
          \return Resultado de la comprobacion
        */
        bool isvalidchar( char c );

        /**
          \brief Decodifica datos en Base64
          \param in Buffer de datos codificados
          \param out Buffer de datos decodificados
          \param outlen Tamanyo de los datos decodificados
          \return Resultado de la decofificacion
        */
        bool decode( const char* in, unsigned char* out, size_t outlen );

    private:

        const char b64chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        int b64invs[80] = { 62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58,
                59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5,
                6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28,
                29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
                43, 44, 45, 46, 47, 48, 49, 50, 51 };
};