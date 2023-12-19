#include <router/router.hpp>
#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <mensaje/mensaje_ack_mensaje/mensaje_ack_mensaje.hpp>
#include <mensaje/ack.hpp>
#include <LoRa.h>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <vector>


Router::Router(uint16_t _id, uint32_t _ttr, unsigned int initial_capacity = 10) : buffer(id) {
    id = _id;
    ttr = _ttr;
    mensajes_enviados = 0;
    mis_mensajes = new Texto[initial_capacity];
    capacidad_mis_mensajes = initial_capacity;
    size_mis_mensajes = 0;
}

Router::~Router() {
    delete[] mis_mensajes;
    mis_mensajes = nullptr;
}

bool Router::agregar_a_mis_mensajes(Texto& texto) {
    if (texto.destinatario != id) return false;

    if (!hay_espacio_en_mis_mensajes())
        if (!realocar_mis_mensajes())
            return false;

    mis_mensajes[size_mis_mensajes++] = texto;
    agregar_a_acks(texto);

    return true;
}

bool Router::realocar_mis_mensajes() {
    Texto* new_mis_mensajes = new Texto[capacidad_mis_mensajes * 2];
    std::copy(mis_mensajes, mis_mensajes + size_mis_mensajes - 1, new_mis_mensajes);

    delete[] mis_mensajes;
    mis_mensajes = new_mis_mensajes;
    capacidad_mis_mensajes *= 2;
    return true;
}

bool Router::hay_espacio_en_mis_mensajes() {
    return size_mis_mensajes < capacidad_mis_mensajes;
}

bool Router::agregar_a_acks(Texto& texto) {
    uint64_t llave = texto.hash();
    buffer.agregar_ack(llave);
    return true;
}

bool Router::enviar_mensaje_texto_maxprop(uint16_t receptor, int periodos_espera) {
    if (!se_puede_transmitir_LoRa(periodos_espera)) return false;
    if (!buffer.hay_mensajes_bajo_threshold()) return true;

    std::vector<Texto> textos = buffer.obtener_textos_bajo_threshold();
    std::vector<Texto> textos_escogidos;
    std::vector<Texto>::iterator texto_actual = textos.begin();

    uint8_t bytes_restantes = Mensaje_texto::payload_max_size;
    while (texto_actual != textos.end()) {
        texto_actual++;
        if (bytes_restantes - (texto_actual - 1)->transmission_size() > 0 && texto_actual != textos.end()) {
            textos_escogidos.push_back(*(texto_actual - 1));
            bytes_restantes -= (texto_actual - 1)->transmission_size();
            continue;
        }
        uint16_t nonce = get_update_nonce();
        Mensaje_texto msg = Mensaje_texto::crear_mensajes(ttr, id, receptor, nonce, textos_escogidos).at(0);
        enviar_mensaje_texto(msg);
        bytes_restantes = Mensaje_texto::payload_max_size;
        if (!esperar_ack(receptor, nonce)) return false;
        textos_escogidos.clear();
    }
    return true;
}

/*
@brief Enviar en ttr significa que se envían los mensajes que estén sobre el threshold de saltos,
y que tengan una probabilidad de entrega mayor a la que este propio nodo tiene. Luego se eliminan.
*/
bool Router::enviar_mensaje_texto_ttr(uint16_t receptor, int periodos_espera) {
    if (!se_puede_transmitir_LoRa(periodos_espera)) return false;
    if (!buffer.hay_mensajes_sobre_threshold()) return true;

    std::vector<Texto> textos = buffer.obtener_textos_sobre_threshold();
    std::vector<Texto> textos_escogidos;
    std::vector<Texto>::iterator texto_actual = textos.begin();

    uint8_t bytes_restantes = Mensaje_texto::payload_max_size;
    while (texto_actual != textos.end()) {
        texto_actual++;
        if (bytes_restantes - (texto_actual - 1)->transmission_size() > 0 && texto_actual != textos.end()) {
            textos_escogidos.push_back(*(texto_actual - 1));
            bytes_restantes -= (texto_actual - 1)->transmission_size();
            continue;
        }
        uint16_t nonce = get_update_nonce();
        Mensaje_texto msg = Mensaje_texto::crear_mensajes(ttr, id, receptor, nonce, textos_escogidos).at(0);
        enviar_mensaje_texto(msg);
        bytes_restantes = Mensaje_texto::payload_max_size;
        if (!esperar_ack(receptor, nonce)) return false;
        buffer.eliminar_textos(textos_escogidos);
        textos_escogidos.clear();
    }
    return true;
}

