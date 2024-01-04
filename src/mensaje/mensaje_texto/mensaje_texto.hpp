#ifndef MENSAJE_TEXTO_HPP
#define MENSAJE_TEXTO_HPP

#include <texto/texto.hpp>
#include <mensaje/mensaje/mensaje.hpp>
#include <cstdint>
#include <cstring>
#include <vector>
#include <Arduino.h>


class Mensaje_texto : public Mensaje {
private:
    Texto* textos;
    uint8_t cantidad_textos;
public:
    Mensaje_texto(
        uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, unsigned char* _payload,
        int payload_size
    );
    Mensaje_texto(Mensaje const& origen);
    Mensaje_texto(const Mensaje_texto& origen);
    Mensaje_texto();
    ~Mensaje_texto();



    static std::vector<Mensaje_texto> crear_mensajes(
        uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, std::vector<Texto>& textos
    );

    void print();

    std::vector<Texto> obtener_textos();

    uint8_t get_cantidad_textos();
};

#endif