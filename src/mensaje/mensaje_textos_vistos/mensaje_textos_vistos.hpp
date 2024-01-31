#ifndef MENSAJE_TEXTOS_VISTOS
#define MENSAJE_TEXTOS_VISTOS

#include <mensaje/mensaje/mensaje.hpp>
#include <vector>

class Mensaje_textos_vistos : public Mensaje {
private:
    static const uint8_t hash_size = 6;
public:
    Mensaje_textos_vistos(
        uint16_t _emisor, uint16_t _receptor, uint16_t _nonce,
        unsigned char* _payload, int _payload_size
    );
    Mensaje_textos_vistos(const Mensaje_textos_vistos& origen);
    Mensaje_textos_vistos(Mensaje const& origen);

    ~Mensaje_textos_vistos();

    static std::vector<Mensaje_textos_vistos> crear_mensajes(
        uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, const std::vector<uint64_t>& hashes
    );

    std::vector<uint64_t> obtener_hashes();
};


#endif