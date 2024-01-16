#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <memoria/memoria.hpp>
#include <texto/texto.hpp>
#include <cstdint>
#include <Arduino.h>
#include <algorithm>
#include <unishox2.h>

Mensaje_texto::Mensaje_texto() {}

Mensaje_texto::Mensaje_texto(
    uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
    uint16_t _nonce, unsigned char* _payload,
    int payload_size
) : Mensaje(
    _ttr, _emisor, _receptor,
    _nonce, Mensaje::PAYLOAD_TEXTO,
    _payload, payload_size
) {
    int _cantidad_textos = 0;
    int pos_size_contenido = Texto::size_variables_transmission - 1;
    do {
        if (_payload[pos_size_contenido] <= 0 || _payload[pos_size_contenido] > Texto::max_largo_contenido_comprimido) break; // está corrupto y no se puede saber que sigue con certeza
        _cantidad_textos++;
        pos_size_contenido += _payload[pos_size_contenido] + Texto::size_variables_transmission;
    } while (pos_size_contenido < payload_size);

    int pos_inicio_texto = 0;
    for (uint8_t i = 0; i < _cantidad_textos; i++) {
        int largo_texto_comprimido = std::min((unsigned)_payload[pos_inicio_texto + Texto::size_variables_transmission - 1], Texto::max_largo_contenido_comprimido);
        if (largo_texto_comprimido >= 0) {
            Texto txt;
            txt.valido = true;
            txt.contenido_comprimido = new char[largo_texto_comprimido];
            memcpy(&(txt.nonce), _payload + pos_inicio_texto, 2);
            memcpy(&(txt.creador), _payload + pos_inicio_texto + 2, 2);
            memcpy(&(txt.destinatario), _payload + pos_inicio_texto + 4, 2);
            txt.saltos = _payload[pos_inicio_texto + 6]; // 1 byte
            txt.largo_texto_comprimido = largo_texto_comprimido;
            memcpy(txt.contenido_comprimido, _payload + pos_inicio_texto + Texto::size_variables_transmission, txt.largo_texto_comprimido);

            //descompresión
            char descomprimido[3 * txt.largo_texto_comprimido];
            unsigned largo_descomprimido = unishox2_decompress_simple(
                txt.contenido_comprimido,
                txt.largo_texto_comprimido,
                descomprimido
            );
            txt.contenido = new char[largo_descomprimido];
            std::memcpy(txt.contenido, descomprimido, largo_descomprimido);
            txt.largo_texto = largo_descomprimido;
            textos.push_back(txt);
        }
        pos_inicio_texto += Texto::size_variables_transmission + (largo_texto_comprimido > 0 ? largo_texto_comprimido : 0);
    }
}


Mensaje_texto::Mensaje_texto(Mensaje const& origen) : Mensaje(origen) {
    unsigned _cantidad_textos = 0;
    int pos_size_contenido = Texto::size_variables_transmission - 1;
    do {
        if (payload[pos_size_contenido] <= 0 || payload[pos_size_contenido] > Texto::max_largo_contenido_comprimido) break; // está corrupto y no se puede saber que sigue con certeza
        _cantidad_textos++;
        pos_size_contenido += payload[pos_size_contenido] + Texto::size_variables_transmission;
    } while (pos_size_contenido < payload_size);


    int pos_inicio_texto = 0;
    for (uint8_t i = 0; i < _cantidad_textos; i++) {
        int largo_texto_comprimido = std::min((unsigned)payload[pos_inicio_texto + Texto::size_variables_transmission - 1], Texto::max_largo_contenido_comprimido);
        if (largo_texto_comprimido > 0) {
            Texto txt;
            txt.valido = true;
            memcpy(&(txt.nonce), payload + pos_inicio_texto, 2);
            memcpy(&(txt.creador), payload + pos_inicio_texto + 2, 2);
            memcpy(&(txt.destinatario), payload + pos_inicio_texto + 4, 2);
            memcpy(&(txt.saltos), payload + pos_inicio_texto + 6, 1);
            memcpy(&(txt.largo_texto_comprimido), payload + pos_inicio_texto + 7, 1);
            txt.contenido_comprimido = new char[txt.largo_texto_comprimido];

            //descompresión
            char descomprimido[3 * txt.largo_texto_comprimido];
            unsigned largo_descomprimido = unishox2_decompress_simple(
                txt.contenido_comprimido,
                txt.largo_texto_comprimido,
                descomprimido
            );
            txt.contenido = new char[largo_descomprimido];
            std::memcpy(txt.contenido, descomprimido, largo_descomprimido);
            txt.largo_texto = largo_descomprimido;
            textos.push_back(txt);
        }
        pos_inicio_texto += Texto::size_variables_transmission + (largo_texto_comprimido > 0 ? largo_texto_comprimido : 0);
    }
}

