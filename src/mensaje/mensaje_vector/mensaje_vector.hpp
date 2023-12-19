#ifndef MENSAJE_VECTOR_HPP
#define MENSAJE_VECTOR_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <cstdint>

class Mensaje_vector : public Mensaje {
private:
    /* data */
public:
    Mensaje_vector(
        uint16_t emisor_creador, uint16_t receptor_destinatario,
        uint16_t _nonce, unsigned char* _payload, int payload_size
    );

};

#endif