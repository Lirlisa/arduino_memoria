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
        if (_payload[pos_size_contenido] == 0) break;
        _cantidad_textos++;
        pos_size_contenido += _payload[pos_size_contenido] + Texto::size_variables_transmission;
    } while (pos_size_contenido < payload_size);
    cantidad_textos = _cantidad_textos;
    textos = new Texto[cantidad_textos];
    int pos_inicio_texto = 0;
    for (uint8_t i = 0; i < cantidad_textos; i++) {
        int largo_texto_comprimido = std::min((unsigned)_payload[pos_inicio_texto + Texto::size_variables_transmission - 1], Texto::max_largo_contenido_comprimido);
        if (largo_texto_comprimido >= 0) {
            textos[i].valido = true;
            textos[i].contenido_comprimido = new char[largo_texto_comprimido];
            memcpy(&(textos[i].nonce), _payload + pos_inicio_texto, 2);
            memcpy(&(textos[i].creador), _payload + pos_inicio_texto + 2, 2);
            memcpy(&(textos[i].destinatario), _payload + pos_inicio_texto + 4, 2);
            textos[i].saltos = _payload[pos_inicio_texto + 6]; // 1 byte
            textos[i].largo_texto_comprimido = largo_texto_comprimido;
            memcpy(textos[i].contenido_comprimido, _payload + pos_inicio_texto + Texto::size_variables_transmission, textos[i].largo_texto_comprimido);

            //descompresión
            char descomprimido[3 * textos[i].largo_texto_comprimido];
            unsigned largo_descomprimido = unishox2_decompress_simple(
                textos[i].contenido_comprimido,
                textos[i].largo_texto_comprimido,
                descomprimido
            );
            textos[i].contenido = new char[largo_descomprimido];
            std::memcpy(textos[i].contenido, descomprimido, largo_descomprimido);
            textos[i].largo_texto = largo_descomprimido;
        }
        pos_inicio_texto += Texto::size_variables_transmission + (largo_texto_comprimido > 0 ? largo_texto_comprimido : 0);
    }
}


Mensaje_texto::Mensaje_texto(Mensaje const& origen) : Mensaje(origen) {
    cantidad_textos = 0;
    int pos_size_contenido = Texto::size_variables_transmission - 1;
    do {
        if (payload[pos_size_contenido] == 0) break;
        cantidad_textos++;
        pos_size_contenido += payload[pos_size_contenido] + Texto::size_variables_transmission;
    } while (pos_size_contenido < payload_size);

    textos = new Texto[cantidad_textos];

    int pos_inicio_texto = 0;
    for (uint8_t i = 0; i < cantidad_textos; i++) {
        memcpy(&(textos[i].nonce), payload + pos_inicio_texto, 2);
        memcpy(&(textos[i].creador), payload + pos_inicio_texto + 2, 2);
        memcpy(&(textos[i].destinatario), payload + pos_inicio_texto + 4, 2);
        memcpy(&(textos[i].saltos), payload + pos_inicio_texto + 6, 1);
        memcpy(&(textos[i].largo_texto), payload + pos_inicio_texto + 7, 1);
        textos[i].contenido = new char[textos[i].largo_texto];
        memcpy(textos[i].contenido, payload + pos_inicio_texto + Texto::size_variables_transmission, textos[i].largo_texto);
        pos_inicio_texto += Texto::size_variables_transmission + textos[i].largo_texto;
    }
}

Mensaje_texto::Mensaje_texto(const Mensaje_texto& origen) : Mensaje(
    origen.ttr, origen.emisor, origen.receptor, origen.nonce, Mensaje_texto::PAYLOAD_TEXTO, origen.payload, origen.payload_size
) {
    Serial.println("Copiando Mensaje_texto");
    cantidad_textos = origen.cantidad_textos;
    if (cantidad_textos <= 0) return;

    textos = new Texto[cantidad_textos];
    for (unsigned i = 0; i < cantidad_textos; i++) {
        uint8_t _largo_texto = std::min((unsigned)origen.textos[i].largo_texto, Texto::max_largo_contenido_comprimido);
        textos[i].nonce = origen.textos[i].nonce;
        textos[i].creador = origen.textos[i].creador;
        textos[i].destinatario = origen.textos[i].destinatario;
        textos[i].saltos = origen.textos[i].saltos;
        textos[i].largo_texto = _largo_texto;
        textos[i].contenido = new char[_largo_texto];
        std::memcpy(textos[i].contenido, origen.textos[i].contenido, _largo_texto);
    }
}

Mensaje_texto::~Mensaje_texto() {
    Serial.println("Eliminando mensaje texto");
    if (cantidad_textos > 0)
        delete[] textos;
    textos = nullptr;
}

void Mensaje_texto::print() {
    Mensaje::print();

    Serial.println("Textos:");
    for (uint8_t i = 0; i < cantidad_textos; i++) {
        Serial.print("\tTexto ");
        Serial.println(i);
        textos[i].print();
    }
}

/*
@brief Crea un vector de mensajes de texto, **EL NONCE NO ES VÁLIDO CUANDO LOS TEXTOS NO CABEN EN UN SÓLO MENSAJE**
*/
std::vector<Mensaje_texto> Mensaje_texto::crear_mensajes(
    uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
    uint16_t _nonce, std::vector<Texto>& textos
) {
    std::vector<Mensaje_texto> mensajes_texto;
    int textos_restantes = textos.size();
    int bytes_restantes;
    std::vector<Texto>::iterator curr_texto = textos.begin();
    std::vector<Texto>::iterator ultimo_texto_a_enviar;
    unsigned contador;
    while (textos_restantes > 0) {
        bytes_restantes = payload_max_size;
        for (std::vector<Texto>::iterator texto = curr_texto; texto != textos.end(); texto++) {
            if (bytes_restantes - texto->transmission_size() < 0 || textos_restantes <= 0) break;
            bytes_restantes -= texto->transmission_size();
            textos_restantes--;
            ultimo_texto_a_enviar = texto;
            Serial.println("----- Texto solo en crear_mensajes -----");
            texto->print();
            Serial.println("----- Fin Texto solo en crear_mensajes -----");
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
        Serial.println("----- Mensaje solo en crear_mensajes -----");
        msg.print();
        Serial.println("----- Fin Mensaje solo en crear_mensajes -----");
        Serial.println("----- Mensaje en vector en crear_mensajes -----");
        mensajes_texto.insert(mensajes_texto.end(), msg);
        mensajes_texto[0].print();
        Serial.println("----- Fin Mensaje en vector en crear_mensajes -----");
        curr_texto = ultimo_texto_a_enviar + 1;
    }
    return mensajes_texto;
}

std::vector<Texto> Mensaje_texto::obtener_textos() {
    std::vector<Texto> _textos(textos, textos + cantidad_textos);
    return _textos;
}

uint8_t Mensaje_texto::get_cantidad_textos() {
    return cantidad_textos;
}