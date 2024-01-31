#ifndef BUFFER_TEXTOS_HPP
#define BUFFER_TEXTOS_HPP

#include <texto/texto.hpp>
#include <router/mapa/mapa.hpp>
#include <vector>
#include <unordered_set>
#include <cstdint>



class Buffer_textos {
private:
    std::vector<Texto> arreglo_textos_threshold_saltos;
    std::vector<Texto> arreglo_textos_mas_de_threshold_saltos;
    std::unordered_set<uint64_t> acks;
    std::unordered_set<uint64_t> textos_ya_vistos;

    Mapa mapa;

    uint16_t id;
    uint8_t saltos_threshold = 1;

    bool _comparador_textos_saltos(const Texto& a, const Texto& b);
    bool texto_en_acks(const Texto& texto);
    bool texto_en_ya_vistos(const Texto& texto) const;
    bool texto_en_ya_vistos(const uint64_t hash) const;
    float obtener_probabilidad_entrega(Texto& texto);
    static uint64_t obtener_hash_texto(Texto& texto);

    unsigned insertar_en_arr_bajo_thresold(Texto& texto);
    unsigned insertar_en_arr_mas_de_thresold(Texto& texto);

    void agregar_a_ya_vistos(uint64_t hash);
    void agregar_a_ya_vistos(const Texto& texto);
    void agregar_a_ya_vistos(const std::vector<uint64_t>& lista_hashes);
    void agregar_a_ya_vistos(const std::vector<Texto>& textos);

public:
    Buffer_textos(uint16_t _id);
    Buffer_textos();
    ~Buffer_textos();

    unsigned size();
    bool hay_mensajes_bajo_threshold();
    bool hay_mensajes_sobre_threshold();

    void agregar_ack(uint64_t ack);
    void agregar_ack(const std::vector<uint64_t>& lista_acks);
    void agregar_ack(Texto* textos, unsigned cantidad_textos);
    void agregar_ack(const std::vector<Texto>& textos);
    void agregar_texto(const Texto& texto);
    void agregar_texto(Texto* textos, unsigned cantidad_textos);
    void agregar_texto(const std::vector<Texto>& textos);
    std::vector<Texto> obtener_textos_destinatario(uint16_t id_destinatario);

    void eliminar_texto();
    void eliminar_textos(std::vector<Texto> textos);

    std::vector<Texto> obtener_textos_bajo_threshold(long long int excluir_id = -1, std::vector<uint64_t>* excepciones = nullptr);
    std::vector<Texto> obtener_textos_sobre_threshold(long long int excluir_id = -1, std::vector<uint64_t>* excepciones = nullptr);

    std::vector<par_costo_id> obtener_vector_probabilidad() const;
    std::vector<uint64_t> obtener_acks();

    void agregar_probabilidades(uint16_t origen, std::vector<par_costo_id>& pares);
    void actualizar_propias_probabilidades(uint16_t nodo_visto);
    void actualizar_propias_probabilidades(uint16_t* nodos_vistos, uint16_t cant_nodos);
    void print() const;

    unsigned get_size_vector_probabilidad();

    void eliminar_textos_sobre_threshold();

    void print_mapa() const;

    std::vector<uint64_t> obtener_textos_ya_vistos(long long excluir_id = -1) const;
};

#endif