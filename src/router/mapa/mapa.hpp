#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>
#include <queue>

struct arco_dirigido {
    uint16_t destino;
    float peso;
};

class Mapa {
private:
    uint16_t id;
    //las llaves son nodos fuente, los valores otros map cuyas llaves son nodos destino cuyo valor es el peso del arco
    std::unordered_map<uint16_t, std::unordered_map<uint16_t, float>> grafo;
    std::unordered_map<uint16_t, float> tabla_distancias_desde_inicio;

public:
    Mapa(uint16_t _id);
    Mapa();
    ~Mapa();
    void dijkstra();
    void actualizar_probabilidades(uint16_t origen, std::vector<par_costo_id>& pares);
    void actualizar_propias_probabilidades(uint16_t nodo_visto);
    void actualizar_propias_probabilidades(uint16_t* nodos_vistos, uint16_t cant_nodos);
    void add_node(uint16_t nodo, float peso, std::vector<par_costo_id> vecinos);

    unsigned char* obtener_vector_probabilidad();
    float costo(uint16_t destino);
};