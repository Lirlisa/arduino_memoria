#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <texto/texto.hpp>
#include <mensaje/ack.hpp>
#include <buffer_textos/buffer_textos.hpp>
#include <cstdint>
#include <unordered_map>


class Router {
private:
    Buffer_textos buffer;
    Texto* mis_mensajes;
    std::vector<Texto> en_espera_ack;
    Mensaje beacon_signal;
    long int conexiones_totales = 0;
    long long int total_bytes_transferidos = 0;

    uint16_t id;
    unsigned int mensajes_enviados;
    uint16_t nonce_actual;

    unsigned int capacidad_mis_mensajes;
    unsigned int size_mis_mensajes;
    uint32_t ttr;
    std::unordered_set<uint64_t> ya_vistos;

    uint16_t get_update_nonce();
    bool agregar_a_mis_mensajes(Texto& texto);
    bool agregar_a_mis_mensajes(std::vector<Texto>& textos);
    bool realocar_mis_mensajes(unsigned n = 0);
    bool hay_espacio_en_mis_mensajes(unsigned n = 1);
    bool agregar_a_acks(Texto& texto);
    bool hay_paquete_LoRa(unsigned periodos_espera = 3);
    uint64_t obtener_hash_mensaje(uint16_t nonce, uint16_t emisor, uint16_t receptor);
    bool mensaje_ya_visto(Mensaje& mensaje);
    void agregar_a_ya_visto(Mensaje& mensaje);

public:
    Router(uint16_t _id, uint32_t _ttr, unsigned int initial_capacity = 10);
    ~Router();

    const static uint16_t BROADCAST_CHANNEL_ID = 0xffff;
    const static unsigned tiempo_transmision = 219;
    const static unsigned tiempo_max_espera = 5 * tiempo_transmision;
    Mensaje mensaje_pendiente;
    bool hay_mensaje_pendiente = false;

    bool enviar_mensaje_texto_maxprop(uint16_t receptor, unsigned periodos_espera = 3);
    bool enviar_mensaje_texto_ttr(uint16_t receptor, unsigned periodos_espera = 3);
    bool recibir_mensaje(unsigned periodos_espera = 3);
    bool emitir_beacon_signal();
    bool emitir_ack_comunicacion(uint16_t receptor, uint16_t nonce_original);
    bool enviar_todos_a_destinatario(uint16_t destinatario);
    bool enviar_vectores_de_probabilidad(uint16_t receptor);
    bool enviar_acks_mensajes(uint16_t receptor);

    void iniciar_comunicacion(uint16_t receptor);
    void recibir_comunicacion(uint16_t receptor);
    void agregar_texto(std::vector<Texto>& textos);
    void agregar_texto(Texto& textos);

    void set_ttr(uint32_t _ttr);
    void update_ttr(uint32_t segundos_transcurridos);
    void print_buffer();
    void print_mapa();

    uint64_t obtener_hash_texto(const Texto& texto);
    void eliminar_de_ya_vistos(const std::vector<Texto>& textos);
    void liberar_memoria();

    unsigned get_cantidad_ya_vistos() const;
    uint16_t get_nonce_beacon_signal() const;
    uint16_t get_beacon_nonce() const;
    uint16_t get_id() const;

    bool enviar_mensaje(const Mensaje& mensaje, unsigned intentos = 3);
};


#endif