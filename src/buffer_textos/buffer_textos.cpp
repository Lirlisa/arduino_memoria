#include <buffer_textos/buffer_textos.hpp>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <Arduino.h>

Buffer_textos::Buffer_textos(uint16_t _id) : mapa(_id) {
    id = _id;
}

Buffer_textos::Buffer_textos() {}

Buffer_textos::~Buffer_textos() {
    Serial.println("Eliminado Buffer");
}

bool Buffer_textos::_comparador_textos_saltos(const Texto& a, const Texto& b) {
    return a.saltos < b.saltos;
}

unsigned Buffer_textos::size() {
    return arreglo_textos_threshold_saltos.size() + arreglo_textos_mas_de_threshold_saltos.size();
}

void Buffer_textos::eliminar_texto() {
    if (arreglo_textos_mas_de_threshold_saltos.size() > 0)
        arreglo_textos_mas_de_threshold_saltos.erase(arreglo_textos_mas_de_threshold_saltos.end());
    else if (arreglo_textos_threshold_saltos.size() > 0)
        arreglo_textos_threshold_saltos.erase(arreglo_textos_threshold_saltos.end());
}

bool Buffer_textos::texto_en_acks(Texto& texto) {
    uint64_t hash = texto.hash();
    return acks.find(hash) == acks.end();
}

void Buffer_textos::agregar_ack(uint64_t ack) {
    if (acks.find(ack) != acks.end())
        acks.insert(ack);

    auto se_remueve = [ack](Texto& texto) { return texto.hash() == ack; };

    auto it = std::remove_if(arreglo_textos_threshold_saltos.begin(), arreglo_textos_threshold_saltos.end(), se_remueve);
    arreglo_textos_threshold_saltos.erase(it, arreglo_textos_threshold_saltos.end());

    it = std::remove_if(arreglo_textos_mas_de_threshold_saltos.begin(), arreglo_textos_mas_de_threshold_saltos.end(), se_remueve);
    arreglo_textos_mas_de_threshold_saltos.erase(it, arreglo_textos_mas_de_threshold_saltos.end());
}

void Buffer_textos::agregar_ack(std::vector<uint64_t>& lista_acks) {
    for (std::vector<uint64_t>::iterator ack = lista_acks.begin(); ack != lista_acks.end(); ack++) {
        if (acks.find(*ack) != acks.end())
            acks.insert(*ack);
    }

    auto se_remueve = [this](Texto& texto) { return this->acks.find(texto.hash()) != this->acks.end();};

    auto it = std::remove_if(arreglo_textos_threshold_saltos.begin(), arreglo_textos_threshold_saltos.end(), se_remueve);
    arreglo_textos_threshold_saltos.erase(it, arreglo_textos_threshold_saltos.end());

    it = std::remove_if(arreglo_textos_mas_de_threshold_saltos.begin(), arreglo_textos_mas_de_threshold_saltos.end(), se_remueve);
    arreglo_textos_mas_de_threshold_saltos.erase(it, arreglo_textos_mas_de_threshold_saltos.end());
}

void Buffer_textos::agregar_ack(Texto* textos, unsigned cantidad_textos) {
    uint64_t hash;
    for (unsigned i = 0; i < cantidad_textos; i++) {
        hash = textos[i].hash();
        agregar_ack(hash);
    }
}

void Buffer_textos::agregar_ack(std::vector<Texto>& textos) {
    uint64_t hash;
    for (std::vector<Texto>::iterator texto = textos.begin(); texto != textos.end(); texto++) {
        hash = texto->hash();
        if (acks.find(hash) != acks.end())
            acks.insert(hash);
    }

    auto se_remueve = [this](Texto& texto) { return this->acks.find(texto.hash()) != this->acks.end(); };

    auto it = std::remove_if(arreglo_textos_threshold_saltos.begin(), arreglo_textos_threshold_saltos.end(), se_remueve);
    arreglo_textos_threshold_saltos.erase(it, arreglo_textos_threshold_saltos.end());

    it = std::remove_if(arreglo_textos_mas_de_threshold_saltos.begin(), arreglo_textos_mas_de_threshold_saltos.end(), se_remueve);
    arreglo_textos_mas_de_threshold_saltos.erase(it, arreglo_textos_mas_de_threshold_saltos.end());
}

unsigned Buffer_textos::insertar_en_arr_bajo_thresold(Texto texto, unsigned revisar_desde) {
    unsigned pos_insert = revisar_desde;
    while (texto.saltos >= arreglo_textos_threshold_saltos.at(pos_insert).saltos) pos_insert++;
    arreglo_textos_threshold_saltos.insert(arreglo_textos_threshold_saltos.begin() + pos_insert, texto);

    return pos_insert;
}
unsigned Buffer_textos::insertar_en_arr_mas_de_thresold(Texto texto, unsigned revisar_desde) {
    unsigned pos_insert = revisar_desde;
    float prob = obtener_probabilidad_entrega(texto);
    while (prob >= obtener_probabilidad_entrega(arreglo_textos_mas_de_threshold_saltos.at(pos_insert))) pos_insert++;
    arreglo_textos_mas_de_threshold_saltos.insert(arreglo_textos_mas_de_threshold_saltos.begin() + pos_insert, texto);

    return pos_insert;
}

void Buffer_textos::agregar_texto(Texto texto) {
    texto.saltos += 1;
    if (texto_en_acks(texto) || fue_visto(texto)) return;
    agregar_a_vistos(texto);
    if (texto.saltos <= saltos_threshold)
        insertar_en_arr_bajo_thresold(texto);
    else
        insertar_en_arr_mas_de_thresold(texto);
}

