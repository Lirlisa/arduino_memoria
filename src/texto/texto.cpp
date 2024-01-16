#include "texto.hpp"

Texto::Texto() { }

Texto::Texto(
    uint16_t _nonce, uint16_t _creador, uint16_t _destinatario,
    uint8_t _saltos, int _largo_texto,
    char* _contenido
) {
    if (_largo_texto <= 0) return;

    largo_texto = _largo_texto;
    nonce = _nonce;
    creador = _creador;
    destinatario = _destinatario;
    saltos = _saltos;

    contenido = new char[largo_texto];
    std::memcpy(contenido, _contenido, largo_texto);

    //compresión
    char temp_comprimido[2 * largo_texto];
    int _largo_texto_comprimido = unishox2_compress_simple(contenido, largo_texto, temp_comprimido);

    if (_largo_texto_comprimido > (int)max_largo_contenido_comprimido) {
        char textito[largo_texto + 1];
        std::memcpy(textito, contenido, largo_texto);
        textito[largo_texto] = '\0';
        Serial.println("intentando comprimir: ");
        Serial.println(textito);
        Serial.println("Texto comprimido es demasiado largo");
        Serial.print("_largo_texto_comprimido = ");
        Serial.println(_largo_texto_comprimido);
        Serial.print("(int)max_largo_contenido_comprimido = ");
        Serial.println((int)max_largo_contenido_comprimido);
        Serial.print("max_largo_contenido_comprimido = ");
        Serial.println(max_largo_contenido_comprimido);
        delete[] contenido;
        largo_texto_comprimido = largo_texto = 0;
        return;
    }
    valido = true;
    largo_texto_comprimido = _largo_texto_comprimido;
    contenido_comprimido = new char[largo_texto_comprimido];
    std::memcpy(contenido_comprimido, temp_comprimido, largo_texto_comprimido);
}

Texto::Texto(const Texto& other) {
    Serial.println("Copiando Texto");
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

/*
@brief convierte los datos en un arreglo de bytes listos para transmitir. Destino debe tener al menos 'size_variables_transmission' + 'largo_texto_comprimido' bytes disponibles.
*/
void Texto::parse_to_transmission(unsigned char* destino) {
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

uint8_t Texto::transmission_size() {
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