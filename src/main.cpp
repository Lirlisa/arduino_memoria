#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <router/router.hpp>
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <cstdint>



Router router;
unsigned long ttr;
bool hay_mensaje;
unsigned long tiempo_ultima_transmision = 0;


void setup() {

    Serial.begin(9600);
    // while (!Serial);
    // Serial.println("LoRa Sender non-blocking");

    if (!LoRa.begin(915E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.setSignalBandwidth(500E3);
    LoRa.enableCrc();

    ttr = 1000 * 60 * 60;
    router = Router(1, ttr, 10);

    char* mensajin = new char[100];
    strcpy(mensajin, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°¬!#$%&/()=ABCDEFG");

    Texto texto;
    texto.creador = 1;
    texto.destinatario = 2;
    texto.contenido = mensajin;
    texto.largo_texto = 100;
    texto.nonce = 0;
    router.agregar_texto(texto);
}

void loop() {
    if (millis() - tiempo_ultima_transmision >= Router::tiempo_max_espera) {
        tiempo_ultima_transmision = millis();
        if (!router.emitir_beacon_signal()) return;
        if (!router.recibir_mensaje()) return;
        uint8_t tipo_payload = router.mensaje_pendiente.getTipoPayload();
        if (tipo_payload == Mensaje::PAYLOAD_BEACON) {
            router.iniciar_comunicacion(router.mensaje_pendiente.getEmisor());
        }
        else if (tipo_payload == Mensaje::PAYLOAD_ACK_COMUNICACION) {
            router.recibir_comunicacion(router.mensaje_pendiente.getEmisor());
        }
    }
}