void Buffer_textos::agregar_texto(Texto* textos, unsigned cantidad_textos) {
    unsigned pos_bajo_threshold = 0, pos_sobre_threshold = 0;
    for (unsigned i = 0; i < cantidad_textos; i++) {
        Texto texto = textos[i];
        texto.saltos += 1;
        if (texto_en_acks(texto) || fue_visto(texto)) continue;
        agregar_a_vistos(texto);
        if (texto.saltos <= saltos_threshold)
            pos_bajo_threshold = insertar_en_arr_bajo_thresold(texto, pos_bajo_threshold);
        else
            pos_sobre_threshold = insertar_en_arr_mas_de_thresold(texto, pos_sobre_threshold);
    }
}

void Buffer_textos::agregar_texto(std::vector<Texto>& textos) {
    unsigned pos_bajo_threshold = 0, pos_sobre_threshold = 0;
    for (std::vector<Texto>::iterator i = textos.begin(); i != textos.end(); i++) {
        Texto texto = *i;
        texto.saltos += 1;
        if (texto_en_acks(texto) || fue_visto(texto)) continue;
        agregar_a_vistos(texto);
        if (texto.saltos <= saltos_threshold)
            pos_bajo_threshold = insertar_en_arr_bajo_thresold(texto, pos_bajo_threshold);
        else
            pos_sobre_threshold = insertar_en_arr_mas_de_thresold(texto, pos_sobre_threshold);
    }
}

/*
@brief entrega probabilidad de entrega del
*/
float Buffer_textos::obtener_probabilidad_entrega(Texto& texto) {
    return mapa.costo(texto.destinatario);
}

bool Buffer_textos::hay_mensajes_bajo_threshold() {
    return arreglo_textos_threshold_saltos.size() > 0;
}

bool Buffer_textos::hay_mensajes_sobre_threshold() {
    return arreglo_textos_mas_de_threshold_saltos.size() > 0;
}

/*
@brief Entrega todos los textos para un destinatario.
*/
std::vector<Texto> Buffer_textos::obtener_textos_destinatario(uint16_t id_destinatario) {
    std::vector<Texto> textos;
    for (std::vector<Texto>::iterator texto = arreglo_textos_threshold_saltos.begin(); texto != arreglo_textos_threshold_saltos.end(); texto++) {
        if (texto->destinatario == id_destinatario) textos.push_back(*texto);
    }
    for (std::vector<Texto>::iterator texto = arreglo_textos_mas_de_threshold_saltos.begin(); texto != arreglo_textos_mas_de_threshold_saltos.end(); texto++) {
        if (texto->destinatario == id_destinatario) textos.push_back(*texto);
    }
    return textos;
}

uint64_t Buffer_textos::obtener_hash_texto(Texto& texto) {
    return texto.hash();
}

void Buffer_textos::eliminar_textos(std::vector<Texto> textos) {
    std::unordered_set<uint64_t> set(textos.size());
    std::transform(textos.begin(), textos.end(), std::inserter(set, set.begin()), obtener_hash_texto);

    auto se_remueve = [&set](Texto text) { return set.find(text.hash()) != set.end(); };
    auto it = std::remove_if(arreglo_textos_threshold_saltos.begin(), arreglo_textos_threshold_saltos.end(), se_remueve);
    arreglo_textos_threshold_saltos.erase(it, arreglo_textos_threshold_saltos.end());

    it = std::remove_if(arreglo_textos_mas_de_threshold_saltos.begin(), arreglo_textos_mas_de_threshold_saltos.end(), se_remueve);
    arreglo_textos_mas_de_threshold_saltos.erase(it, arreglo_textos_mas_de_threshold_saltos.end());
}

std::vector<Texto> Buffer_textos::obtener_textos_bajo_threshold() {
    return arreglo_textos_threshold_saltos;
}


std::vector<Texto> Buffer_textos::obtener_textos_sobre_threshold() {
    return arreglo_textos_mas_de_threshold_saltos;
}

unsigned char* Buffer_textos::obtener_vector_probabilidad() {
    return mapa.obtener_vector_probabilidad();
}

std::vector<uint64_t> Buffer_textos::obtener_acks() {
    return std::vector<uint64_t>(acks.begin(), acks.end());
}

void Buffer_textos::agregar_a_vistos(uint64_t hash) {
    if (ya_vistos.find(hash) != ya_vistos.end())
        ya_vistos.insert(hash);
}
void Buffer_textos::agregar_a_vistos(Texto& texto) {
    uint64_t hash = texto.hash();
    agregar_a_vistos(hash);
}
void Buffer_textos::agregar_a_vistos(std::vector<Texto>& textos) {
    uint64_t hash;
    for (std::vector<Texto>::iterator texto = textos.begin(); texto != textos.end(); texto++) {
        hash = texto->hash();
        if (ya_vistos.find(hash) != ya_vistos.end())
            ya_vistos.insert(hash);
    }
}

bool Buffer_textos::fue_visto(uint64_t hash) {
    return ya_vistos.find(hash) != ya_vistos.end();
}
bool Buffer_textos::fue_visto(Texto& texto) {
    return ya_vistos.find(texto.hash()) != ya_vistos.end();
}

void Buffer_textos::agregar_probabilidades(uint16_t origen, std::vector<par_costo_id>& pares) {
    mapa.actualizar_probabilidades(origen, pares);
}

void Buffer_textos::actualizar_propias_probabilidades(uint16_t nodo_visto) {
    mapa.actualizar_propias_probabilidades(nodo_visto);
}
void Buffer_textos::actualizar_propias_probabilidades(uint16_t* nodos_vistos, uint16_t cant_nodos) {
    mapa.actualizar_propias_probabilidades(nodos_vistos, cant_nodos);
}