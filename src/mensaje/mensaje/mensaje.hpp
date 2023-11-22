#ifndef MENSAJE_HPP
#define MENSAJE_HPP
#include <cstdint>

class Mensaje {
protected:
    uint32_t ttr;
    uint16_t emisor, receptor, creador, destinatario, nonce;
    uint8_t tipo_payload, modo_transmision;
    unsigned char payload[101];
public:
    const static int raw_message_size = 114;
    const static uint8_t TTR_MODE = 0;
    const static uint8_t MAXPROP_MODE = 1;

    Mensaje();

    Mensaje(uint32_t _ttr, uint16_t _emisor, uint16_t _receptor,
        uint16_t _creador, uint16_t _destinatario, uint16_t _nonce,
        uint8_t _tipo_payload, uint8_t _modo_transmision, unsigned char* _payload, int payload_size
    );

    unsigned char* parse_to_transmission();

    void print();


    void setEmisor(uint16_t _emisor);
    void setReceptor(uint16_t _receptor);

    uint16_t getDestinatario();
    uint16_t getNonce();

    /*
    Crea un mensaje desde lo recibido por una transmisión, es deber de caller liberar la memoria
    */
    static Mensaje* parse_from_transmission(const unsigned char* data);
};

#endif