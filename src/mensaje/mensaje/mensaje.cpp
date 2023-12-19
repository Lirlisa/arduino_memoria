#include <cstdint>
#include <cstring>
#include <Arduino.h>
#include <mensaje/mensaje/mensaje.hpp>


Mensaje::Mensaje() {}

Mensaje::Mensaje(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
    uint16_t _nonce, uint8_t _tipo_payload,
    unsigned char* _payload, int _payload_size
) {
    ttr = _ttr;
    emisor = _emisor;
    receptor = _receptor;
    nonce = _nonce;
    tipo_payload = _tipo_payload;
    payload_size = _payload_size < Mensaje::payload_max_size ? _payload_size : Mensaje::payload_max_size;

    if (payload_size > 0) {
        payload = new unsigned char[payload_size];
        std::memcpy(payload, _payload, payload_size);
    }

    std::memset(payload + payload_size, 0, Mensaje::payload_max_size - payload_size);
    transmission_size = message_without_payload_size + payload_size;
}

Mensaje::~Mensaje() {
    delete[] payload;
}

void Mensaje::print() {
    Serial.print("Emisor: ");
    Serial.println(emisor);
    Serial.print("Receptor: ");
    Serial.println(receptor);
    Serial.print("Nonce: ");
    Serial.println(nonce);
    Serial.print("TTR: ");
    Serial.println(ttr);
    Serial.print("Tipo payload: ");
    Serial.println(tipo_payload);
}

/**
@brief Crea mensaje para transmitir a partir del mensaje, es deber de caller liberar la memoria.
*/
unsigned char* Mensaje::parse_to_transmission() {
    unsigned char* msg = new unsigned char[payload_size];

    memcpy(msg, &emisor, 2); // 0, 1
    memcpy(msg + 2, &receptor, 2); // 2, 3
    memcpy(msg + 4, &nonce, 2); // 4, 5

    msg[6] = (uint8_t)(ttr & 0xff); // 8 bits
    msg[7] = (uint8_t)((ttr >> 8) & 0xff); // 16 bits
    msg[8] = (uint8_t)(((ttr >> 16) & 0x1f) << 3); // 21 bits

    msg[8] |= tipo_payload & 0x7;

    memcpy(msg + message_without_payload_size, payload, payload_size);

    return msg;
}

/*
@brief Crea un mensaje desde lo recibido por una transmisión, es deber de caller liberar la memoria.
*/
Mensaje* Mensaje::parse_from_transmission(const unsigned char* data, uint8_t largo_data) {
    uint32_t _ttr;
    int16_t _emisor, _receptor, _nonce;
    int8_t _tipo_payload;
    unsigned char _payload[payload_max_size];

    memcpy(&_emisor, data, 2); // 0, 1
    memcpy(&_receptor, data + 2, 2); // 2, 3
    memcpy(&_nonce, data + 6, 2); // 4, 5

    _ttr = (data[6] & 0xff); // 8 bits
    _ttr |= ((uint32_t)data[7] & 0xff) << 8; // 16 bits
    _ttr |= ((uint32_t)data[8] & 0xf8) << 13; // 21 bits

    _tipo_payload = data[8] & 0x7;

    memcpy(_payload, data + message_without_payload_size, largo_data - message_without_payload_size);

    return new Mensaje(_ttr, _emisor, _receptor, _nonce, _tipo_payload, _payload, largo_data - message_without_payload_size);
}

void Mensaje::setEmisor(uint16_t _emisor) {
    emisor = _emisor;
}
void Mensaje::setReceptor(uint16_t _receptor) {
    receptor = _receptor;
}

void Mensaje::setNonce(uint16_t _nonce) {
    nonce = _nonce;
}

uint16_t Mensaje::getNonce() {
    return nonce;
}

uint16_t Mensaje::getEmisor() {
    return emisor;
}
uint16_t Mensaje::getReceptor() {
    return receptor;
}

uint8_t Mensaje::getTipoPayload() {
    return tipo_payload;
}