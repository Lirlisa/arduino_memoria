#ifndef MENSAJE_ACK_COMUNICACION_HPP
#define MENSAJE_ACK_COMUNICACION_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <cstdint>

class Mensaje_ack_comunicacion : public Mensaje {
private:
    uint16_t nonce_mensaje_original;

    const static int raw_message_max_size = 15;
    const static int payload_max_size = 6;
public:
    Mensaje_ack_comunicacion(uint16_t _emisor, uint16_t _receptor, uint16_t _nonce, uint16_t nonce_msj_original);
    Mensaje_ack_comunicacion(const Mensaje_ack_comunicacion& original);
    Mensaje_ack_comunicacion(Mensaje const& origen);

    bool confirmar_ack(uint16_t nonce_original, uint16_t emisor_original, uint16_t receptor_original);
    void print();
};

#endif