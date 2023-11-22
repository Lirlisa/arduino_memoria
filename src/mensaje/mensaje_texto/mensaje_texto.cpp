#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <cstdint>
#include <Arduino.h>

Mensaje_texto::Mensaje_texto() {
    return;
}

Mensaje_texto::Mensaje_texto(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
    uint16_t _creador, uint16_t _destinatario, uint16_t _nonce,
    uint8_t _tipo_payload, uint8_t _modo_transmision, unsigned char* _payload, int payload_size)
    : Mensaje(_ttr, _emisor, _receptor, _creador, _destinatario, _nonce, _tipo_payload, _modo_transmision, _payload, payload_size) {

    text_size = (uint8_t)payload[0];
    contenido = (char*)payload + 1;
}


Mensaje_texto::Mensaje_texto(Mensaje const& origen) : Mensaje(origen) {
    text_size = (uint8_t)payload[0];
    contenido = (char*)payload + 1;
}

void Mensaje_texto::print() {
    Mensaje::print();
    Serial.print("Tamaño del texto: ");
    Serial.println(text_size);
    Serial.print("Contenido: ");
    Serial.println(contenido);
}