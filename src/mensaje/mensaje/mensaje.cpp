#include <mensaje/mensaje/mensaje.hpp>
#include <cstdint>
#include <cstring>
#include <Arduino.h>
#include <algorithm>


Mensaje::Mensaje() {
    ttr = 0;
    emisor = receptor = nonce = 0;
    tipo_payload = payload_size = 0;
    payload = nullptr;
    transmission_size = message_without_payload_size;
}

Mensaje::Mensaje(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
    uint16_t _nonce, uint8_t _tipo_payload,
    unsigned char* _payload, unsigned _payload_size
) {
    ttr = _ttr;
    emisor = _emisor;
    receptor = _receptor;
    nonce = _nonce;
    tipo_payload = _tipo_payload;
    payload_size = std::min(_payload_size, payload_max_size);

    if (payload_size > 0) {
        payload = new unsigned char[payload_size];
        std::memcpy(payload, _payload, payload_size);
    }
    transmission_size = message_without_payload_size + payload_size;
}

Mensaje::Mensaje(const Mensaje& original) {
    ttr = original.ttr;
    emisor = original.emisor;
    receptor = original.receptor;
    nonce = original.nonce;
    tipo_payload = original.tipo_payload;
    payload_size = std::min((unsigned)(original.payload_size), payload_max_size);
    transmission_size = std::min(original.transmission_size, message_without_payload_size + payload_max_size);

    if (payload_size > 0) {
        payload = new unsigned char[payload_size];
        std::memcpy(payload, original.payload, payload_size);
    }
}

Mensaje::Mensaje(const unsigned char* data, uint8_t largo_data) {
    uint32_t _ttr;
    int16_t _emisor, _receptor, _nonce;
    int8_t _tipo_payload;

    std::memcpy(&_emisor, data, 2); // 0, 1
    std::memcpy(&_receptor, data + 2, 2); // 2, 3
    std::memcpy(&_nonce, data + 4, 2); // 4, 5

    _ttr = (data[6] & 0xff); // 8 bits
    _ttr |= ((uint32_t)data[7] & 0xff) << 8; // 16 bits
    _ttr |= ((uint32_t)data[8] & 0xf8) << 13; // 21 bits

    _tipo_payload = data[8] & 0x7;

    ttr = _ttr;
    emisor = _emisor;
    receptor = _receptor;
    nonce = _nonce;
    tipo_payload = _tipo_payload;
    payload_size = std::min(largo_data - message_without_payload_size, payload_max_size);
    if (payload_size > 0) {
        payload = new unsigned char[payload_size];
        std::memcpy(payload, data + message_without_payload_size, payload_size);
    }
    transmission_size = message_without_payload_size + payload_size;
}

Mensaje::~Mensaje() {
    if (payload_size > 0)
        delete[] payload;

    payload_size = 0;
    payload = nullptr;
}

bool Mensaje::operator!=(const Mensaje& other) const {
    return nonce != other.nonce || emisor != other.emisor || receptor != other.receptor;
}

bool Mensaje::operator==(const Mensaje& other) const {
    return nonce == other.nonce && emisor == other.emisor && receptor == other.receptor;
}

Mensaje& Mensaje::operator=(const Mensaje& other) {
    if (this == &other) {
        return *this;
    }
    ttr = other.ttr;
    emisor = other.emisor;
    receptor = other.receptor;
    nonce = other.nonce;
    tipo_payload = other.tipo_payload;
    payload_size = other.payload_size;
    transmission_size = other.transmission_size;

    // delete[] payload; // Free existing memory
    if (payload_size > 0) {
        payload = new unsigned char[payload_size];
        memcpy(payload, other.payload, payload_size);
    }

    return *this;
}

/*
@brief Imprime los primeros bytes del payload.
*/
void Mensaje::peek(unsigned cant_bytes) const {
    if (payload_size < 1) {
        Serial.println("Sin payload");
        return;
    }
    char repr[5];
    repr[4] = '\0';
    for (unsigned i = 0; i < cant_bytes; i++) {
        sprintf(repr, "%02x", payload[i]);
        Serial.print(repr);
    }
}

void Mensaje::print(unsigned cant_bytes) const {
    Serial.print("Emisor: ");
    Serial.println(emisor);
    Serial.print("Receptor: ");
    Serial.println(receptor);
    Serial.print("Nonce: ");
    Serial.println(nonce);
    Serial.print("TTR: ");
    Serial.println(ttr);
    Serial.print("Tipo payload: ");
    switch (tipo_payload) {
    case PAYLOAD_ACK_COMUNICACION:
        Serial.println("ACK Comunicación");
        break;
    case PAYLOAD_ACK_MENSAJE:
        Serial.println("ACK Mensaje");
        break;
    case PAYLOAD_BEACON:
        Serial.println("Beacon");
        break;
    case PAYLOAD_TEXTO:
        Serial.println("Texto");
        break;
    case PAYLOAD_VECTOR:
        Serial.println("Vector");
        break;
    case PAYLOAD_TEXTO_VISTO:
        Serial.println("Textos Vistos");
        break;
    case PAYLOAD_BEACON_CENTRAL:
        Serial.println("Beacon Central");
        break;
    default:
        Serial.println("Tipo de payload corrupto");
    }
    Serial.print("Payload size: ");
    Serial.println(payload_size);
    Serial.print("transmission size: ");
    Serial.println(transmission_size);
    if (payload_size > 0) {
        Serial.print("Payload(");
        Serial.print(cant_bytes);
        Serial.print("): ");
        peek(cant_bytes);
        Serial.println("");
    }
}

/**
@brief Crea mensaje para transmitir a partir del mensaje. El destino debe tener al menos 'transmission_size' bytes disponibles.
*/
void Mensaje::parse_to_transmission(unsigned char* destino) const {
    std::memcpy(destino, &emisor, 2); // 0, 1
    std::memcpy(destino + 2, &receptor, 2); // 2, 3
    std::memcpy(destino + 4, &nonce, 2); // 4, 5

    destino[6] = (uint8_t)(ttr & 0xff); // 8 bits
    destino[7] = (uint8_t)((ttr >> 8) & 0xff); // 16 bits
    destino[8] = (uint8_t)(((ttr >> 16) & 0x1f) << 3); // 21 bits

    destino[8] |= tipo_payload & 0x7;

    std::memcpy(destino + message_without_payload_size, payload, payload_size);
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

void Mensaje::setTTR(uint32_t _ttr) {
    ttr = _ttr;
}

uint16_t Mensaje::getNonce() const {
    return nonce;
}

uint16_t Mensaje::getEmisor() const {
    return emisor;
}
uint16_t Mensaje::getReceptor() const {
    return receptor;
}

uint8_t Mensaje::getTipoPayload() const {
    return tipo_payload;
}

uint32_t Mensaje::getTTR() const {
    return ttr;
}

unsigned Mensaje::get_transmission_size() const {
    return transmission_size;
}