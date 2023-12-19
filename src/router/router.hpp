#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <mensaje/ack.hpp>
#include <buffer_textos/buffer_textos.hpp>
#include <cstdint>
#include <unordered_map>


class Router {
private:
    Buffer_textos buffer;
    Texto* mis_mensajes;
    std::vector<Texto> en_espera_ack;
    long int conexiones_totales = 0;
    long long int total_bytes_transferidos = 0;

    Mensaje mensaje_pendiente;
    bool hay_mensaje_pendiente = false;
    uint16_t id;
    unsigned int mensajes_enviados;
    uint16_t nonce_actual = 0;

    unsigned int capacidad_mis_mensajes;
    unsigned int size_mis_mensajes;
    uint32_t ttr;

    bool posicion_valida(unsigned int pos);
    bool borrar_mensaje_en_pos(unsigned int pos);
    bool esperar_ack(uint16_t id_nodo, uint16_t nonce, int periodos_espera = 3);
    uint16_t get_update_nonce();
    bool agregar_a_mis_mensajes(Texto& texto);
    bool realocar_mis_mensajes();
    bool hay_espacio_en_mis_mensajes();
    bool agregar_a_acks(Texto& texto);
    bool se_puede_transmitir_LoRa(int periodos_espera = 3);
    bool hay_paquete_LoRa(int periodos_espera = 3);

public:
    Router(uint16_t _id, uint32_t _ttr, unsigned int initial_capacity);
    ~Router();

    const static uint16_t BROADCAST_CHANNEL_ID = 0xffff;

    bool enviar_mensaje_texto_maxprop(uint16_t receptor, int periodos_espera = 3);
    bool enviar_mensaje_texto_ttr(uint16_t receptor, int periodos_espera = 3);
    bool enviar_mensaje_texto(Mensaje_texto& msg, int periodos_espera = 3);
    bool recibir_mensaje();
    bool guardar_mensaje(Texto& texto);
    bool emitir_beacon_signal();
    bool emitir_ack_comunicacion(uint16_t receptor_destinatario, uint16_t nonce_original);
    bool enviar_todos_a_destinatario(uint16_t destinatario);
    bool enviar_vectores_de_probabilidad(uint16_t receptor);
    bool enviar_acks_mensajes(uint16_t receptor);
};


#endif