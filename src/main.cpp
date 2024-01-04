#include <router/router.hpp>
#include <memoria/memoria.hpp>
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <cstdint>
#include <texto/texto.hpp>


unsigned long ttr = 60 * 60;
Router router;
bool hay_mensaje;
unsigned long tiempo_ultima_transmision = 0;
unsigned long ultima_actualizacion_ttr = 0;
unsigned long intervalo_actualizacion_ttr = 1000; //debe ser multiplo de 1000 para que vaya de a segundos por el tema de la división entera
unsigned long aux_tiempo;

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("LoRa setup");

    if (!LoRa.begin(915E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.setSignalBandwidth(500E3);
    LoRa.enableCrc();
    router = Router(1, ttr, 10);
    char textin[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°!#$%&/()=ABCDEFG";
    Texto texto(0, 1, 1, 1, 98, textin);
    router.agregar_texto(texto);
    // texto.print();
}

void loop() {
    if (millis() - ultima_actualizacion_ttr >= intervalo_actualizacion_ttr) {
        aux_tiempo = millis();
        router.update_ttr((uint32_t)(aux_tiempo - ultima_actualizacion_ttr) / 1000);
        ultima_actualizacion_ttr = aux_tiempo;
    }
    //agregar modo sleep o idle e intentar recibir mensajes todo el rato
    if (millis() - tiempo_ultima_transmision >= Router::tiempo_max_espera) {
        tiempo_ultima_transmision = millis();
        Serial.println("Sending beacon");
        router.enviar_mensaje_texto_ttr(1);
        Serial.print("Memoria Disponible: ");
        Serial.println(freeMemory());
        // if (!router.emitir_beacon_signal()) return;
        // if (!router.recibir_mensaje()) return;
        // uint8_t tipo_payload = router.mensaje_pendiente.getTipoPayload();
        // if (tipo_payload == Mensaje::PAYLOAD_BEACON) {
        //     router.iniciar_comunicacion(router.mensaje_pendiente.getEmisor());
        // }
        // else if (tipo_payload == Mensaje::PAYLOAD_ACK_COMUNICACION) {
        //     router.recibir_comunicacion(router.mensaje_pendiente.getEmisor());
        // }
    }

    // if (millis() - tiempo_ultima_transmision >= Router::tiempo_max_espera) {
    //     if (!router.recibir_mensaje()) return;
    // }
}