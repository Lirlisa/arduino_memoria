#ifndef MENSAJE_TEXTO_HPP
#define MENSAJE_TEXTO_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <cstdint>

class Mensaje_texto : public Mensaje {
private:
    uint8_t text_size;
    char* contenido;

public:
    Mensaje_texto();
    Mensaje_texto(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _creador, uint16_t _destinatario, uint16_t _nonce,
        uint8_t _tipo_payload, uint8_t _modo_transmision, unsigned char* _payload, int payload_size);
    Mensaje_texto(Mensaje const& origen);

    void print();
};

#endif