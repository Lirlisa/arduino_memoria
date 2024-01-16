#ifndef MENSAJE_HPP
#define MENSAJE_HPP
#include <cstdint>


class Mensaje {
protected:
    uint32_t ttr;
    uint16_t emisor, receptor, nonce;
    uint8_t tipo_payload, payload_size;
    unsigned char* payload;
    const static unsigned message_without_payload_size = 9;
    unsigned transmission_size;
    const static uint16_t BROADCAST_CHANNEL_ID = 0xffff;
public:
    const static uint8_t TTR_MODE = 0;
    const static uint8_t MAXPROP_MODE = 1;
    const static uint8_t PAYLOAD_TEXTO = 0;
    const static uint8_t PAYLOAD_VECTOR = 1;
    const static uint8_t PAYLOAD_ACK_MENSAJE = 2;
    const static uint8_t PAYLOAD_ACK_COMUNICACION = 3;
    const static uint8_t PAYLOAD_BEACON = 4;
    const static unsigned payload_max_size = 191;
    const static unsigned raw_message_max_size = 200;

    Mensaje();
    Mensaje(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, uint8_t _tipo_payload,
        unsigned char* _payload, unsigned payload_size
    );
    Mensaje(const Mensaje& original);
    ~Mensaje();

    void parse_to_transmission(unsigned char* destino);

    void print() const;


    void setEmisor(uint16_t _emisor);
    void setReceptor(uint16_t _receptor);
    void setNonce(uint16_t _nonce);
    void setTTR(uint32_t _ttr);

    uint16_t getEmisor();
    uint16_t getReceptor();
    uint16_t getNonce();
    uint8_t getTipoPayload();
    uint32_t getTTR();

    static Mensaje* parse_from_transmission(const unsigned char* data, uint8_t largo_data);
    unsigned get_transmission_size();
};

#endif