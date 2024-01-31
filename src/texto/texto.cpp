#include "texto.hpp"
#include <Arduino.h>

Texto::Texto() { }

Texto::Texto(
    uint16_t _nonce, uint16_t _creador, uint16_t _destinatario,
    uint8_t _saltos, int _largo_texto,
    char* _contenido, bool comprimido
) {
    Serial.println("Flag Texto constructor 1");
    if (_largo_texto < 1) return;
    Serial.println("Flag Texto constructor 2");
    nonce = _nonce;
    creador = _creador;
    destinatario = _destinatario;
    saltos = _saltos;
    Serial.println("Flag Texto constructor 3");
    if (comprimido) { //si el contenido viene o no comprimido
        Serial.println("Flag Texto constructor 4");
        if (_largo_texto > (int)max_largo_contenido_comprimido) {
            Serial.println("Flag Texto constructor 5");
            largo_texto_comprimido = largo_texto = 0;
            contenido = nullptr;
            contenido_comprimido = nullptr;
            Serial.println("Flag Texto constructor 6");
            return;
        }
        Serial.println("Flag Texto constructor 7");
        largo_texto_comprimido = _largo_texto;
        Serial.println("Flag Texto constructor 8");
        contenido_comprimido = new char[largo_texto_comprimido];
        Serial.println("Flag Texto constructor 9");
        std::memcpy(contenido_comprimido, _contenido, largo_texto_comprimido);
        Serial.println("Flag Texto constructor 10");
        valido = true;
        Serial.println("Flag Texto constructor 11");
        //descompresión
        char temp_descomprimido[3 * largo_texto_comprimido];
        Serial.println("Flag Texto constructor 12");
        largo_texto = unishox2_decompress_simple(_contenido, largo_texto_comprimido, temp_descomprimido);
        Serial.println("Flag Texto constructor 13");
        contenido = new char[largo_texto];
        std::memcpy(contenido, temp_descomprimido, largo_texto);
        Serial.println("Flag Texto constructor 14");
    }
    else {
        //compresión
        Serial.println("Flag Texto constructor 15");
        largo_texto = _largo_texto;
        char temp_comprimido[2 * largo_texto];
        Serial.println("Flag Texto constructor 16");
        int _largo_texto_comprimido = unishox2_compress_simple(_contenido, largo_texto, temp_comprimido);
        Serial.println("Flag Texto constructor 17");
        if (_largo_texto_comprimido > (int)max_largo_contenido_comprimido) {
            Serial.println("Flag Texto constructor 18");
            largo_texto_comprimido = largo_texto = 0;
            contenido = nullptr;
            contenido_comprimido = nullptr;
            Serial.println("Flag Texto constructor 19");
            return;
        }
        Serial.println("Flag Texto constructor 20");
        contenido = new char[largo_texto];
        Serial.println("Flag Texto constructor 21");
        std::memcpy(contenido, _contenido, largo_texto);
        Serial.println("Flag Texto constructor 22");
        valido = true;
        largo_texto_comprimido = _largo_texto_comprimido;
        Serial.println("Flag Texto constructor 23");
        contenido_comprimido = new char[largo_texto_comprimido];
        Serial.println("Flag Texto constructor 24");
        std::memcpy(contenido_comprimido, temp_comprimido, largo_texto_comprimido);
        Serial.println("Flag Texto constructor 25");
    }
    Serial.println("Flag Texto constructor 26");
}

Texto::Texto(const Texto& other) {
    if (!other.valido) return;

    valido = true;
    nonce = other.nonce;
    creador = other.creador;
    destinatario = other.destinatario;
    saltos = other.saltos;
    largo_texto = other.largo_texto;
    contenido = new char[largo_texto];
    std::memcpy(contenido, other.contenido, largo_texto);

    largo_texto_comprimido = other.largo_texto_comprimido;
    contenido_comprimido = new char[largo_texto_comprimido];
    std::memcpy(contenido_comprimido, other.contenido_comprimido, largo_texto_comprimido);
}

Texto::~Texto() {
    valido = false;
    if (largo_texto > 0)
        delete[] contenido;
    contenido = nullptr;
    largo_texto = 0;

    if (largo_texto_comprimido > 0)
        delete[] contenido_comprimido;
    contenido_comprimido = nullptr;
    largo_texto_comprimido = 0;
}

Texto& Texto::operator=(const Texto& other) {
    nonce = other.nonce;
    creador = other.creador;
    destinatario = other.destinatario;
    saltos = other.saltos;
    valido = other.valido;

    if (this != &other) {
        if (largo_texto > 0) delete[] contenido;
        if (other.largo_texto > 0) {
            largo_texto = other.largo_texto;
            contenido = new char[largo_texto];
            memcpy(contenido, other.contenido, largo_texto);
        }

        if (largo_texto_comprimido > 0) delete[] contenido_comprimido;
        if (other.largo_texto_comprimido > 0) {
            largo_texto_comprimido = other.largo_texto_comprimido;
            contenido_comprimido = new char[largo_texto_comprimido];
            memcpy(contenido_comprimido, other.contenido_comprimido, largo_texto_comprimido);
        }
    }
    return *this;
}

/*
@brief convierte los datos en un arreglo de bytes listos para transmitir. Destino debe tener al menos 'size_variables_transmission' + 'largo_texto_comprimido' bytes disponibles.
*/
void Texto::parse_to_transmission(unsigned char* destino) const {
    std::memcpy(destino, &nonce, 2);
    std::memcpy(destino + 2, &creador, 2);
    std::memcpy(destino + 4, &destinatario, 2);
    std::memcpy(destino + 6, &saltos, 1);
    std::memcpy(destino + 7, &largo_texto_comprimido, 1);
    std::memcpy(destino + 8, contenido_comprimido, largo_texto_comprimido);
}

/*
@brief Obtiene el hash del texto.
*/
uint64_t Texto::hash() const {
    uint64_t data = 0;
    data |= ((uint64_t)nonce) << 48;
    data |= ((uint64_t)creador) << 32;
    data |= ((uint64_t)destinatario) << 16;
    return data;
}

uint8_t Texto::transmission_size() const {
    return size_variables_transmission + largo_texto_comprimido;
}

void Texto::print() const {
    char _contenido[largo_texto + 1];
    std::memcpy(_contenido, contenido, largo_texto);
    _contenido[largo_texto] = '\0';
    Serial.print("\tValido: ");
    Serial.println(valido ? "True" : "False");
    Serial.print("\tCreador: ");
    Serial.println(creador);
    Serial.print("\tDestinatario: ");
    Serial.println(destinatario);
    Serial.print("\tNonce: ");
    Serial.println(nonce);
    Serial.print("\tSaltos: ");
    Serial.println(saltos);
    Serial.print("\tLargo texto: ");
    Serial.println(largo_texto);
    Serial.print("\tLargo texto comprimido: ");
    Serial.println(largo_texto_comprimido);
    Serial.print("\tContenido: ");
    Serial.println(_contenido);
}

bool Texto::operator==(const Texto& texto) const {
    return nonce == texto.nonce && creador == texto.creador && destinatario == texto.destinatario;
}

bool Texto::operator!=(const Texto& texto) const {
    return nonce != texto.nonce || creador != texto.creador || destinatario != texto.destinatario;
}

bool Texto::es_valido() {
    return valido;
}