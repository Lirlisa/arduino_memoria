#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>

class Router {
private:
    Mensaje_texto** buffer;
    unsigned int size;
    unsigned int mensajes_enviados;
    unsigned int capacidad;
public:
    Router(int initial_capacity = 10);
    ~Router();
    bool enviar_mensaje(int receptor);
    bool recibir_mensaje();
    bool guardar_mensaje(Mensaje_texto* msg);
    bool realocar_buffer();
    bool hay_espacio();
};


#endif