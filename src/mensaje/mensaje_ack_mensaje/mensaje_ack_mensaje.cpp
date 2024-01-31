#include <mensaje/ack.hpp>
#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_ack_mensaje/mensaje_ack_mensaje.hpp>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <Arduino.h>

Mensaje_ack_mensaje::Mensaje_ack_mensaje(
    uint16_t _emisor, uint16_t _receptor, uint16_t _nonce,
    unsigned char* _payload, int _payload_size
) : Mensaje(0, _emisor, _receptor, _nonce, Mensaje::PAYLOAD_ACK_MENSAJE, _payload, _payload_size) { }

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
    uint16_t aux_nonce, aux_creador, aux_destinatario;
    unsigned acks_restantes = acks.size(), total_acks_enviados = 0, acks_por_enviar;
    unsigned char _payload[Mensaje_ack_mensaje::payload_max_size];
    uint64_t aux_ack = 0;
    std::vector<Mensaje_ack_mensaje> mensajes;
    while (acks_restantes > 0) {
        acks_por_enviar = (unsigned)std::min((unsigned)(payload_max_size - 1) / ack_size, acks_restantes);
        _payload[0] = acks_por_enviar;
        for (unsigned i = 0; i < acks_por_enviar; i++) {
            aux_ack = acks[total_acks_enviados + i];
            aux_nonce = aux_ack >> 48;
            aux_creador = aux_ack >> 32;
            aux_destinatario = aux_ack >> 16;
            std::memcpy(_payload + 1 + i * 6, &aux_nonce, 2);
            std::memcpy(_payload + 1 + i * 6 + 2, &aux_creador, 2);
            std::memcpy(_payload + 1 + i * 6 + 4, &aux_destinatario, 2);
        }
        acks_restantes -= acks_por_enviar;
        total_acks_enviados += acks_por_enviar;
        mensajes.push_back(Mensaje_ack_mensaje(_emisor, _receptor, _nonce, _payload, 1 + total_acks_enviados * ack_size));
    }
    return mensajes;
}

std::vector<uint64_t> Mensaje_ack_mensaje::obtener_acks() {
    std::vector<uint64_t> acks;
    uint64_t ack;
    uint16_t _nonce;
    uint16_t _creador;
    uint16_t _destinatario;
    uint8_t cantidad_acks = payload[0];
    char repr[5];
    repr[4] = '\0';
    Serial.print("Cantidad acks: ");
    Serial.println(cantidad_acks);
    Serial.println("----- Mensaje_ack_mensaje en obtener_acks -----");
    print();
    Serial.println("----- Fin Mensaje_ack_mensaje en obtener_acks -----");
    for (uint8_t i = 0; i < cantidad_acks; i++) {
        std::memcpy(&_nonce, payload + 1 + i * ack_size, 2);
        std::memcpy(&_creador, payload + 1 + i * ack_size + 2, 2);
        std::memcpy(&_destinatario, payload + 1 + i * ack_size + 4, 2);
        Serial.print("Ack: ");
        sprintf(repr, "%02x", _nonce);
        Serial.print(repr);
        sprintf(repr, "%02x", _creador);
        Serial.print(repr);
        sprintf(repr, "%02x", _destinatario);
        Serial.print(repr);
        Serial.println();

        ack = 0;
        ack |= ((uint64_t)_nonce) << 48;
        ack |= ((uint64_t)_creador) << 32;
        ack |= ((uint64_t)_destinatario) << 16;
        acks.push_back(ack);
    }
    return acks;
}