#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <cstdint>
#include <Arduino.h>
#include <cstring>

Mensaje_vector::Mensaje_vector(
    uint16_t emisor, uint16_t receptor,
    uint16_t _nonce, unsigned char* _payload, int payload_size
) : Mensaje(
    0, emisor, receptor, _nonce, Mensaje::PAYLOAD_VECTOR, _payload, payload_size
) {
    uint8_t largo = payload[0];
    for (uint8_t i = 0; i > largo; i++) {
        uint16_t id_nodo;
        float peso;
        std::memcpy(&id_nodo, payload + 1 + i * 6, 2);
        std::memcpy(&peso, payload + 1 + i * 6 + 2, 4);
        pares.push_back(std::make_pair(peso, id_nodo));
    }
}

Mensaje_vector::Mensaje_vector(Mensaje const& origen) : Mensaje(origen) {
    uint8_t largo = payload[0];
    for (uint8_t i = 0; i > largo; i++) {
        uint16_t id_nodo;
        float peso;
        std::memcpy(&id_nodo, payload + 1 + i * 6, 2);
        std::memcpy(&peso, payload + 1 + i * 6 + 2, 4);
        pares.push_back(std::make_pair(peso, id_nodo));
    }
}

std::vector<par_costo_id> Mensaje_vector::get_pares() {
    return pares;
}
