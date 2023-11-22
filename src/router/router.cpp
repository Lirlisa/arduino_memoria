#include <LoRa.h>
#include <router/router.hpp>
#include <algorithm>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>

Router::Router(int initial_capacity) {
    buffer = new Mensaje_texto * [initial_capacity];
    size = 0;
    mensajes_enviados = 0;
    capacidad = initial_capacity;
    primer_elemento = 0;
    ultimo_elemento = 0;
}

Router::~Router() {
    for (unsigned int i = 0; i < size; i++) {
        delete buffer[i];
    }
    delete[] buffer;
    buffer = nullptr;
}

bool Router::realocar_buffer() {
    Mensaje_texto** new_buffer = new Mensaje_texto * [capacidad * 2];
    if (primer_elemento <= ultimo_elemento)
        std::copy(buffer + primer_elemento, buffer + ultimo_elemento, new_buffer);
    else {
        std::copy(buffer + primer_elemento, buffer + capacidad - 1, new_buffer);
        std::copy(buffer, buffer + ultimo_elemento, new_buffer + capacidad - primer_elemento);
    }
    primer_elemento = 0;
    ultimo_elemento = size - 1;

    delete[] buffer;
    buffer = new_buffer;
    return true;
}

bool Router::hay_espacio() {
    return size < capacidad;
}

bool Router::guardar_mensaje(Mensaje_texto* msg) {
    if (!hay_espacio()) realocar_buffer();

    ultimo_elemento = (ultimo_elemento + 1) % capacidad;
    buffer[ultimo_elemento] = msg;
    size++;
    return true;
}

bool Router::hay_mensaje() {
    return size > 0;
}

/*
    Hay que asegurarse de revisar si hay mensaje primero
*/
Mensaje_texto* Router::obtener_sgte_mensaje() {
    if (!hay_mensaje()) return nullptr;
    return buffer[primer_elemento];
}

bool Router::enviar_mensaje(int receptor) {
    if (LoRa.beginPacket() == 0) return false;
    if (!hay_mensaje()) return false;

    Mensaje_texto mensaje = *obtener_sgte_mensaje();
    unsigned char* mensaje_a_enviar = mensaje.parse_to_transmission();
    LoRa.beginPacket();
    LoRa.write(mensaje_a_enviar, mensaje.raw_message_size);
    LoRa.endPacket(true); // true = async / non-blocking mode

    return true;
}

bool Router::eliminar_mensaje() {
    delete buffer[ultimo_elemento];
    ultimo_elemento = ultimo_elemento == 0 ? capacidad - 1 : ultimo_elemento - 1;
    size--;
    return true;
}

bool Router::recibir_mensaje() {
    if (LoRa.parsePacket() == 0) return false;

    unsigned char data[Mensaje_texto::raw_message_size];
    for (int i = 0; i < Mensaje_texto::raw_message_size && LoRa.available(); i++) {
        data[i] = LoRa.read();
    }

    Mensaje* msg = Mensaje::parse_from_transmission(data);
    Mensaje_texto mensaje_recibido = Mensaje_texto(*msg);
    mensaje_recibido.print();

    delete msg;
    return true;
}