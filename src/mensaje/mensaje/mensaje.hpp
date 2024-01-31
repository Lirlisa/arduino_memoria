#ifndef MENSAJE_HPP
#define MENSAJE_HPP
#include <cstdint>


class Mensaje {
protected:
    uint32_t ttr;
    uint16_t emisor, receptor, nonce;
    uint8_t tipo_payload, payload_size;
    unsigned char* payload;
    unsigned transmission_size;
    const static unsigned message_without_payload_size = 9;
    const static uint16_t BROADCAST_CHANNEL_ID = 0xffff;
public:
    const static uint8_t PAYLOAD_TEXTO = 0;
    const static uint8_t PAYLOAD_VECTOR = 1;
    const static uint8_t PAYLOAD_ACK_MENSAJE = 2;
    const static uint8_t PAYLOAD_ACK_COMUNICACION = 3;
    const static uint8_t PAYLOAD_BEACON = 4;
    const static uint8_t PAYLOAD_TEXTO_VISTO = 5;
    const static uint8_t PAYLOAD_BEACON_CENTRAL = 6;

    const static unsigned payload_max_size = 191;
    const static unsigned raw_message_max_size = 200;

    Mensaje();
    Mensaje(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _nonce, uint8_t _tipo_payload,
        unsigned char* _payload, unsigned payload_size
    );
    Mensaje(const Mensaje& original);
    Mensaje(const unsigned char* data, uint8_t largo_data);
    virtual ~Mensaje();

    Mensaje& operator=(const Mensaje& other);
    bool operator!=(const Mensaje& other) const;
    bool operator==(const Mensaje& other) const;

    void parse_to_transmission(unsigned char* destino) const;

    void peek(unsigned cant_bytes = 6) const;
    void print(unsigned cant_bytes = 6) const;

    void setEmisor(uint16_t _emisor);
    void setReceptor(uint16_t _receptor);
    void setNonce(uint16_t _nonce);
    void setTTR(uint32_t _ttr);

    uint16_t getEmisor() const;
    uint16_t getReceptor() const;
    uint16_t getNonce() const;
    uint8_t getTipoPayload() const;
    uint32_t getTTR() const;

    unsigned get_transmission_size() const;
};

#endif