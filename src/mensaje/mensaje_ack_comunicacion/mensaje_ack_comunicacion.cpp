#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <cstdint>
#include <string.h>


Mensaje_ack_comunicacion::Mensaje_ack_comunicacion(
    uint16_t _emisor, uint16_t _receptor, uint16_t _nonce, uint16_t nonce_msj_original
) : Mensaje(
    0, _emisor, _receptor, _nonce, Mensaje::PAYLOAD_ACK_COMUNICACION, (unsigned char*)nullptr, 0
) {
    memcpy(payload, &nonce_msj_original, 2);
    memcpy(payload + 2, &receptor, 2); //emisor mensaje original
    memcpy(payload + 4, &emisor, 2); //receptor mensaje original
    payload_size = 6;
}

/*
@brief Crea un mensaje ack comunicación a partir de un mnensaje base. Es importante asegurarse que el payload sea el correcto
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