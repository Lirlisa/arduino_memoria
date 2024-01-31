#ifndef MENSAJE_VECTOR_HPP
#define MENSAJE_VECTOR_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <cstdint>
#include <utility>
#include <vector>

typedef std::pair<float, uint16_t> par_costo_id;

class Mensaje_vector : public Mensaje {
private:
    std::vector<par_costo_id> pares;

public:
    Mensaje_vector(
        uint16_t emisor, uint16_t receptor,
        uint16_t _nonce, unsigned char* _payload, int payload_size
    );
    const static unsigned par_costo_id_size = 6;
    const static unsigned max_cantidad_pares_por_payload = (payload_max_size - 1) / par_costo_id_size;

    Mensaje_vector(Mensaje const& origen);

    std::vector<par_costo_id> get_pares() const;

    void print(unsigned int cant_bytes = 6) const;
};

#endif