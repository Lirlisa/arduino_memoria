#ifndef ACK_HPP
#define ACK_HPP

#include <cstdint>
#include <cstring>

struct ACK {
    uint16_t nonce, creador, destinatario;
    /*
    @brief convierte los datos en un array de bytes, el destino debe tener al menos 6 bytes disponibles.
    */
    void parse_to_transmission(unsigned char* destino) {
        std::memcpy(destino, &nonce, 2);
        std::memcpy(destino + 2, &creador, 2);
        std::memcpy(destino + 4, &destinatario, 2);
    }
};

#endif