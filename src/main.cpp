#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <cstdint>
#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <router/router.hpp>


int counter = 0;
long lastSendTime = 0;
int interval = 2000;

Router router;

void setup() {

    Serial.begin(9600);

    Serial.println("LoRa Sender non-blocking");

    if (!LoRa.begin(915E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }
    LoRa.enableCrc();




    router = Router();
    // router.guardar_mensaje(msg);
    // unsigned char mensajin[101];
    // mensajin[0] = 100;
    // strcpy((char*)mensajin + 1, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{}[]?.,;:-_+*~'\"|°¬!#$%&/()=ABCDEFG");
    // Mensaje msg(131071, 2, 3, 4, 5, 6, 0, 1, mensajin, 101);
    // msg.print();
    // Serial.println();
    // unsigned char* codificado = msg.parse_to_transmission();
    // Mensaje* msg2 = Mensaje::parse_from_transmission(codificado);
    // Mensaje_texto msg3(*msg2);
    // msg3.print();
    // delete msg2;
}

void loop() {
    if (millis() - lastSendTime > interval) {
        Serial.println("Enter data:");
        while (Serial.available() == 0) {}     //wait for data available
        String teststr = Serial.readString();  //read until timeout
        teststr.trim();

        unsigned char mensajin[101];
        mensajin[0] = 100;
        teststr.toCharArray((char*)mensajin + 1, 100);
        Mensaje_texto* msg = new Mensaje_texto(131071, 2, 3, 4, 5, 6, 0, 1, mensajin, 101);
        router.guardar_mensaje(msg);
        router.enviar_mensaje(0);

        lastSendTime = millis();
        interval = 2000;
    }
    router.recibir_mensaje();
}