#include <cstdint>
#include <cstring>
#include <Arduino.h>
#include <string>
#include <mensaje/mensaje/mensaje.hpp>


Mensaje::Mensaje() {}

Mensaje::Mensaje(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
    uint16_t _creador, uint16_t _destinatario, uint16_t _nonce,
    uint8_t _tipo_payload, uint8_t _modo_transmision, unsigned char* _payload, int payload_size
) {
    ttr = _ttr;
    emisor = _emisor;
    receptor = _receptor;
    creador = _creador;
    destinatario = _destinatario;
    nonce = _nonce;
    tipo_payload = _tipo_payload;
    modo_transmision = _modo_transmision;
    memcpy(payload, _payload, payload_size);
}

void Mensaje::print() {
    Serial.print("Emisor: ");
    Serial.println(emisor);
    Serial.print("Receptor: ");
    Serial.println(receptor);
    Serial.print("Creador: ");
    Serial.println(creador);
    Serial.print("Destinatario: ");
    Serial.println(destinatario);
    Serial.print("Nonce: ");
    Serial.println(nonce);
    Serial.print("Modo Transmisión: ");
    Serial.println(modo_transmision);
    Serial.print("TTR: ");
    Serial.println(ttr);
    Serial.print("Tipo payload: ");
    Serial.println(tipo_payload);
}

/*
Crea mensaje para transmitir a partir del mensaje, es deber de caller liberar la memoria.
*/
unsigned char* Mensaje::parse_to_transmission() {
    unsigned char* msg = new unsigned char[raw_message_size];

    memcpy(msg, &emisor, 2);
    memcpy(msg + 2, &receptor, 2);
    memcpy(msg + 4, &creador, 2);
    memcpy(msg + 6, &destinatario, 2);
    memcpy(msg + 8, &nonce, 2);

    msg[10] = modo_transmision << 7;

    msg[10] |= (uint8_t)(ttr & 0x7F);
    msg[11] = (uint8_t)((ttr >> 7) & 0xFF);
    msg[12] = (uint8_t)(((ttr >> 15) & 0x3) << 6);

    msg[12] |= tipo_payload << 3;

    msg[12] |= (payload[0] & 0x7);

    int inicio_msg = 13;
    for (int i = 0; i < 100; i++) {
        msg[inicio_msg + i] = payload[i] & 0xf8;
        msg[inicio_msg + i] |= payload[i + 1] & 0x7;
    }
    msg[113] = payload[100] & 0xf8;

    return msg;
}

/*
Crea un mensaje desde lo recibido por una transmisión, es deber de caller liberar la memoria.
*/
Mensaje* Mensaje::parse_from_transmission(const unsigned char* data) {
    uint32_t _ttr;
    int16_t _emisor, _receptor, _creador, _destinatario, _nonce;
    int8_t _tipo_payload, _modo_transmision;
    unsigned char _payload[101];

    std::memcpy(&_emisor, data, 2);
    std::memcpy(&_receptor, data + 2, 2);
    std::memcpy(&_creador, data + 4, 2);
    std::memcpy(&_destinatario, data + 6, 2);
    std::memcpy(&_nonce, data + 8, 2);

    _modo_transmision = (data[10] >> 7);

    _ttr = (data[10] & 0x7f);
    _ttr |= ((uint32_t)data[11] & 0xff) << 7;
    _ttr |= ((uint32_t)data[12] & 0xc0) << 9;

    _tipo_payload = (data[12] & 0x38) >> 3;

    _payload[0] = (data[12] & 0x7);
    int inicio_msg = 13;
    for (int i = 0; i < 100; i++) {
        _payload[i] |= data[inicio_msg + i] & 0xf8;
        _payload[i + 1] = data[inicio_msg + i] & 0x7;
    }
    _payload[100] = data[113] & 0xf8;

    return new Mensaje(_ttr, _emisor, _receptor, _creador, _destinatario, _nonce, _tipo_payload, _modo_transmision, _payload, 101);
}

void Mensaje::setEmisor(uint16_t _emisor) {
    emisor = _emisor;
}
void Mensaje::setReceptor(uint16_t _receptor) {
    receptor = _receptor;
}

uint16_t Mensaje::getDestinatario() {
    return destinatario;
}
uint16_t Mensaje::getNonce() {
    return nonce;
}