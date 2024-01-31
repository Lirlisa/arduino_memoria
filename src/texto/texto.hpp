#ifndef TEXTO_HPP
#define TEXTO_HPP

#include <unishox2.h>
#include <cstdint>
#include <cstring>
#include <Arduino.h>

struct Texto {
    uint16_t nonce = 0, creador = 0, destinatario = 0;
    uint8_t saltos = 0, largo_texto_comprimido = 0;
    unsigned largo_texto = 0;

    char* contenido, * contenido_comprimido;
    bool valido = false;

    const static unsigned max_largo_contenido_comprimido = 183;
    // tamaño de las variables al transmitir (sin contenido)
    const static unsigned size_variables_transmission = 8;
    const static unsigned max_size_transmision = max_largo_contenido_comprimido + size_variables_transmission;

    Texto();
    Texto(
        uint16_t _nonce, uint16_t _creador, uint16_t _destinatario,
        uint8_t _saltos, int _largo_texto,
        char* _contenido, bool comprimido = false
    );
    Texto(const Texto& other);
    ~Texto();

    Texto& operator=(const Texto& other);

    void parse_to_transmission(unsigned char* destino) const;
    uint64_t hash() const;
    uint8_t transmission_size() const;

    void print() const;

    bool operator==(const Texto& texto) const;
    bool operator!=(const Texto& texto) const;
    bool es_valido();
};

#endif