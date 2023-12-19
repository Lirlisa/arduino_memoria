#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <cstdint>
#include <Arduino.h>

Mensaje_vector::Mensaje_vector(
    uint16_t emisor, uint16_t receptor,
    uint16_t _nonce, unsigned char* _payload, int payload_size
) : Mensaje(
    0, emisor, receptor, _nonce, Mensaje::PAYLOAD_VECTOR, _payload, payload_size
) {

}