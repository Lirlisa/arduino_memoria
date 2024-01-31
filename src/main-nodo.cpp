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
unsigned long aux_tiempo, random_offset;
long long MEMORIA_MINIMA = 5000;
unsigned id_router;
uint16_t curr_emisor;

void setup() {
    Serial.begin(9600);
    while (!Serial);
    Serial.println("LoRa setup");

    if (!LoRa.begin(915E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.setSpreadingFactor(8);
    LoRa.setSignalBandwidth(500E3);
    LoRa.enableCrc();

    random_offset = (unsigned)LoRa.random() % Router::tiempo_transmision;
    tiempo_ultima_transmision += random_offset;

    id_router = 3;
    router = new Router(id_router, ttr, 10);
    if (id_router == 1) {
        char textin[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°!#$%&/()=ABCDEFG";
        Texto texto(0, id_router, 2, 0, 98, textin);
        router->agregar_texto(texto);
        char textin2[] = "Probando cosas áéíóúññ :)^{}";
        Texto texto2(1, id_router, 2, 0, 36, textin2);
        router->agregar_texto(texto2);
        char textin3[] = "oñABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°!#$%&/()=ABCDEFGABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°!#$%&/()=ABCDEFGñ";
        Texto texto3(2, id_router, 3, 0, 200, textin3);
        router->agregar_texto(texto3);
    }
    else if (id_router == 2) {
        char textin[] = "texto cortito hehe";
        Texto texto(3, id_router, 1, 0, 19, textin);
        router->agregar_texto(texto);
        char textin2[] = "texto del 2 al 3 :O áññ";
        Texto texto2(4, id_router, 3, 0, 27, textin2);
        router->agregar_texto(texto2);
        char textin3[] = "````";
        Texto texto3(5, id_router, 3, 0, 5, textin3);
        router->agregar_texto(texto3);
    }
    else if (id_router == 3) {
        char textin[] = "Texto al límite?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°!#$%&/()=ABCDEFGABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°!#$%&/()=ABCDEFG";
        Texto texto(6, id_router, 1, 0, 212, textin);
        router->agregar_texto(texto);
        char textin2[] = "texto del 3 al 2 :O áññ";
        Texto texto2(7, id_router, 2, 0, 27, textin2);
        router->agregar_texto(texto2);
        char textin3[] = "";
        Texto texto3(8, id_router, 4, 0, 1, textin3);
        router->agregar_texto(texto3);
    }
    router->print_buffer();
}

void loop() {
    if (millis() - ultima_actualizacion_ttr >= intervalo_actualizacion_ttr) {
        aux_tiempo = millis();
        router->update_ttr((uint32_t)(aux_tiempo - ultima_actualizacion_ttr) / 1000);
        ultima_actualizacion_ttr = aux_tiempo;
    }

    // agregar modo sleep o idle e intentar recibir mensajes todo el rato

    // emitimos beacon si corresponde
    if (millis() - tiempo_ultima_transmision >= Router::tiempo_max_espera + random_offset) {
        tiempo_ultima_transmision = millis();
        random_offset = (unsigned)LoRa.random() % Router::tiempo_transmision;
        router->emitir_beacon_signal();
    }

    Serial.print("Memoria libre: ");
    Serial.println(freeMemory());
    // se libera memoria si corresponde
    if (freeMemory() <= MEMORIA_MINIMA) {
        Serial.println("Borrando memoria");
        router->liberar_memoria();
    }

    //escuchamos
    if (!router->recibir_mensaje()) return;
    uint8_t tipo_payload = router->mensaje_pendiente.getTipoPayload();
    if (tipo_payload == Mensaje::PAYLOAD_BEACON || Mensaje::PAYLOAD_BEACON_CENTRAL) {
        if (id_router > router->mensaje_pendiente.getEmisor()) {
            //recibe vectores
            if (
                !router->emitir_ack_comunicacion_hasta_respuesta(
                    router->mensaje_pendiente.getEmisor(),
                    router->mensaje_pendiente.getNonce(),
                    Mensaje::PAYLOAD_VECTOR
                )
                ) return;
            router->iniciar_comunicacion(router->mensaje_pendiente.getEmisor());
        }
        else {
            // manda vectores
            router->emitir_ack_comunicacion(router->mensaje_pendiente.getEmisor(), router->mensaje_pendiente.getNonce());
            router->recibir_comunicacion(router->mensaje_pendiente.getEmisor());
        }
        router->print_buffer();
        router->print_mapa();
        router->print_mis_mensajes();

        if (tipo_payload == Mensaje::PAYLOAD_BEACON_CENTRAL)
            router->set_ttr(ttr);
        return;
    }
    if (tipo_payload == Mensaje::PAYLOAD_ACK_COMUNICACION) {
        Mensaje_ack_comunicacion ack_com(router->mensaje_pendiente);
        if (router->confirmar_con_ultimos_beacons(ack_com)) {
            if (id_router > router->mensaje_pendiente.getEmisor()) {
                //recibe vectores
                curr_emisor = router->mensaje_pendiente.getEmisor();
                if (!router->recibir_mensaje(3, Mensaje::PAYLOAD_VECTOR, curr_emisor)) return;
                router->iniciar_comunicacion(router->mensaje_pendiente.getEmisor());
            }
            else {
                // manda vectores
                router->recibir_comunicacion(router->mensaje_pendiente.getEmisor());
            }
            router->print_buffer();
            router->print_mapa();
            router->print_mis_mensajes();
        }
    }
    else if (tipo_payload == Mensaje::PAYLOAD_VECTOR && id_router > router->mensaje_pendiente.getEmisor()) {
        router->iniciar_comunicacion(router->mensaje_pendiente.getEmisor());
        router->print_buffer();
        router->print_mapa();
        router->print_mis_mensajes();
    }
}