#include <LoRa.h>
#include <router/router.hpp>
#include <algorithm>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>

Router::Router(int initial_capacity) {
    buffer = new Mensaje_texto * [initial_capacity];
    size = 0;
    mensajes_enviados = 0;
    capacidad = initial_capacity;
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
    std::copy(buffer, buffer + capacidad, new_buffer); // Suggested by comments from Nick and Bojan
    delete[] buffer;
    buffer = new_buffer;
    return true;
}

bool Router::hay_espacio() {
    return size < capacidad;
}

bool Router::guardar_mensaje(Mensaje_texto* msg) {
    if (!hay_espacio()) realocar_buffer();

    buffer[size++] = msg;
    return true;
}

bool Router::enviar_mensaje(int receptor) {
    if (LoRa.beginPacket() == 0) return false;

    Mensaje_texto mensaje = *buffer[0];
    unsigned char* mensaje_a_enviar = mensaje.parse_to_transmission();
    LoRa.beginPacket();
    LoRa.write(mensaje_a_enviar, mensaje.raw_message_size);
    LoRa.endPacket(true); // true = async / non-blocking mode

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


// wait until the radio is ready to send a packet
    // while (LoRa.beginPacket() == 0) {
    //     Serial.print("waiting for radio ... ");
    //     delay(100);
    // }

    // Serial.print("Sending packet non-blocking: ");
    // Serial.println(counter);

    // // send in async / non-blocking mode
    // LoRa.beginPacket();
    // LoRa.print("hello ");
    // LoRa.print(counter);
    // LoRa.endPacket(true); // true = async / non-blocking mode

    // counter++;