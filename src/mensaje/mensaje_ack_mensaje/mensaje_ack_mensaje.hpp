#ifndef MENSAJE_ACK_MENSAJE_HPP
#define MENSAJE_ACK_MENSAJE_HPP

#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/ack.hpp>
#include <cstdint>
#include <vector>

class Mensaje_ack_mensaje : public Mensaje {
private:
    static const uint8_t ack_size = 6;

public:
    Mensaje_ack_mensaje(
        uint16_t _emisor, uint16_t _receptor, uint16_t _nonce,
        unsigned char* _payload, int _payload_size
    );
    Mensaje_ack_mensaje(const Mensaje_ack_mensaje& origen);
    Mensaje_ack_mensaje(Mensaje const& origen);

    ~Mensaje_ack_mensaje();


    static std::vector<Mensaje_ack_mensaje> crear_mensajes(
        uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, std::vector<uint64_t>& acks
    );

    std::vector<uint64_t> obtener_acks();
};

#endif