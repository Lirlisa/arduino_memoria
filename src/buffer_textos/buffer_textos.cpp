#include <buffer_textos/buffer_textos.hpp>
#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <Arduino.h>

Buffer_textos::Buffer_textos(uint16_t _id) : mapa(_id) {
    id = _id;
}

Buffer_textos::~Buffer_textos() {
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

bool Buffer_textos::texto_en_acks(const Texto& texto) {
    uint64_t hash = texto.hash();
    return acks.find(hash) != acks.end();
}

bool Buffer_textos::texto_en_ya_vistos(const Texto& texto) const {
    uint64_t hash = texto.hash();
    return texto_en_ya_vistos(hash);
}

bool Buffer_textos::texto_en_ya_vistos(const uint64_t hash) const {
    return textos_ya_vistos.find(hash) != textos_ya_vistos.end();
}

void Buffer_textos::agregar_ack(uint64_t ack) {
    if (acks.find(ack) == acks.end())
        acks.insert(ack);

    auto se_remueve = [ack](Texto& texto) { return texto.hash() == ack; };

    auto it = std::remove_if(arreglo_textos_threshold_saltos.begin(), arreglo_textos_threshold_saltos.end(), se_remueve);
    arreglo_textos_threshold_saltos.erase(it, arreglo_textos_threshold_saltos.end());

    it = std::remove_if(arreglo_textos_mas_de_threshold_saltos.begin(), arreglo_textos_mas_de_threshold_saltos.end(), se_remueve);
    arreglo_textos_mas_de_threshold_saltos.erase(it, arreglo_textos_mas_de_threshold_saltos.end());
}

void Buffer_textos::agregar_ack(const std::vector<uint64_t>& lista_acks) {
    for (std::vector<uint64_t>::const_iterator ack = lista_acks.begin(); ack != lista_acks.end(); ack++) {
        if (acks.find(*ack) == acks.end())
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

void Buffer_textos::agregar_ack(const std::vector<Texto>& textos) {
    uint64_t hash;
    for (std::vector<Texto>::const_iterator texto = textos.begin(); texto != textos.end(); texto++) {
        hash = texto->hash();
        if (acks.find(hash) == acks.end())
            acks.insert(hash);
    }

    auto se_remueve = [this](Texto& texto) { return this->acks.find(texto.hash()) != this->acks.end(); };

    auto it = std::remove_if(arreglo_textos_threshold_saltos.begin(), arreglo_textos_threshold_saltos.end(), se_remueve);
    arreglo_textos_threshold_saltos.erase(it, arreglo_textos_threshold_saltos.end());

    it = std::remove_if(arreglo_textos_mas_de_threshold_saltos.begin(), arreglo_textos_mas_de_threshold_saltos.end(), se_remueve);
    arreglo_textos_mas_de_threshold_saltos.erase(it, arreglo_textos_mas_de_threshold_saltos.end());
}

unsigned Buffer_textos::insertar_en_arr_bajo_thresold(Texto& texto) {
    unsigned i = 0;
    while (i < arreglo_textos_threshold_saltos.size()) {
        if (texto.saltos < arreglo_textos_threshold_saltos.at(i).saltos) break;
        i++;
    }
    arreglo_textos_threshold_saltos.insert(arreglo_textos_threshold_saltos.begin() + i, texto);
    return i;
}

unsigned Buffer_textos::insertar_en_arr_mas_de_thresold(Texto& texto) {
    float prob = obtener_probabilidad_entrega(texto);
    unsigned i = 0;
    while (i < arreglo_textos_mas_de_threshold_saltos.size()) {
        if (prob >= obtener_probabilidad_entrega(arreglo_textos_mas_de_threshold_saltos.at(i))) break;
        i++;
    };
    arreglo_textos_mas_de_threshold_saltos.insert(arreglo_textos_mas_de_threshold_saltos.begin() + i, texto);
    return i;
}

void Buffer_textos::agregar_texto(const Texto& texto) {
    if (texto_en_acks(texto)) return;
    if (texto_en_ya_vistos(texto)) return;
    Texto nuevo_texto(texto);
    nuevo_texto.saltos += 1;
    agregar_a_ya_vistos(nuevo_texto);
    if (nuevo_texto.saltos <= saltos_threshold)
        insertar_en_arr_bajo_thresold(nuevo_texto);
    else
        insertar_en_arr_mas_de_thresold(nuevo_texto);
}

void Buffer_textos::agregar_texto(Texto* textos, unsigned cantidad_textos) {
    for (unsigned i = 0; i < cantidad_textos; i++) {
        if (texto_en_ya_vistos(textos[i])) continue;
        if (texto_en_acks(textos[i])) continue;
        Texto texto = textos[i];
        texto.saltos += 1;
        agregar_a_ya_vistos(texto);
        if (texto.saltos <= saltos_threshold)
            insertar_en_arr_bajo_thresold(texto);
        else
            insertar_en_arr_mas_de_thresold(texto);
    }
}

void Buffer_textos::agregar_texto(const std::vector<Texto>& textos) {
    Serial.println("Flag Buffer_textos::agregar_texto 1");
    Texto texto;
    Serial.println("Flag Buffer_textos::agregar_texto 2");
    for (std::vector<Texto>::const_iterator _texto = textos.begin(); _texto != textos.end(); _texto++) {
        Serial.println("Flag Buffer_textos::agregar_texto 3");
        if (texto_en_acks(*_texto)) continue;
        Serial.println("Flag Buffer_textos::agregar_texto 4");
        if (texto_en_ya_vistos(*_texto)) continue;
        Serial.println("Flag Buffer_textos::agregar_texto 5");
        _texto->print();
        texto = Texto(*_texto);
        Serial.println("Flag Buffer_textos::agregar_texto 6");
        texto.saltos += 1;
        Serial.println("Flag Buffer_textos::agregar_texto 7");
        agregar_a_ya_vistos(texto);
        Serial.println("Flag Buffer_textos::agregar_texto 8");
        if (texto.saltos <= saltos_threshold) {
            insertar_en_arr_bajo_thresold(texto);
            Serial.println("Flag Buffer_textos::agregar_texto 9");
        }
        else {
            insertar_en_arr_mas_de_thresold(texto);
            Serial.println("Flag Buffer_textos::agregar_texto 10");
        }
        Serial.println("Flag Buffer_textos::agregar_texto 11");
    }
    Serial.println("Flag Buffer_textos::agregar_texto 12");
}

/*
@brief entrega probabilidad de entregar al nodo destino del texto dado.
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

std::vector<Texto> Buffer_textos::obtener_textos_bajo_threshold(long long int excluir_id, std::vector<uint64_t>* excepciones) {
    if (excluir_id < 0 && excepciones == nullptr) return arreglo_textos_threshold_saltos;

    std::vector<Texto> elegidos;
    elegidos.reserve(arreglo_textos_threshold_saltos.size());
    std::unordered_set<int> set_evitar(excepciones->begin(), excepciones->end());
    auto diferente_de = [excluir_id, &set_evitar](Texto& texto) {
        return excluir_id != texto.creador && set_evitar.find(texto.hash()) != set_evitar.end();
        };

    std::copy_if(
        arreglo_textos_threshold_saltos.begin(),
        arreglo_textos_threshold_saltos.end(),
        std::back_inserter(elegidos),
        diferente_de
    );

    return elegidos;
}

std::vector<Texto> Buffer_textos::obtener_textos_sobre_threshold(long long int excluir_id, std::vector<uint64_t>* excepciones) {
    if (excluir_id < 0 && excepciones == nullptr) return arreglo_textos_mas_de_threshold_saltos;

    std::vector<Texto> elegidos;
    elegidos.reserve(arreglo_textos_mas_de_threshold_saltos.size());
    std::unordered_set<int> set_evitar(excepciones->begin(), excepciones->end());
    auto diferente_de = [excluir_id, &set_evitar](Texto& texto) {
        return texto.creador != (uint16_t)excluir_id && set_evitar.find(texto.hash()) != set_evitar.end();
        };

    std::copy_if(
        arreglo_textos_mas_de_threshold_saltos.begin(),
        arreglo_textos_mas_de_threshold_saltos.end(),
        std::back_inserter(elegidos),
        diferente_de
    );

    return elegidos;
}

std::vector<par_costo_id> Buffer_textos::obtener_vector_probabilidad() const {
    return mapa.obtener_vector_probabilidad();
}

std::vector<uint64_t> Buffer_textos::obtener_acks() {
    return std::vector<uint64_t>(acks.begin(), acks.end());
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

void Buffer_textos::print() const {
    Serial.println("Mensajes en buffer:");
    Serial.print("Cantidad mensajes en arreglo_textos_threshold_saltos: ");
    Serial.println(arreglo_textos_threshold_saltos.size());
    std::vector<Texto>::const_iterator texto;
    for (texto = arreglo_textos_threshold_saltos.begin(); texto != arreglo_textos_threshold_saltos.end(); texto++) {
        texto->print();
        Serial.println("--------");
    }
    Serial.print("Cantidad mensajes en arreglo_textos_mas_de_threshold_saltos: ");
    Serial.println(arreglo_textos_mas_de_threshold_saltos.size());
    for (texto = arreglo_textos_mas_de_threshold_saltos.begin(); texto != arreglo_textos_mas_de_threshold_saltos.end(); texto++) {
        texto->print();
        Serial.println("--------");
    }
}

unsigned Buffer_textos::get_size_vector_probabilidad() {
    return mapa.get_size_vector_probabilidad();
}

void Buffer_textos::eliminar_textos_sobre_threshold() {
    arreglo_textos_mas_de_threshold_saltos.clear();
}

void Buffer_textos::print_mapa() const {
    mapa.print();
}

void Buffer_textos::agregar_a_ya_vistos(uint64_t hash) {
    if (!texto_en_ya_vistos(hash))
        textos_ya_vistos.insert(hash);
}

void Buffer_textos::agregar_a_ya_vistos(const Texto& texto) {
    if (!texto_en_ya_vistos(texto))
        textos_ya_vistos.insert(texto.hash());
}

void Buffer_textos::agregar_a_ya_vistos(const std::vector<uint64_t>& lista_hashes) {
    for (std::vector<uint64_t>::const_iterator hash = lista_hashes.begin(); hash != lista_hashes.end(); hash++) {
        agregar_a_ya_vistos(*hash);
    }

}

void Buffer_textos::agregar_a_ya_vistos(const std::vector<Texto>& textos) {
    for (std::vector<Texto>::const_iterator texto = textos.begin(); texto != textos.end(); texto++) {
        if (!texto_en_ya_vistos(*texto))
            textos_ya_vistos.insert(texto->hash());
    }
}

std::vector<uint64_t> Buffer_textos::obtener_textos_ya_vistos(long long excluir_id) const {
    std::vector<uint64_t> hashes;
    uint16_t destinatario;
    for (std::unordered_set<uint64_t>::const_iterator hash = textos_ya_vistos.begin(); hash != textos_ya_vistos.end(); hash++) {
        if (excluir_id >= 0) {
            std::memcpy(&destinatario, (&(*hash)) + 4, 2);
            if (destinatario != excluir_id)
                hashes.push_back(*hash);
        }
        else
            hashes.push_back(*hash);
    }
    return hashes;
}