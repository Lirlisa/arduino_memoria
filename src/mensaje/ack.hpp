#ifndef ACK_HPP
#define ACK_HPP

#include <cstdint>
#include <string.h>

struct ACK {
    uint16_t nonce, creador, destinatario;
    /*
    @brief convierte los datos en un array de bytes, es deber del caller liberar la memoria.
    */
    unsigned char* parse_to_transmission() {
        unsigned char* data = new unsigned char[6];
        memcpy(data, &nonce, 2);
        memcpy(data + 2, &creador, 2);
        memcpy(data + 4, &destinatario, 2);
        return data;
    }
};

#endif