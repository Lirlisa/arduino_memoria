#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_textos_vistos/mensaje_textos_vistos.hpp>
#include <algorithm>
#include <cstring>

Mensaje_textos_vistos::Mensaje_textos_vistos(
    uint16_t _emisor, uint16_t _receptor, uint16_t _nonce,
    unsigned char* _payload, int _payload_size
) : Mensaje(0, _emisor, _receptor, _nonce, Mensaje::PAYLOAD_TEXTO_VISTO, _payload, _payload_size) { }

Mensaje_textos_vistos::Mensaje_textos_vistos(const Mensaje_textos_vistos& origen) : Mensaje_textos_vistos(
    origen.emisor, origen.receptor, origen.nonce, origen.payload, origen.payload_size
) { }

Mensaje_textos_vistos::Mensaje_textos_vistos(Mensaje const& origen) : Mensaje(origen) {
    tipo_payload = Mensaje::PAYLOAD_TEXTO_VISTO;
    ttr = 0;
}

Mensaje_textos_vistos::~Mensaje_textos_vistos() { }

/*
@brief Entrega un vector con todos los mensajes necesarios que contienen los hashes entregados. Si la cantidad de mensajes es mayor a 1, entonces el nonce de cada mensaje no es válido.
*/
std::vector<Mensaje_textos_vistos> Mensaje_textos_vistos::crear_mensajes(
    uint16_t _emisor, uint16_t _receptor,
    uint16_t _nonce, const std::vector<uint64_t>& hashes
) {
    uint16_t aux_nonce, aux_creador, aux_destinatario;
    unsigned hashes_restantes = hashes.size(), total_hashes_enviados = 0, hashes_por_enviar;
    unsigned char _payload[Mensaje_textos_vistos::payload_max_size];
    uint64_t aux_hash = 0;
    std::vector<Mensaje_textos_vistos> mensajes;
    while (hashes_restantes > 0) {
        hashes_por_enviar = (unsigned)std::min((unsigned)(Mensaje_textos_vistos::payload_max_size - 1) / hash_size, hashes_restantes);
        _payload[0] = hashes_por_enviar;
        for (unsigned i = 0; i < hashes_por_enviar; i++) {
            aux_hash = hashes[total_hashes_enviados + i];
            aux_nonce = aux_hash >> 48;
            aux_creador = aux_hash >> 32;
            aux_destinatario = aux_hash >> 16;
            std::memcpy(_payload + 1 + i * hash_size, &aux_nonce, 2);
            std::memcpy(_payload + 1 + i * hash_size + 2, &aux_creador, 2);
            std::memcpy(_payload + 1 + i * hash_size + 4, &aux_destinatario, 2);
        }
        hashes_restantes -= hashes_por_enviar;
        total_hashes_enviados += hashes_por_enviar;
        mensajes.push_back(Mensaje_textos_vistos(_emisor, _receptor, _nonce, _payload, 1 + total_hashes_enviados * hash_size));
    }
    return mensajes;
}

std::vector<uint64_t> Mensaje_textos_vistos::obtener_hashes() {
    std::vector<uint64_t> hashes;
    uint64_t hash;
    uint16_t _nonce;
    uint16_t _creador;
    uint16_t _destinatario;
    uint8_t cantidad_hashes = payload[0];
    for (uint8_t i = 0; i < cantidad_hashes; i++) {
        std::memcpy(&_nonce, payload + 1 + i * hash_size, 2);
        std::memcpy(&_creador, payload + 1 + i * hash_size + 2, 2);
        std::memcpy(&_destinatario, payload + 1 + i * hash_size + 4, 2);

        hash = 0;
        hash |= ((uint64_t)_nonce) << 48;
        hash |= ((uint64_t)_creador) << 32;
        hash |= ((uint64_t)_destinatario) << 16;
        hashes.push_back(hash);
    }
    return hashes;
}