bool Router::enviar_mensaje_texto(Mensaje_texto& msg, int periodos_espera) {
    if (!se_puede_transmitir_LoRa(periodos_espera)) return false;

    unsigned char* mensaje_a_enviar = msg.parse_to_transmission();

    LoRa.beginPacket();
    LoRa.write(mensaje_a_enviar, msg.transmission_size);
    LoRa.endPacket(true); // true = async / non-blocking mode

    delete[] mensaje_a_enviar;
    return true;
}

bool Router::se_puede_transmitir_LoRa(int periodos_espera) {
    int i = 0;
    bool transmision_disponible = false;
    while (i++ < periodos_espera) {
        if (LoRa.beginPacket() != 0) {
            transmision_disponible = true;
            break;
        }
        delay(300);
    }
    return transmision_disponible;
}

bool Router::hay_paquete_LoRa(int periodos_espera) {
    int i = 0;
    bool paquete_disponible = false;
    while (i++ < periodos_espera) {
        if (LoRa.parsePacket() != 0) {
            paquete_disponible = true;
            break;
        }
        delay(300);
    }
    return paquete_disponible;
}

bool Router::recibir_mensaje() {
    if (!hay_paquete_LoRa()) return false;

    unsigned char data[Mensaje::raw_message_max_size];
    unsigned largo = 0;
    for (int i = 0; i < Mensaje::raw_message_max_size && LoRa.available(); i++) {
        data[i] = LoRa.read();
        largo++;
    }

    Mensaje* msg = Mensaje::parse_from_transmission(data, largo);
    mensaje_pendiente = *msg;
    hay_mensaje_pendiente = true;
    delete msg;

    return true;
}

uint16_t Router::get_update_nonce() {
    return nonce_actual++;
}

bool Router::emitir_beacon_signal() {
    if (!se_puede_transmitir_LoRa()) return false;

    Mensaje mensaje = Mensaje(
        ttr, id, BROADCAST_CHANNEL_ID, get_update_nonce(), Mensaje::PAYLOAD_BEACON, (unsigned char*)nullptr, 0
    );
    unsigned char* mensaje_a_enviar = mensaje.parse_to_transmission();

    LoRa.beginPacket();
    LoRa.write(mensaje_a_enviar, mensaje.transmission_size);
    LoRa.endPacket(true); // true = async / non-blocking mode

    delete[] mensaje_a_enviar;

    return true;
}

bool Router::emitir_ack_comunicacion(uint16_t receptor_destinatario, uint16_t nonce_original) {
    if (!se_puede_transmitir_LoRa()) return false;

    Mensaje_ack_comunicacion mensaje = Mensaje_ack_comunicacion(
        id, receptor_destinatario, get_update_nonce(), nonce_original
    );
    unsigned char* mensaje_a_enviar = mensaje.parse_to_transmission();

    LoRa.beginPacket();
    LoRa.write(mensaje_a_enviar, mensaje.transmission_size);
    LoRa.endPacket(true); // true = async / non-blocking mode

    delete[] mensaje_a_enviar;

    return true;
}

bool  Router::esperar_ack(uint16_t id_nodo, uint16_t nonce, int periodos_espera) {
    int i = 0;
    while (i++ < periodos_espera && !recibir_mensaje()) {}

    if (!hay_mensaje_pendiente) return false;

    if (mensaje_pendiente.getReceptor() != id || mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION) {
        hay_mensaje_pendiente = false;
        return false;
    }
    if (!Mensaje_ack_comunicacion(mensaje_pendiente).confirmar_ack(nonce, id, id_nodo)) {
        hay_mensaje_pendiente = false;
        return false;
    }
    return true;
}

