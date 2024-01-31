#include <router/mapa/mapa.hpp>
#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <cstdint>
#include <unordered_map>
#include <limits>
#include <cstring>
#include <unordered_set>
#include <vector>
#include <queue>
#include <Arduino.h>


Mapa::Mapa(uint16_t _id) {
    Serial.print("Id en mapa: ");
    Serial.println(_id);
    id = _id;
    grafo.insert({ id, {} });
}

Mapa::~Mapa() { }

void Mapa::dijkstra() {
    std::priority_queue<par_costo_id, std::vector<par_costo_id>, std::greater<par_costo_id>> pq;
    tabla_distancias_desde_inicio.clear();
    tabla_distancias_desde_inicio.reserve(grafo.size());

    pq.push(std::make_pair(0, id));
    tabla_distancias_desde_inicio.insert({ id, 0 });

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
                if (tabla_distancias_desde_inicio.find(v) == tabla_distancias_desde_inicio.end())
                    tabla_distancias_desde_inicio.insert({ v, 0 });
                tabla_distancias_desde_inicio[v] = dist_u + weight;
                pq.push(std::make_pair(tabla_distancias_desde_inicio[v], v));
            }
        }
    }
}

void Mapa::actualizar_probabilidades(uint16_t origen, const std::vector<par_costo_id>& pares) {
    if (grafo.find(origen) == grafo.end())
        grafo.insert({ origen, {} });
    for (std::vector<par_costo_id>::const_iterator par = pares.begin(); par != pares.end(); par++)
        grafo[origen].insert({ par->second, par->first });

    dijkstra();
}

void Mapa::actualizar_propias_probabilidades(uint16_t nodo_visto) {
    if (grafo[id].find(nodo_visto) == grafo[id].end())
        grafo[id].insert({ nodo_visto, 0 });
    grafo[id][nodo_visto] += 1;

    float suma = 0;
    for (auto& v : grafo[id])
        suma += v.second;

    for (auto& v : grafo[id])
        grafo[id][v.first] /= suma;

    dijkstra();
}

void Mapa::actualizar_propias_probabilidades(uint16_t* nodos_vistos, uint16_t cant_nodos) {
    for (uint16_t i = 0; i < cant_nodos; i++) {
        if (grafo[id].find(nodos_vistos[i]) == grafo[id].end()) {
            grafo[id].insert({ nodos_vistos[i], 0 });
        }
        grafo[id][nodos_vistos[i]] += 1;
    }

    float suma = 0;
    for (auto& v : grafo[id])
        suma += v.second;

    for (auto& v : grafo[id])
        grafo[id][v.first] /= suma;

    dijkstra();
}

/*
@brief Entrega todos los pares de costo e id de los nodos vecinos para el nodo base.
*/
std::vector<par_costo_id> Mapa::obtener_vector_probabilidad() const {
    std::vector<par_costo_id> pares;
    pares.reserve(grafo.find(id)->second.size());
    for (
        std::unordered_map<uint16_t, float>::const_iterator vecino = grafo.find(id)->second.begin();
        vecino != grafo.find(id)->second.end();
        ++vecino
        ) {
        pares.push_back(std::make_pair(vecino->second, vecino->first));
    }

    return pares;
}

float Mapa::costo(uint16_t destino) {
    return tabla_distancias_desde_inicio.find(destino) == tabla_distancias_desde_inicio.end() ?
        std::numeric_limits<float>::infinity() :
        tabla_distancias_desde_inicio[destino];
}

unsigned Mapa::get_size_vector_probabilidad() const {
    return 2 + grafo.find(id)->second.size() * 6;
}

void Mapa::print() const {
    Serial.println("----- Mapa -----");
    for (
        std::unordered_map<uint16_t, std::unordered_map<uint16_t, float>>::const_iterator nodoA = grafo.begin();
        nodoA != grafo.end();
        nodoA++
        ) {
        for (std::unordered_map<uint16_t, float>::const_iterator nodoB = nodoA->second.begin(); nodoB != nodoA->second.end();nodoB++) {
            Serial.print("Nodo ");
            Serial.print(nodoA->first);
            Serial.print(" -> Nodo ");
            Serial.print(nodoB->first);
            Serial.print(": ");
            Serial.println(nodoB->second);
        }
    }
    Serial.println("----- Fin Mapa -----");
}