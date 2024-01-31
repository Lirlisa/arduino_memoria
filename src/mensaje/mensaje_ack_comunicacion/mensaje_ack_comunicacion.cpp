#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <cstdint>
#include <cstring>
#include <Arduino.h>


Mensaje_ack_comunicacion::Mensaje_ack_comunicacion(
    uint16_t _emisor, uint16_t _receptor, uint16_t _nonce, uint16_t _nonce_msj_original
) : Mensaje(
    0, _emisor, _receptor, _nonce, Mensaje::PAYLOAD_ACK_COMUNICACION, (unsigned char*)nullptr, 0
) {
    nonce_mensaje_original = _nonce_msj_original;
    payload_size = payload_max_size;
    transmission_size = message_without_payload_size + payload_size;
    payload = new unsigned char[payload_size];
    std::memcpy(payload, &nonce_mensaje_original, 2);
    std::memcpy(payload + 2, &receptor, 2); //emisor mensaje original
    std::memcpy(payload + 4, &emisor, 2); //receptor mensaje original
}

Mensaje_ack_comunicacion::Mensaje_ack_comunicacion(const Mensaje_ack_comunicacion& original) : Mensaje_ack_comunicacion(
    original.emisor, original.receptor, original.nonce, original.nonce_mensaje_original
) { }

/*
@brief Crea un mensaje ack comunicación a partir de un mensaje base. Es importante asegurarse que el payload sea el correcto
*/
Mensaje_ack_comunicacion::Mensaje_ack_comunicacion(Mensaje const& origen) : Mensaje(origen) {
    std::memcpy(&nonce_mensaje_original, payload, 2);
    ttr = 0;
    tipo_payload = Mensaje::PAYLOAD_ACK_COMUNICACION;
}

Mensaje_ack_comunicacion::~Mensaje_ack_comunicacion() {
}

bool Mensaje_ack_comunicacion::confirmar_ack(uint16_t nonce_original, uint16_t emisor_original, uint16_t receptor_original) const {
    uint16_t aux_emisor, aux_receptor;
    memcpy(&aux_emisor, payload + 2, 2);
    memcpy(&aux_receptor, payload + 4, 2);
    // Serial.println("----- Datos en confirmar_ack -----");
    // Serial.print("Emisor en payload: ");
    // Serial.println(aux_emisor);
    // Serial.print("Emisor entregado: ");
    // Serial.println(emisor_original);
    // Serial.print("Receptor en payload: ");
    // Serial.println(aux_receptor);
    // Serial.print("Receptor entregado: ");
    // Serial.println(receptor_original);
    // Serial.print("Nonce en payload: ");
    // Serial.println(nonce_mensaje_original);
    // Serial.print("Nonce entregado: ");
    // Serial.println(nonce_original);
    // Serial.print("Resultado: ");
    // Serial.println((nonce_mensaje_original == nonce_original &&
    //     aux_emisor == emisor_original &&
    //     (aux_receptor == receptor_original || receptor_original == BROADCAST_CHANNEL_ID || aux_receptor == BROADCAST_CHANNEL_ID)) ? "True" : "False");
    // Serial.println("----- Fin Datos en confirmar_ack -----");
    return nonce_mensaje_original == nonce_original &&
        aux_emisor == emisor_original &&
        (aux_receptor == receptor_original || receptor_original == BROADCAST_CHANNEL_ID || aux_receptor == BROADCAST_CHANNEL_ID);
}

void Mensaje_ack_comunicacion::print() {
    Mensaje::print();

    Serial.print("Emisor original: ");
    Serial.println(receptor);
    Serial.print("Nonce original: ");
    Serial.println(nonce_mensaje_original);
}