/*
@brief Envia todos los mensajes dirigidos al destinatario
*/
bool Router::enviar_todos_a_destinatario(uint16_t destinatario) {
    std::vector<Texto> textos_para_destinatario = buffer.obtener_textos_destinatario(destinatario);
    std::vector<Mensaje_texto> mensajes_a_enviar = Mensaje_texto::crear_mensajes(
        ttr, id, destinatario, 0, textos_para_destinatario
    );

    std::vector<Mensaje_texto>::iterator mensaje;
    uint16_t nonce;
    unsigned textos_enviados = 0;
    for (mensaje = mensajes_a_enviar.begin(); mensaje != mensajes_a_enviar.end(); mensaje++) {
        nonce = get_update_nonce();
        mensaje->setNonce(nonce);
        if (!enviar_mensaje_texto(*mensaje)) return false;
        if (!esperar_ack(destinatario, nonce)) return false;

        textos_enviados += mensaje->cantidad_textos;
        std::vector<Texto> textos_a_borrar(
            textos_para_destinatario.begin() + textos_enviados - mensaje->cantidad_textos,
            textos_para_destinatario.begin() + textos_enviados + 1
        );
        buffer.agregar_ack(textos_a_borrar);
    }
    return true;
}

bool Router::enviar_vectores_de_probabilidad(uint16_t receptor) {
    unsigned char* data = buffer.obtener_vector_probabilidad();
    uint16_t cantidad_pares_restantes;
    std::memcpy(&cantidad_pares_restantes, data, 2);

    uint16_t cantidad_pares_a_enviar, cantidad_pares_enviados, cantidad_pares_enviados_totales = 0;
    uint8_t cantidad_bytes;
    unsigned char data_a_enviar[Mensaje_vector::payload_max_size];
    while (cantidad_pares_restantes > 0) {
        cantidad_pares_a_enviar = std::min((unsigned)std::floor((Mensaje_vector::payload_max_size - 1) / 6), (unsigned)cantidad_pares_restantes);
        cantidad_bytes = cantidad_pares_a_enviar * 6;
        data_a_enviar[0] = cantidad_bytes;
        cantidad_pares_enviados = 0;
        for (uint8_t i = 0; i < cantidad_pares_a_enviar; i++) {
            memcpy(
                data_a_enviar + 1 + cantidad_pares_enviados * 6,
                data + 2 + (cantidad_pares_enviados_totales + cantidad_pares_enviados) * 6,
                6
            );
            cantidad_pares_enviados++;
        }

        if (!se_puede_transmitir_LoRa()) return false;

        uint16_t nonce = get_update_nonce();
        Mensaje_vector mensaje = Mensaje_vector(
            id, receptor, nonce, data_a_enviar, 1 + cantidad_bytes
        );
        unsigned char* mensaje_a_enviar = mensaje.parse_to_transmission();

        LoRa.beginPacket();
        LoRa.write(mensaje_a_enviar, mensaje.transmission_size);
        LoRa.endPacket(true); // true = async / non-blocking mode

        delete[] mensaje_a_enviar;

        cantidad_pares_restantes -= cantidad_pares_a_enviar;
        cantidad_pares_enviados_totales += cantidad_pares_enviados;
        if (!esperar_ack(receptor, nonce)) return false;
    }
    return true;
}

bool Router::enviar_acks_mensajes(uint16_t receptor) {
    std::vector<uint64_t> acks = buffer.obtener_acks();
    std::vector<Mensaje_ack_mensaje> mensajes = Mensaje_ack_mensaje::crear_mensajes(id, receptor, 0, acks);
    uint16_t nonce;

    for (std::vector<Mensaje_ack_mensaje>::iterator mensaje = mensajes.begin(); mensaje != mensajes.end(); mensaje++) {
        nonce = get_update_nonce();
        mensaje->setNonce(nonce);
        unsigned char* mensaje_a_enviar = mensaje->parse_to_transmission();

        if (!se_puede_transmitir_LoRa()) return false;
        LoRa.beginPacket();
        LoRa.write(mensaje_a_enviar, mensaje->transmission_size);
        LoRa.endPacket(true); // true = async / non-blocking mode

        if (!esperar_ack(receptor, nonce)) return false;
    }

    return true;
}