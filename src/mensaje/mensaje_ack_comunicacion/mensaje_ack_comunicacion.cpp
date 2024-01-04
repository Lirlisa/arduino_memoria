#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <cstdint>
#include <cstring>


Mensaje_ack_comunicacion::Mensaje_ack_comunicacion(
    uint16_t _emisor, uint16_t _receptor, uint16_t _nonce, uint16_t _nonce_msj_original
) : Mensaje(
    0, _emisor, _receptor, _nonce, Mensaje::PAYLOAD_ACK_COMUNICACION, (unsigned char*)nullptr, 0
) {
    nonce_mensaje_original = _nonce_msj_original;
    std::memcpy(payload, &nonce_mensaje_original, 2);
    std::memcpy(payload + 2, &receptor, 2); //emisor mensaje original
    std::memcpy(payload + 4, &emisor, 2); //receptor mensaje original
    payload_size = 6;
}

Mensaje_ack_comunicacion::Mensaje_ack_comunicacion(const Mensaje_ack_comunicacion& original) : Mensaje_ack_comunicacion(
    original.emisor, original.receptor, original.nonce, original.nonce_mensaje_original
) { }

/*
@brief Crea un mensaje ack comunicación a partir de un mensaje base. Es importante asegurarse que el payload sea el correcto
*/
Mensaje_ack_comunicacion::Mensaje_ack_comunicacion(Mensaje const& origen) : Mensaje(origen) {
    ttr = 0;
    tipo_payload = Mensaje::PAYLOAD_ACK_COMUNICACION;
}

bool Mensaje_ack_comunicacion::confirmar_ack(uint16_t nonce_original, uint16_t emisor_original, uint16_t receptor_original) {
    uint16_t aux_nonce, aux_emisor, aux_receptor;
    memcpy(&aux_nonce, payload, 2);
    memcpy(&aux_emisor, payload + 2, 2);
    memcpy(&aux_receptor, payload + 4, 2);
    return aux_nonce == nonce_original && aux_emisor == emisor_original && aux_receptor == receptor_original;
}