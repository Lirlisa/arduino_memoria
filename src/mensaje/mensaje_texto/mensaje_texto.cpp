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
    uint16_t aux_nonce, aux_creador, aux_destinatario;
    uint8_t aux_saltos;
    int largo_texto_comprimido, pos_inicio_texto, _cantidad_textos = 0, pos_size_contenido = Texto::size_variables_transmission - 1;
    Serial.println("Flag Mensaje_texto constructor 1");
    do {
        Serial.println("Flag Mensaje_texto constructor 1.1");
        if (_payload[pos_size_contenido] <= 0 || _payload[pos_size_contenido] > Texto::max_largo_contenido_comprimido) break; // está corrupto y no se puede saber que sigue con certeza
        Serial.println("Flag Mensaje_texto constructor 1.2");
        _cantidad_textos++;
        Serial.println("Flag Mensaje_texto constructor 1.3");
        pos_size_contenido += _payload[pos_size_contenido] + Texto::size_variables_transmission;
        Serial.println("Flag Mensaje_texto constructor 1.4");
    } while (pos_size_contenido < payload_size);
    Serial.println("Flag Mensaje_texto constructor 2");
    pos_inicio_texto = 0;
    for (uint8_t i = 0; i < _cantidad_textos; i++) {
        Serial.println("Flag Mensaje_texto constructor 2.1");
        largo_texto_comprimido = _payload[pos_inicio_texto + Texto::size_variables_transmission - 1];
        Serial.println("Flag Mensaje_texto constructor 2.2");
        if (largo_texto_comprimido >= 0) {
            Serial.println("Flag Mensaje_texto constructor 2.3");
            memcpy(&aux_nonce, _payload + pos_inicio_texto, 2);
            Serial.println("Flag Mensaje_texto constructor 2.4");
            memcpy(&aux_creador, _payload + pos_inicio_texto + 2, 2);
            Serial.println("Flag Mensaje_texto constructor 2.5");
            memcpy(&aux_destinatario, _payload + pos_inicio_texto + 4, 2);
            Serial.println("Flag Mensaje_texto constructor 2.6");
            aux_saltos = _payload[pos_inicio_texto + 6]; // 1 byte
            Serial.println("Flag Mensaje_texto constructor 2.7");
            Serial.print("aux_nonce ");
            Serial.println(aux_nonce);
            Serial.print("aux_creador ");
            Serial.println(aux_creador);
            Serial.print("aux_destinatario ");
            Serial.println(aux_destinatario);
            Serial.print("aux_saltos ");
            Serial.println(aux_saltos);
            Serial.print("largo_texto_comprimido ");
            Serial.println(largo_texto_comprimido);
            Texto txt(
                aux_nonce, aux_creador, aux_destinatario,
                aux_saltos, largo_texto_comprimido, reinterpret_cast<char*>(_payload + pos_inicio_texto + Texto::size_variables_transmission),
                true
            );
            Serial.println("Flag Mensaje_texto constructor 2.8");
            textos.push_back(txt);
            Serial.println("Flag Mensaje_texto constructor 2.9");
        }
        Serial.println("Flag Mensaje_texto constructor 2.10");
        pos_inicio_texto += Texto::size_variables_transmission + (largo_texto_comprimido > 0 ? largo_texto_comprimido : 0);
        Serial.println("Flag Mensaje_texto constructor 2.11");
    }
}


