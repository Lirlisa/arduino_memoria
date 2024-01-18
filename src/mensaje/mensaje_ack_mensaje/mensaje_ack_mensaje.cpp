#include <mensaje/ack.hpp>
#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_ack_mensaje/mensaje_ack_mensaje.hpp>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <Arduino.h>

/*
Inicializa un mensaje_ack_mensaje. En caso de que la cantidad de hashes supere el máximo de bytes, sólo los primeros serán guardados.
*/
Mensaje_ack_mensaje::Mensaje_ack_mensaje(
    uint16_t _emisor, uint16_t _receptor, uint16_t _nonce,
    ACK* lista_hashes, unsigned int cantidad_hashes
) : Mensaje(
    0, _emisor, _receptor,
    _nonce, Mensaje::PAYLOAD_ACK_MENSAJE,
    (unsigned char*)nullptr, 0
) {
    uint8_t cantidad_real_hashes = std::min(cantidad_hashes, (unsigned)(Mensaje::payload_max_size - 1) / ack_size);
    payload_size = 1 + 6 * cantidad_real_hashes;
    payload = new unsigned char[payload_size];
    payload[0] = cantidad_real_hashes;
    for (uint8_t i = 0; i < cantidad_hashes; i++) {
        unsigned char hash[6];
        lista_hashes[i].parse_to_transmission(hash);
        std::memcpy(payload + 1 + i * 6, hash, 6);
    }
}

Mensaje_ack_mensaje::Mensaje_ack_mensaje(
    uint16_t _emisor, uint16_t _receptor, uint16_t _nonce,
    unsigned char* _payload, int _payload_size
) : Mensaje(0, _emisor, _receptor, _nonce, Mensaje::PAYLOAD_ACK_MENSAJE, _payload, _payload_size) {

}

Mensaje_ack_mensaje::Mensaje_ack_mensaje(const Mensaje_ack_mensaje& origen) : Mensaje_ack_mensaje(
    origen.emisor, origen.receptor, origen.nonce, origen.payload, origen.payload_size
) { }

/*
@brief Hay que asegurarse de que el payload tenga el formato correcto
*/
Mensaje_ack_mensaje::Mensaje_ack_mensaje(Mensaje const& origen) : Mensaje(origen) {
    ttr = 0;
    tipo_payload = Mensaje::PAYLOAD_ACK_MENSAJE;
}

Mensaje_ack_mensaje::~Mensaje_ack_mensaje() {
}

/*
@brief Entrega un vector con mensajes. En caso de que el vector tenga más de 1 mensaje el Nonce de los mensajes es inválido.
*/
std::vector<Mensaje_ack_mensaje> Mensaje_ack_mensaje::crear_mensajes(
    uint16_t _emisor, uint16_t _receptor,
    uint16_t _nonce, std::vector<uint64_t>& acks
) {
    unsigned acks_restantes = acks.size(), total_acks_enviados = 0, acks_por_enviar;
    unsigned char _payload[Mensaje_ack_mensaje::payload_max_size];
    uint64_t aux_ack = 0;
    std::vector<Mensaje_ack_mensaje> mensajes;
    while (acks_restantes > 0) {
        acks_por_enviar = (unsigned)std::min((unsigned)(payload_max_size - 1) / ack_size, acks_restantes);
        _payload[0] = acks_por_enviar;
        for (unsigned i = 0; i < acks_por_enviar; i++) {
            aux_ack = acks[total_acks_enviados + i] >> 16;
            _payload[1 + i * 6] = (uint8_t)(aux_ack & 0xff);
            _payload[1 + i * 6 + 1] = (uint8_t)((aux_ack >> 8) & 0xff);
            _payload[1 + i * 6 + 2] = (uint8_t)((aux_ack >> 16) & 0xff);
            _payload[1 + i * 6 + 3] = (uint8_t)((aux_ack >> 24) & 0xff);
            _payload[1 + i * 6 + 4] = (uint8_t)((aux_ack >> 32) & 0xff);
            _payload[1 + i * 6 + 5] = (uint8_t)((aux_ack >> 40) & 0xff);
        }
        acks_restantes -= acks_por_enviar;
        total_acks_enviados += acks_por_enviar;
        mensajes.push_back(Mensaje_ack_mensaje(_emisor, _receptor, _nonce, _payload, 1 + total_acks_enviados * ack_size));
    }
    return mensajes;
}

std::vector<uint64_t> Mensaje_ack_mensaje::obtener_acks() {
    std::vector<uint64_t> acks;
    uint8_t cantidad_acks = payload[0];
    for (uint8_t i = 0; i < cantidad_acks; i++) {
        uint64_t ack;
        std::memcpy(&ack, payload + 1 + i * ack_size, ack_size);
        acks.push_back(ack);
    }
    return acks;
}