Mensaje_texto::Mensaje_texto(const Mensaje_texto& origen) : Mensaje(
    origen.ttr, origen.emisor, origen.receptor, origen.nonce, Mensaje_texto::PAYLOAD_TEXTO, origen.payload, origen.payload_size
) {
    if (origen.get_cantidad_textos() <= 0) return;

    for (unsigned i = 0; i < origen.get_cantidad_textos(); i++) {
        textos.push_back(
            Texto(
                origen.textos[i].nonce, origen.textos[i].creador, origen.textos[i].destinatario,
                origen.textos[i].saltos, origen.textos[i].largo_texto, origen.textos[i].contenido
            )
        );
    }
}

Mensaje_texto::~Mensaje_texto() {
}

void Mensaje_texto::print() {
    Mensaje::print();
    Serial.println("Textos:");
    for (unsigned i = 0; i < textos.size(); i++) {
        Serial.print("\tTexto ");
        Serial.println(i);
        textos.at(i).print();
    }
}

/*
@brief Crea un vector de mensajes de texto, **EL NONCE NO ES VÁLIDO CUANDO LOS TEXTOS NO CABEN EN UN SÓLO MENSAJE**
*/
std::vector<Mensaje_texto> Mensaje_texto::crear_mensajes(
    uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
    uint16_t _nonce, std::vector<Texto>& _textos
) {
    std::vector<Mensaje_texto> mensajes_texto;
    int textos_restantes = _textos.size();
    int bytes_restantes;
    std::vector<Texto>::iterator curr_texto = _textos.begin();
    std::vector<Texto>::iterator ultimo_texto_a_enviar;
    unsigned contador;
    while (textos_restantes > 0) {
        bytes_restantes = payload_max_size;
        for (std::vector<Texto>::iterator texto = curr_texto; texto != _textos.end(); texto++) {
            if (bytes_restantes - texto->transmission_size() < 0 || textos_restantes <= 0) break;
            bytes_restantes -= texto->transmission_size();
            textos_restantes--;
            ultimo_texto_a_enviar = texto;
        }
        contador = 0;

        unsigned char _payload[payload_max_size - bytes_restantes];
        for (std::vector<Texto>::iterator it = curr_texto; it != ultimo_texto_a_enviar + 1; it++) {
            uint8_t size_text = it->transmission_size();
            unsigned char data[it->transmission_size()];
            it->parse_to_transmission(data);
            std::memcpy(_payload + contador, data, size_text);
            contador += size_text;

        }
        Mensaje_texto msg = Mensaje_texto(
            _ttr, _emisor, _receptor,
            _nonce, _payload,
            payload_max_size - bytes_restantes
        );
        mensajes_texto.push_back(msg);
        curr_texto = ultimo_texto_a_enviar + 1;
    }
    return mensajes_texto;
}

std::vector<Texto> Mensaje_texto::obtener_textos() {
    return textos;
}

uint8_t Mensaje_texto::get_cantidad_textos() const {
    return textos.size();
}