Mensaje_texto::Mensaje_texto(Mensaje const& origen) : Mensaje(origen) {
    unsigned _cantidad_textos = 0;
    int pos_size_contenido = Texto::size_variables_transmission - 1;
    uint16_t _nonce, _creador, _destinatario;
    uint8_t _saltos;
    int largo_texto_comprimido;
    do {
        if (payload[pos_size_contenido] <= 0 || payload[pos_size_contenido] > Texto::max_largo_contenido_comprimido) break; // está corrupto y no se puede saber que sigue con certeza
        _cantidad_textos++;
        pos_size_contenido += payload[pos_size_contenido] + Texto::size_variables_transmission;
    } while (pos_size_contenido < payload_size);


    int pos_inicio_texto = 0;
    for (uint8_t i = 0; i < _cantidad_textos; i++) {
        largo_texto_comprimido = payload[pos_inicio_texto + Texto::size_variables_transmission - 1];
        if (largo_texto_comprimido > 0) {
            std::memcpy(&_nonce, payload + pos_inicio_texto, 2);
            std::memcpy(&_creador, payload + pos_inicio_texto + 2, 2);
            std::memcpy(&_destinatario, payload + pos_inicio_texto + 4, 2);
            std::memcpy(&_saltos, payload + pos_inicio_texto + 6, 1);
            Texto txt(
                _nonce, _creador, _destinatario,
                _saltos, largo_texto_comprimido, reinterpret_cast<char*>(payload + pos_inicio_texto + Texto::size_variables_transmission),
                true
            );
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
    Serial.println("Flag crear_mensajes 1");
    std::vector<Mensaje_texto> mensajes_texto;
    int textos_restantes = _textos.size();
    int bytes_restantes;
    Serial.println("Flag crear_mensajes 2");
    std::vector<Texto>::iterator curr_texto = _textos.begin();
    std::vector<Texto>::iterator ultimo_texto_a_enviar;
    Serial.println("Flag crear_mensajes 3");
    unsigned contador;
    while (textos_restantes > 0) {
        bytes_restantes = payload_max_size;
        Serial.println("Flag crear_mensajes 4");
        for (std::vector<Texto>::iterator texto = curr_texto; texto != _textos.end(); texto++) {
            Serial.println("Flag crear_mensajes 4.1");
            if (bytes_restantes - texto->transmission_size() < 0 || textos_restantes <= 0) break;
            Serial.println("Flag crear_mensajes 4.2");
            bytes_restantes -= texto->transmission_size();
            textos_restantes--;
            ultimo_texto_a_enviar = texto;
            Serial.println("Flag crear_mensajes 4.3");
        }
        Serial.println("Flag crear_mensajes 5");
        contador = 0;

        unsigned char _payload[payload_max_size - bytes_restantes];
        Serial.println("Flag crear_mensajes 6");
        for (std::vector<Texto>::iterator it = curr_texto; it != ultimo_texto_a_enviar + 1; it++) {
            Serial.println("Flag crear_mensajes 6.1");
            uint8_t size_text = it->transmission_size();
            Serial.println("Flag crear_mensajes 6.2");
            unsigned char data[it->transmission_size()];
            Serial.println("Flag crear_mensajes 6.3");
            it->parse_to_transmission(data);
            Serial.println("Flag crear_mensajes 6.4");
            std::memcpy(_payload + contador, data, size_text);
            Serial.println("Flag crear_mensajes 6.5");
            contador += size_text;
            Serial.println("Flag crear_mensajes 6.6");
        }
        Serial.println("Flag crear_mensajes 7");
        Serial.println(_ttr);
        Serial.println(_emisor);
        Serial.println(_receptor);
        Serial.println(_nonce);
        Serial.println(payload_max_size);
        Serial.println(bytes_restantes);
        Mensaje_texto msg = Mensaje_texto(
            _ttr, _emisor, _receptor,
            _nonce, _payload,
            payload_max_size - bytes_restantes
        );
        Serial.println("Flag crear_mensajes 8");
        mensajes_texto.push_back(msg);
        Serial.println("Flag crear_mensajes 9");
        curr_texto = ultimo_texto_a_enviar + 1;
        Serial.println("Flag crear_mensajes 10");
    }
    Serial.println("Flag crear_mensajes 11");
    return mensajes_texto;
}

std::vector<Texto> Mensaje_texto::obtener_textos() {
    return textos;
}

uint8_t Mensaje_texto::get_cantidad_textos() const {
    return textos.size();
}