#include <router/router.hpp>
#include <memoria/memoria.hpp>
#include <texto/texto.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <cstdint>



unsigned long ttr = 60 * 60;
Router* router;
bool hay_mensaje;
unsigned long tiempo_ultima_transmision = 0;
unsigned long ultima_actualizacion_ttr = 0;
unsigned long intervalo_actualizacion_ttr = 1000; //debe ser multiplo de 1000 para que vaya de a segundos por el tema de la división entera
unsigned long aux_tiempo;
long long MEMORIA_MINIMA = 5000;

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
    router = new Router(1, ttr, 10);
    char textin[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°!#$%&/()=ABCDEFG";
    Texto texto(0, 1, 2, 0, 98, textin);
    router->agregar_texto(texto);
}

void loop() {
    if (millis() - ultima_actualizacion_ttr >= intervalo_actualizacion_ttr) {
        aux_tiempo = millis();
        // router.update_ttr((uint32_t)(aux_tiempo - ultima_actualizacion_ttr) / 1000);
        ultima_actualizacion_ttr = aux_tiempo;
    }
    // agregar modo sleep o idle e intentar recibir mensajes todo el rato
    if (millis() - tiempo_ultima_transmision >= Router::tiempo_max_espera) {
        tiempo_ultima_transmision = millis();
        if (!router->recibir_mensaje()) {
            if (!router->emitir_beacon_signal()) return;
            Serial.println("Emitido beacon");
            return;
        };
        uint8_t tipo_payload = router->mensaje_pendiente.getTipoPayload();
        if (tipo_payload == Mensaje::PAYLOAD_BEACON) {
            router->emitir_ack_comunicacion(router->mensaje_pendiente.getEmisor(), router->mensaje_pendiente.getNonce());
            if (router->get_id() > router->mensaje_pendiente.getEmisor())
                router->iniciar_comunicacion(router->mensaje_pendiente.getEmisor());
            else
                router->recibir_comunicacion(router->mensaje_pendiente.getEmisor());
        }
        else if (tipo_payload == Mensaje::PAYLOAD_ACK_COMUNICACION) {
            Mensaje_ack_comunicacion ack_com(router->mensaje_pendiente);
            Serial.println("Parametros de confirmar_ack en main.cpp");
            Serial.println(router->get_beacon_nonce());
            Serial.println(router->get_id());
            Serial.println(router->BROADCAST_CHANNEL_ID);
            if (ack_com.confirmar_ack(router->get_beacon_nonce(), router->get_id(), router->BROADCAST_CHANNEL_ID)) {
                if (router->get_id() > router->mensaje_pendiente.getEmisor())
                    router->iniciar_comunicacion(router->mensaje_pendiente.getEmisor());
                else
                    router->recibir_comunicacion(router->mensaje_pendiente.getEmisor());
            }
        }
        if (freeMemory() <= MEMORIA_MINIMA) {
            router->liberar_memoria();
        }

    }

    // if (millis() - tiempo_ultima_transmision >= Router::tiempo_max_espera) {
    //     tiempo_ultima_transmision = millis();
    //     if (!router.recibir_mensaje()) return;
    // }
}