#include <router/mapa/mapa.hpp>
#include <cstdint>
#include <unordered_map>
#include <limits>
#include <cstring>
#include <unordered_set>
#include <vector>
#include <queue>


Mapa::Mapa(uint16_t _id) {
    id = _id;
    grafo[id] = std::unordered_map<uint16_t, float>();
}

void Mapa::dijkstra() {
    std::priority_queue<par_costo_id, std::vector<par_costo_id>, std::greater<par_costo_id>> pq;
    tabla_distancias_desde_inicio = std::unordered_map<uint16_t, float>(grafo.size());

    pq.push(std::make_pair(0, id));
    tabla_distancias_desde_inicio[id] = 0;

    while (!pq.empty()) {
        uint16_t u = pq.top().second;
        pq.pop();

        std::unordered_map<uint16_t, float>::iterator i;
        for (i = grafo[u].begin(); i != grafo[u].end(); ++i) {
            uint16_t v = i->first;
            float weight = i->second;

            float dist_v = tabla_distancias_desde_inicio.find(v) == tabla_distancias_desde_inicio.end() ?
                std::numeric_limits<float>::infinity() :
                tabla_distancias_desde_inicio[v];
            float dist_u = tabla_distancias_desde_inicio.find(u) == tabla_distancias_desde_inicio.end() ?
                std::numeric_limits<float>::infinity() :
                tabla_distancias_desde_inicio[u];

            if (dist_v > dist_u + weight) {
                tabla_distancias_desde_inicio[v] = dist_u + weight;
                pq.push(std::make_pair(tabla_distancias_desde_inicio[v], v));
            }
        }
    }
}

void Mapa::actualizar_probabilidades(uint16_t origen, arco_dirigido* pesos, uint16_t cant_pesos) {
    for (uint16_t i = 0; i < cant_pesos; i++) {
        grafo[origen][pesos[i].destino] = pesos[i].peso;
    }
}

void Mapa::actualizar_propias_probabilidades(uint16_t nodo_visto) {
    if (grafo[id].find(nodo_visto) == grafo[id].end())
        grafo[id][nodo_visto] = 0;
    grafo[id][nodo_visto] += 1;

    float suma = 0;
    for (auto& v : grafo[id])
        suma += v.second;

    for (auto& v : grafo[id])
        grafo[id][v.first] /= suma;
}

void Mapa::actualizar_propias_probabilidades(uint16_t* nodos_vistos, uint16_t cant_nodos) {
    for (uint16_t i = 0; i < cant_nodos; i++) {
        if (grafo[id].find(nodos_vistos[i]) == grafo[id].end()) {
            grafo[id][nodos_vistos[i]] = 0;
        }
        grafo[id][nodos_vistos[i]] += 1;
    }

    float suma = 0;
    for (auto& v : grafo[id])
        suma += v.second;

    for (auto& v : grafo[id])
        grafo[id][v.first] /= suma;
}

/*
Entrega el vector de probabilidades del nodo, es deber del caller liberar la memoria
*/
unsigned char* Mapa::obtener_vector_probabilidad() {
    unsigned char* data = new unsigned char[2 + grafo[id].size() * 6];
    uint16_t _size = grafo[id].size();
    memcpy(data, &_size, 2);
    uint16_t i = 0;
    for (auto& v : grafo[id]) {
        memcpy(data + 2 + i * 6, &(v.first), 2);
        memcpy(data + 2 + i * 6 + 2, &(v.second), 4);
        i++;
    }
    return data;
}

float Mapa::costo(uint16_t destino) {
    return tabla_distancias_desde_inicio.find(destino) == tabla_distancias_desde_inicio.end() ?
        std::numeric_limits<float>::infinity() :
        tabla_distancias_desde_inicio[destino];
}

void Mapa::add_node(uint16_t nodo, float peso, std::vector<par_costo_id> vecinos) {
    if (grafo.find(nodo) == grafo.end())
        grafo[nodo] = std::unordered_map<uint16_t, float>();

    for (std::vector<par_costo_id>::iterator vecino = vecinos.begin(); vecino != vecinos.end(); vecino++) {
        if (grafo.find(vecino->second) == grafo.end())
            grafo[vecino->second] = std::unordered_map<uint16_t, float>();

        grafo[nodo][vecino->second] = vecino->first;
    }

    dijkstra();
}