#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <router/router.hpp>
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <cstdint>



unsigned long ttr = 1000 * 60 * 60;
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
    Serial.println("01");
    LoRa.setSignalBandwidth(500E3);
    LoRa.enableCrc();
    Serial.println("02");
    char* mensajin = new char[101];
    Serial.println("03");
    char str1[101] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°¬!#$%&/()=ABCDEFG\0";
    Serial.println("04");
    strcpy(mensajin, str1);
    Serial.println("05");
    router = Router(1, ttr, 10);
    Serial.println("06");
    Texto texto;
    texto.creador = 1;
    texto.destinatario = 2;
    texto.contenido = mensajin;
    texto.largo_texto = 100;
    texto.nonce = 0;
    router.agregar_texto(texto);
    Serial.println("07");
}

void loop() {
    if (millis() - ultima_actualizacion_ttr >= intervalo_actualizacion_ttr) {
        aux_tiempo = millis();
        router.update_ttr((uint32_t)(aux_tiempo - ultima_actualizacion_ttr) / 1000);
        ultima_actualizacion_ttr = aux_tiempo;
    }
    //agregar modo sleep o idle e intentar recibir mensajes todo el rato
    if (millis() - tiempo_ultima_transmision >= Router::tiempo_max_espera) {
        Serial.println("Sending beacon");
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