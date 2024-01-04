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

    const static unsigned max_largo_contenido_comprimido = 183;
    const static unsigned size_variables_transmission = 8; // tamaño de las variables al transmitir (sin contenido)
    const static unsigned max_size_transmision = max_largo_contenido_comprimido + size_variables_transmission;

    char* contenido, * contenido_comprimido;
    bool valido = false;

    Texto();
    Texto(
        uint16_t _nonce, uint16_t _creador, uint16_t _destinatario,
        uint8_t _saltos, unsigned _largo_texto,
        char* _contenido
    );
    Texto(const Texto& other);
    ~Texto();

    void parse_to_transmission(unsigned char* destino);
    uint64_t hash() const;
    uint8_t transmission_size();

    void print();

    bool operator==(const Texto& texto) const;
    bool operator!=(const Texto& texto) const;
    bool es_valido();
};

#endif