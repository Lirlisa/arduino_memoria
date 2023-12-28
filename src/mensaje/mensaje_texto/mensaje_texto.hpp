#ifndef MENSAJE_TEXTO_HPP
#define MENSAJE_TEXTO_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <cstdint>
#include <cstring>
#include <vector>

struct Texto {
    uint16_t nonce = 0, creador = 0, destinatario = 0;
    uint8_t saltos = 0, largo_texto = 0;
    char* contenido;

    ~Texto() {
        if (largo_texto > 0)
            delete[] contenido;
        contenido = nullptr;
    }

    /*
    @brief convierte los datos en un arreglo de bytes listos para transmitir. Es deber del caller liberar la memoria.
    */
    unsigned char* parse_to_transmission() {
        unsigned char* data = new unsigned char[8 + largo_texto];
        std::memcpy(data, &nonce, 2);
        std::memcpy(data + 2, &creador, 2);
        std::memcpy(data + 4, &destinatario, 2);
        std::memcpy(data + 6, &saltos, 1);
        std::memcpy(data + 7, &largo_texto, 1);
        std::memcpy(data + 8, contenido, largo_texto);

        return data;
    }

    /*
    @brief Obtiene el hash del texto.
    */
    uint64_t hash() const {
        uint64_t data = 0;
        data |= ((uint64_t)nonce) << 48;
        data |= ((uint64_t)creador) << 32;
        data |= ((uint64_t)destinatario) << 16;
        return data;
    }

    uint8_t transmission_size() {
        return 8 + largo_texto;
    }

    bool operator==(const Texto& texto) const {
        return nonce == texto.nonce && creador == texto.creador && destinatario == texto.destinatario;
    }

    bool operator!=(const Texto& texto) const {
        return nonce != texto.nonce || creador != texto.creador || destinatario != texto.destinatario;
    }
};

class Mensaje_texto : public Mensaje {
private:
    Texto* textos;

public:
    Mensaje_texto(
        uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, unsigned char* _payload,
        int payload_size
    );
    Mensaje_texto(Mensaje const& origen);
    ~Mensaje_texto();

    uint8_t cantidad_textos;

    static std::vector<Mensaje_texto> crear_mensajes(
        uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, std::vector<Texto>& textos
    );

    void print();

    std::vector<Texto> obtener_textos();
};

#endif