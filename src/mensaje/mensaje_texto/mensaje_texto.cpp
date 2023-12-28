#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <cstdint>
#include <Arduino.h>


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
    int pos_size_contenido = 7;
    do {
        if (_payload[pos_size_contenido] == 0) break;
        _cantidad_textos++;
        pos_size_contenido += _payload[pos_size_contenido] + 8;
    } while (pos_size_contenido < payload_size);

    cantidad_textos = _cantidad_textos;
    textos = new Texto[cantidad_textos];

    int pos_inicio_texto = 0;
    for (uint8_t i = 0; i < cantidad_textos; i++) {
        uint8_t largo_texto = _payload[pos_inicio_texto + 7];
        textos[i].contenido = new char[largo_texto];
        memcpy(&(textos[i].nonce), _payload + pos_inicio_texto, 2);
        memcpy(&(textos[i].creador), _payload + pos_inicio_texto + 2, 2);
        memcpy(&(textos[i].destinatario), _payload + pos_inicio_texto + 4, 2);
        textos[i].saltos = _payload[pos_inicio_texto + 6]; // 1 byte
        textos[i].largo_texto = largo_texto; // _payload + pos_inicio_texto + 7, 1 byte
        memcpy(&(textos[i].contenido), _payload + pos_inicio_texto + 8, textos[i].largo_texto);
        pos_inicio_texto += 8 + textos[i].largo_texto;
    }
}


Mensaje_texto::Mensaje_texto(Mensaje const& origen) : Mensaje(origen) {
    int cantidad_textos = 0;
    int pos_size_contenido = 7;
    do {
        if (payload[pos_size_contenido] == 0) break;
        cantidad_textos++;
        pos_size_contenido += payload[pos_size_contenido] + 8;
    } while (pos_size_contenido < payload_size);

    textos = new Texto[cantidad_textos];

    int pos_inicio_texto = 0;
    for (uint8_t i = 0; i < cantidad_textos; i++) {
        memcpy(&(textos[i].nonce), payload + pos_inicio_texto, 2);
        memcpy(&(textos[i].creador), payload + pos_inicio_texto + 2, 2);
        memcpy(&(textos[i].destinatario), payload + pos_inicio_texto + 4, 2);
        memcpy(&(textos[i].saltos), payload + pos_inicio_texto + 6, 1);
        memcpy(&(textos[i].largo_texto), payload + pos_inicio_texto + 7, 1);
        memcpy(&(textos[i].contenido), payload + pos_inicio_texto + 8, textos[i].largo_texto);
        pos_inicio_texto += 8 + textos[i].largo_texto;
    }
}

Mensaje_texto::~Mensaje_texto() {
    if (cantidad_textos > 0)
        delete[] textos;
    textos = nullptr;
}

void Mensaje_texto::print() {
    Mensaje::print();

    Serial.println("Textos:");
    for (uint8_t i = 0; i < cantidad_textos; i++) {
        Serial.print("\tTexto ");
        Serial.print(i);
        Serial.println(":");
        Serial.print("\t\tNonce: ");
        Serial.println(textos[i].nonce);
        Serial.print("\t\tCreador: ");
        Serial.println(textos[i].creador);
        Serial.print("\t\tDestinatario: ");
        Serial.println(textos[i].destinatario);
        Serial.print("\t\tSaltos: ");
        Serial.println(textos[i].saltos);
        Serial.print("\t\tLargo texto: ");
        Serial.println(textos[i].largo_texto);
        Serial.print("\t\tContenido: ");
        Serial.println(textos[i].contenido);
        Serial.println("-----");
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
    unsigned textos_restantes = textos.size();
    uint8_t bytes_restantes;
    std::vector<Texto>::iterator curr_texto = textos.begin();

    while (textos_restantes > 0) {
        bytes_restantes = payload_max_size;
        for (std::vector<Texto>::iterator texto = curr_texto; texto != textos.end(); texto++) {
            if (bytes_restantes - texto->transmission_size() >= 0 && textos_restantes > 0) {
                bytes_restantes -= texto->transmission_size();
                textos_restantes--;
                continue;
            }
            unsigned char* _payload = new unsigned char[payload_max_size - bytes_restantes];
            unsigned contador = 0;
            for (std::vector<Texto>::iterator it = curr_texto; it != texto; it++) {
                uint8_t size_text = it->transmission_size();
                unsigned char* data = texto->parse_to_transmission();
                std::memcpy(_payload + contador, data, size_text);
                contador += size_text;
                delete[] data;
            }
            mensajes_texto.push_back(
                Mensaje_texto(
                    _ttr, _emisor, _receptor,
                    _nonce, _payload,
                    payload_max_size - bytes_restantes
                )
            );
            delete[] _payload;
            curr_texto = texto;
            break;
        }
    }

    return mensajes_texto;
}

std::vector<Texto> Mensaje_texto::obtener_textos() {
    std::vector<Texto> _textos(textos, textos + cantidad_textos);
    return _textos;
}