#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <texto/texto.hpp>
#include <mensaje/ack.hpp>
#include <buffer_textos/buffer_textos.hpp>
#include <cstdint>
#include <unordered_map>
#include <vector>


class Router {
private:
    Buffer_textos buffer;
    std::vector<Texto> mis_mensajes;
    Mensaje beacon_signal;

    uint16_t id;
    uint16_t nonce_actual;

    uint32_t ttr;
    std::unordered_set<uint64_t> ya_vistos;
    unsigned static const max_size_ultimos_nonce_beacon = 3;
    unsigned size_ultimos_nonce_beacon = 0;
    unsigned cabeza_ultimos_nonce_beacon = max_size_ultimos_nonce_beacon - 1;
    uint16_t* ultimos_nonce_beacon;

    uint16_t get_update_nonce();
    bool agregar_a_mis_mensajes(const Texto& texto);
    bool agregar_a_mis_mensajes(const std::vector<Texto>& textos);
    bool agregar_a_acks(const Texto& texto);
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

    bool enviar_mensaje_texto_maxprop(uint16_t receptor, unsigned periodos_espera = 3, std::vector<uint64_t>* excepciones = nullptr);
    bool enviar_mensaje_texto_ttr(uint16_t receptor, unsigned periodos_espera = 3, std::vector<uint64_t>* excepciones = nullptr);
    bool recibir_mensaje(unsigned periodos_espera = 3, int tipo_mensaje = -1, int emisor_esperado = -1);
    bool emitir_beacon_signal();
    bool emitir_ack_comunicacion(uint16_t receptor, uint16_t nonce_original, uint16_t* nonce_usado = nullptr);
    bool enviar_todos_a_destinatario(uint16_t destinatario);
    bool enviar_vectores_de_probabilidad(uint16_t receptor);
    bool enviar_acks_mensajes(uint16_t receptor);

    void iniciar_comunicacion(uint16_t receptor);
    void recibir_comunicacion(uint16_t receptor);
    void agregar_texto(std::vector<Texto>& textos);
    void agregar_texto(Texto& textos);

    void set_ttr(uint32_t _ttr);
    void update_ttr(uint32_t segundos_transcurridos);
    void print_buffer() const;
    void print_mapa() const;
    void print_mis_mensajes() const;

    uint64_t obtener_hash_texto(const Texto& texto);
    void liberar_memoria();

    unsigned get_cantidad_ya_vistos() const;
    uint16_t get_nonce_beacon_signal() const;
    uint16_t get_beacon_nonce() const;
    uint16_t get_id() const;

    bool enviar_mensaje(const Mensaje& mensaje, unsigned intentos = 3);
    bool confirmar_con_ultimos_beacons(const Mensaje_ack_comunicacion& ack_com) const;
    bool emitir_ack_comunicacion_hasta_respuesta(uint16_t receptor, uint16_t nonce_original, uint8_t tipo_payload, unsigned intentos = 3);

    bool enviar_textos_ya_vistos(uint16_t receptor, unsigned periodos_espera = 3);
};


#endif