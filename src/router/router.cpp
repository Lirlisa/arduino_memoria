#include <router/router.hpp>
#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <texto/texto.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <mensaje/mensaje_ack_mensaje/mensaje_ack_mensaje.hpp>
#include <mensaje/mensaje_textos_vistos/mensaje_textos_vistos.hpp>
#include <mensaje/ack.hpp>
#include <LoRa.h>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <vector>
#include <cmath>


Router::Router(uint16_t _id, uint32_t _ttr, unsigned int initial_capacity) : buffer(_id) {
    mis_mensajes.reserve(initial_capacity);
    id = _id;
    ttr = _ttr;
    nonce_actual = 0;
    beacon_signal = Mensaje(
        ttr, id, BROADCAST_CHANNEL_ID, get_update_nonce(), Mensaje::PAYLOAD_BEACON, (unsigned char*)nullptr, 0
    );

    ultimos_nonce_beacon = new uint16_t[max_size_ultimos_nonce_beacon];
}

Router::~Router() {
    delete[] ultimos_nonce_beacon;
    ultimos_nonce_beacon = nullptr;
}

bool Router::agregar_a_mis_mensajes(const Texto& texto) {
    if (texto.destinatario != id) return false;

    mis_mensajes.push_back(texto);
    agregar_a_acks(texto);
    return true;
}

bool Router::agregar_a_mis_mensajes(const std::vector<Texto>& textos) {
    const uint16_t mi_id = id;
    auto para_mi = [mi_id](const Texto& texto) {return mi_id == texto.destinatario;};

    mis_mensajes.reserve(mis_mensajes.size() + textos.size());
    std::copy_if(
        textos.begin(),
        textos.end(),
        std::back_inserter(mis_mensajes),
        para_mi
    );

    buffer.agregar_ack(textos);
    return true;
}

bool Router::agregar_a_acks(const Texto& texto) {
    uint64_t llave = texto.hash();
    buffer.agregar_ack(llave);
    return true;
}

bool Router::enviar_mensaje_texto_maxprop(uint16_t receptor, unsigned periodos_espera, std::vector<uint64_t>* excepciones) {
    Serial.println("Flag enviar_mensaje_texto_maxprop 1");
    if (!buffer.hay_mensajes_bajo_threshold()) return true;
    Serial.println("Flag enviar_mensaje_texto_maxprop 1");
    std::vector<Texto> textos = buffer.obtener_textos_bajo_threshold(receptor, excepciones);
    Serial.println("Flag enviar_mensaje_texto_maxprop 2");
    std::vector<Mensaje_texto> mensajes = Mensaje_texto::crear_mensajes(ttr, id, receptor, 0, textos);
    Serial.println("Flag enviar_mensaje_texto_maxprop 3");
    uint16_t nonce;
    for (std::vector<Mensaje_texto>::iterator mensaje = mensajes.begin(); mensaje != mensajes.end(); mensaje++) {
        Serial.println("Flag enviar_mensaje_texto_maxprop 4");
        nonce = get_update_nonce();
        mensaje->setNonce(nonce);
        Serial.println("Flag enviar_mensaje_texto_maxprop 5");
        Serial.println("----- Enviando mensaje texto maxprop -----");
        mensaje->print();
        Serial.println("----- Enviando mensaje texto maxprop -----");
        Serial.println("Flag enviar_mensaje_texto_maxprop 6");
        if (!enviar_mensaje(*mensaje, periodos_espera)) return false;
        Serial.println("Flag enviar_mensaje_texto_maxprop 7");
    }
    Serial.println("Flag enviar_mensaje_texto_maxprop 8");
    return true;
}

/*
@brief Enviar en ttr significa que se envían los mensajes que estén sobre el threshold de saltos,
y que tengan una probabilidad de entrega mayor a la que este propio nodo tiene. Luego se eliminan.
*/
bool Router::enviar_mensaje_texto_ttr(uint16_t receptor, unsigned periodos_espera, std::vector<uint64_t>* excepciones) {
    if (!buffer.hay_mensajes_sobre_threshold()) return true;

    std::vector<Texto> textos = buffer.obtener_textos_sobre_threshold(receptor, excepciones);
    std::vector<Mensaje_texto> mensajes = Mensaje_texto::crear_mensajes(ttr, id, receptor, 0, textos);
    uint16_t nonce;
    Mensaje_texto mensaje;
    unsigned cantidad_total_textos_enviados = 0;
    unsigned cantidad_textos_enviados;
    for (unsigned i = 0; i < mensajes.size(); i++) {
        mensaje = mensajes[i];
        nonce = get_update_nonce();
        mensaje.setNonce(nonce);

        Serial.println("----- Enviando mensaje texto ttr -----");
        mensaje.print();
        Serial.println("----- Enviando mensaje texto ttr -----");

        if (!enviar_mensaje(mensaje, periodos_espera)) return false;

        cantidad_textos_enviados = mensaje.get_cantidad_textos();
        std::vector<Texto> v(
            textos.begin() + cantidad_total_textos_enviados,
            textos.begin() + cantidad_total_textos_enviados + cantidad_textos_enviados
        );
        buffer.eliminar_textos(v);
        cantidad_total_textos_enviados += cantidad_textos_enviados;
    }
    return true;
}

bool Router::hay_paquete_LoRa(unsigned periodos_espera) {
    unsigned long tiempo_inicio = millis();
    while (true) {
        if (LoRa.parsePacket() != 0) return true;
        if (millis() - tiempo_inicio > periodos_espera * tiempo_transmision) return false;
    }
}

bool Router::recibir_mensaje(unsigned periodos_espera, int tipo_mensaje, int emisor_esperado) {
    for (unsigned intentos = 0; intentos < periodos_espera; intentos++) {
        if (!hay_paquete_LoRa(1)) continue;
        unsigned char data[Mensaje::raw_message_max_size];
        unsigned largo = 0;
        for (unsigned i = 0; i < Mensaje::raw_message_max_size && LoRa.available() > 0; i++) {
            data[i] = LoRa.read();
            largo++;
        }
        Mensaje msg = Mensaje(data, largo);
        Serial.println("----- Mensaje recibido -----");
        msg.print();
        Serial.println("----- Fin Mensaje recibido -----");
        if (mensaje_ya_visto(msg) || msg.getEmisor() == id) { //evitar mensajes reflejados
            Serial.println("^^^^^ Mensaje rechazado ^^^^^");
            continue;
        }
        Serial.println("^^^^^ Mensaje aceptado ^^^^^");
        agregar_a_ya_visto(msg);
        Serial.println("Flag 1 recibir_mensaje");

        if (msg.getReceptor() == id || msg.getReceptor() == BROADCAST_CHANNEL_ID) {
            if (tipo_mensaje > -1 && (tipo_mensaje != msg.getTipoPayload() || Mensaje::PAYLOAD_ACK_COMUNICACION != msg.getTipoPayload())) continue;
            if (emisor_esperado > -1 && emisor_esperado != msg.getEmisor()) continue;
            mensaje_pendiente = msg;
            hay_mensaje_pendiente = true;
            Serial.println("Flag 2 recibir_mensaje");
            return true;
        }

        Serial.println("Flag 3 recibir_mensaje");
        if (msg.getTipoPayload() == Mensaje::PAYLOAD_VECTOR) {
            Serial.println("Flag 4 recibir_mensaje");
            std::vector<par_costo_id> pares = Mensaje_vector(msg).get_pares();
            Serial.println("Flag 4.1 recibir_mensaje");
            buffer.agregar_probabilidades(msg.getEmisor(), pares); //eavesdropping por defecto
            Serial.println("Flag 4.2 recibir_mensaje");
            hay_mensaje_pendiente = false;
        }
        else if (msg.getTipoPayload() == Mensaje::PAYLOAD_ACK_MENSAJE) {
            Serial.println("Flag 5 recibir_mensaje");
            Mensaje_ack_mensaje msg_ack_mensaje(msg);
            Serial.println("Flag 5.1 recibir_mensaje");
            std::vector<uint64_t> acks = msg_ack_mensaje.obtener_acks();
            Serial.println("Flag 5.2 recibir_mensaje");
            buffer.agregar_ack(acks); //eavesdropping por defecto
            Serial.println("Flag 5.3 recibir_mensaje");
            hay_mensaje_pendiente = false;
        }

        hay_mensaje_pendiente = false;
        Serial.println("Flag 6 recibir_mensaje");
    }
    return false;
}

uint16_t Router::get_update_nonce() {
    return nonce_actual++;
}

bool Router::emitir_beacon_signal() {
    uint16_t _nonce = get_update_nonce();
    beacon_signal.setNonce(_nonce);
    beacon_signal.setTTR(ttr);
    unsigned transmission_size = beacon_signal.get_transmission_size();
    unsigned char data[transmission_size];
    beacon_signal.parse_to_transmission(data);

    Serial.println("----- Emitiendo beacon -----");
    beacon_signal.print();
    Serial.println("----- Emitido beacon -----");

    if (LoRa.beginPacket()) {
        LoRa.write(data, transmission_size);
        LoRa.endPacket(); // true = async / non-blocking mode

        cabeza_ultimos_nonce_beacon = (cabeza_ultimos_nonce_beacon + 1) % max_size_ultimos_nonce_beacon;
        ultimos_nonce_beacon[cabeza_ultimos_nonce_beacon] = _nonce;
        size_ultimos_nonce_beacon += size_ultimos_nonce_beacon + 1 > max_size_ultimos_nonce_beacon ? 0 : 1;
        return true;
    }
    return false;
}

bool Router::emitir_ack_comunicacion(uint16_t receptor, uint16_t nonce_original, uint16_t* nonce_usado) {
    uint16_t _nonce = get_update_nonce();
    Mensaje_ack_comunicacion mensaje = Mensaje_ack_comunicacion(
        id, receptor, _nonce, nonce_original
    );
    Serial.println("----- Emitiendo ack comunicación -----");
    mensaje.print();
    Serial.println("----- Emitido ack comunicación -----");

    unsigned char mensaje_a_enviar[mensaje.get_transmission_size()];
    mensaje.parse_to_transmission(mensaje_a_enviar);

    if (LoRa.beginPacket()) {
        LoRa.write(mensaje_a_enviar, mensaje.get_transmission_size());
        LoRa.endPacket(); // true = async / non-blocking mode

        Serial.println("flag 1 emitir_ack_comunicacion");
        if (nonce_usado != (uint16_t*)nullptr) {
            *nonce_usado = _nonce;
        }
        Serial.println("flag 2 emitir_ack_comunicacion");
        return true;
    }
    return false;
}

/*
@brief Envia todos los mensajes dirigidos al destinatario
*/
bool Router::enviar_todos_a_destinatario(uint16_t destinatario) {
    Serial.println("flag enviar_todos_a_destinatario 0");
    std::vector<Texto> textos_para_destinatario = buffer.obtener_textos_destinatario(destinatario);
    Serial.println("flag enviar_todos_a_destinatario 0.1");
    std::vector<Mensaje_texto> mensajes_a_enviar = Mensaje_texto::crear_mensajes(
        ttr, id, destinatario, 0, textos_para_destinatario
    );
    Serial.println("flag enviar_todos_a_destinatario 0.2");
    std::vector<Mensaje_texto>::iterator mensaje;
    uint16_t nonce;
    unsigned textos_enviados = 0;
    Serial.println("flag enviar_todos_a_destinatario 1");
    for (mensaje = mensajes_a_enviar.begin(); mensaje != mensajes_a_enviar.end(); mensaje++) {
        Serial.println("flag enviar_todos_a_destinatario 2");
        nonce = get_update_nonce();
        mensaje->setNonce(nonce);

        Serial.println("flag enviar_todos_a_destinatario 3");
        Serial.println("----- Enviando mensaje todos a destinatario -----");
        mensaje->print();
        Serial.println("----- Fin Enviando mensaje todos a destinatario -----");
        Serial.println("flag enviar_todos_a_destinatario 4");
        if (!enviar_mensaje(*mensaje)) return false;

        Serial.println("flag enviar_todos_a_destinatario 5");
        std::vector<Texto> textos_a_borrar(
            textos_para_destinatario.begin() + textos_enviados,
            textos_para_destinatario.begin() + textos_enviados + mensaje->get_cantidad_textos()
        );
        Serial.println("flag enviar_todos_a_destinatario 6");
        textos_enviados += mensaje->get_cantidad_textos();
        Serial.println("flag enviar_todos_a_destinatario 7");
        buffer.agregar_ack(textos_a_borrar);
        Serial.println("flag enviar_todos_a_destinatario 8");
    }
    return true;
}

bool Router::enviar_vectores_de_probabilidad(uint16_t receptor) {
    unsigned char data_a_enviar[Mensaje_vector::payload_max_size];
    std::vector<par_costo_id> pares = buffer.obtener_vector_probabilidad();
    unsigned tandas = pares.size() / Mensaje_vector::max_cantidad_pares_por_payload + 1;
    uint8_t cantidad_a_enviar;
    float costo;
    uint16_t id_nodo, nonce;
    int ultimo_enviado = -1;
    for (unsigned i = 0; i < tandas; ++i) {
        cantidad_a_enviar = std::min(pares.size() - ultimo_enviado - 1, Mensaje_vector::max_cantidad_pares_por_payload);
        data_a_enviar[0] = cantidad_a_enviar;
        for (unsigned j = ultimo_enviado + 1; j < (unsigned)(ultimo_enviado + 1 + cantidad_a_enviar); ++j) {
            id_nodo = pares.at(j).second;
            costo = pares.at(j).first;
            std::memcpy(data_a_enviar + 1 + j * Mensaje_vector::par_costo_id_size, &id_nodo, 2);
            std::memcpy(data_a_enviar + 1 + j * Mensaje_vector::par_costo_id_size + 2, &costo, 4);
        }
        nonce = get_update_nonce();
        Mensaje_vector mensaje = Mensaje_vector(
            id, receptor, nonce, data_a_enviar, 1 + Mensaje_vector::par_costo_id_size * cantidad_a_enviar
        );
        Serial.println("----- Enviando vector -----");
        mensaje.print();
        Serial.println("----- Enviado vector -----");

        if (!enviar_mensaje(mensaje)) return false;
    }
    return true;
}

bool Router::enviar_acks_mensajes(uint16_t receptor) {
    std::vector<uint64_t> acks = buffer.obtener_acks();
    std::vector<Mensaje_ack_mensaje> mensajes = Mensaje_ack_mensaje::crear_mensajes(id, receptor, 0, acks);
    uint16_t nonce;

    for (std::vector<Mensaje_ack_mensaje>::iterator mensaje = mensajes.begin(); mensaje != mensajes.end(); mensaje++) {
        nonce = get_update_nonce();
        mensaje->setNonce(nonce);

        if (!enviar_mensaje(*mensaje))return false;
    }

    return true;
}

/*
@brief Cuando se recibió una señal de beacon entonces se inicia la comunicación con un ack de comunicación para que el nodo receptor
sepa que puede enviar mensajes.
*/
void Router::iniciar_comunicacion(uint16_t receptor) {
    buffer.actualizar_propias_probabilidades(receptor);

    uint32_t receptor_ttr = mensaje_pendiente.getTTR(), ttr_inicio = ttr; // para no comparar valores pasados con futuros

    Serial.println("----- Paso 1 iniciar_comunicacion -----");
    //esperamos los vectores de maxprop
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_VECTOR && receptor == mensaje_pendiente.getEmisor()) {
            Serial.println("Recibí un vector");
            Mensaje_vector msg(mensaje_pendiente);
            Serial.println("----- Mensaje vector en iniciar_comunicacion en paso 1 -----");
            msg.print(7);
            Serial.println("----- Fin Mensaje vector en iniciar_comunicacion paso 1 -----");
            std::vector<par_costo_id> pares = msg.get_pares();
            buffer.agregar_probabilidades(receptor, pares);
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_ACK_MENSAJE)) {
                Serial.println("Termino porque no se recibió ningún mensaje 1");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION || receptor != mensaje_pendiente.getEmisor());
    // enviamos nuestros vectores
    if (!enviar_vectores_de_probabilidad(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los vectores");
        return;
    };

    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_ACK_MENSAJE)) {
        Serial.println("Termino porque no se recibió ningún mensaje 2");
        return;
    }

    Serial.println("----- Paso 2 iniciar_comunicacion -----");
    //recibimos los ack de mensajes
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_ACK_MENSAJE) {
            Mensaje_ack_mensaje msg(mensaje_pendiente);
            std::vector<uint64_t> acks = msg.obtener_acks();
            buffer.agregar_ack(acks);
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_ACK_MENSAJE)) {
                Serial.println("Termino porque no se recibió ningún mensaje 2.5");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    //enviamos acks de mensajes
    if (!enviar_acks_mensajes(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los ack de mensajes");
        return;
    }
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO_VISTO)) {
        Serial.println("Termino porque no se recibió ningún mensaje 3");
        return;
    }

    Serial.println("----- Paso 3 iniciar_comunicacion -----");
    //recibimos los mensajes que la otra parte ya tiene
    std::vector<uint64_t> hashes_textos_vistos;
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO_VISTO) {
            Mensaje_textos_vistos msg(mensaje_pendiente);
            std::vector<uint64_t> hashes = msg.obtener_hashes();
            hashes_textos_vistos.insert(hashes_textos_vistos.end(), hashes.begin(), hashes.end());
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO_VISTO)) {
                Serial.println("Termino porque no se recibió ningún mensaje 3.5");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    if (!enviar_textos_ya_vistos(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los mensajes ya vistos");
        return;
    }
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
        Serial.println("Termino porque no se recibió ningún mensaje 3");
        return;
    }

    Serial.println("----- Paso 4 iniciar_comunicacion -----");
    //recibo mensajes de texto para mi
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            agregar_a_mis_mensajes(textos);
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
                Serial.println("Termino porque no se recibió ningún mensaje 4.5");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    // enviamos mensajes de texto para el receptor
    if (!enviar_todos_a_destinatario(receptor)) {
        Serial.println("Termino porque no se pudieron enviar todos los mensajes para el destinatario");
        return;
    }
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
        Serial.println("Termino porque no se recibió ningún mensaje 4");
        return;
    }

    Serial.println("----- Paso 5 iniciar_comunicacion -----");
    // recibo textos en modo maxprop
    do {
        Serial.println("Flag iniciar_comunicacion paso 5 1");
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Serial.println("Flag iniciar_comunicacion paso 5 2");
            Mensaje_texto msg(mensaje_pendiente);
            Serial.println("Flag iniciar_comunicacion paso 5 3");
            std::vector<Texto> textos = msg.obtener_textos();
            Serial.println("Flag iniciar_comunicacion paso 5 4");
            buffer.agregar_texto(textos);
            Serial.println("Flag iniciar_comunicacion paso 5 5");
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
                Serial.println("Termino porque no se recibió ningún mensaje 5.5");
                return;
            }
            Serial.println("Flag iniciar_comunicacion paso 5 6");
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    //enviamos textos en modo maxprop
    if (!enviar_mensaje_texto_maxprop(receptor, 3, &hashes_textos_vistos)) {
        Serial.println("Termino porque no se pudieron enviar los mensajes en maxprop");
        return;
    };
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
        Serial.println("Termino porque no se recibió ningún mensaje 5");
        return;
    }

    Serial.println("----- Paso 6 iniciar_comunicacion -----");

    if (receptor_ttr <= ttr_inicio) {
        //enviar textos en modo ttr
        if (!enviar_mensaje_texto_ttr(receptor, 3, &hashes_textos_vistos)) {
            Serial.println("Termino porque no se recibió ningún mensaje 6");
            return;
        }
        emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
    }
    else {
        //recibo textos en modo ttr
        if (!recibir_mensaje(5)) {
            Serial.println("Termino porque no se recibió ningún mensaje 6.5");
            return;
        }
        do {
            if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
                Mensaje_texto msg(mensaje_pendiente);
                std::vector<Texto> textos = msg.obtener_textos();
                buffer.agregar_texto(textos);
                if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
                    Serial.println("Termino porque no se recibió ningún mensaje 7");
                    return;
                }
            }
        } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    }
}

/*
@brief Cuando un nodo receptor respondió a una señal de beacon con un ack de comunicación entonces podemos comenzar a enviar mensajes.
*/
void Router::recibir_comunicacion(uint16_t receptor) {
    buffer.actualizar_propias_probabilidades(receptor);
    uint32_t receptor_ttr = mensaje_pendiente.getTTR(), ttr_inicio = ttr; // para no comparar valores pasados con futuros

    Serial.println("----- Paso 1 recibir_comunicacion -----");
    // enviamos nuestros vectores
    if (!enviar_vectores_de_probabilidad(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los vectores");
        return;
    };
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_VECTOR)) {
        Serial.println("Termino porque no se recibió ningún mensaje 1");
        return;
    }
    //esperamos los vectores de maxprop
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_VECTOR && receptor == mensaje_pendiente.getEmisor()) {
            Serial.println("Recibí un vector");
            Mensaje_vector msg(mensaje_pendiente);
            Serial.println("----- Mensaje vector en recibir_comunicacion en paso 1 -----");
            msg.print(7);
            Serial.println("----- Fin Mensaje vector en recibir_comunicacion paso 1 -----");
            std::vector<par_costo_id> pares = msg.get_pares();

            buffer.agregar_probabilidades(receptor, pares);
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_VECTOR)) {
                Serial.println("Termino porque no se recibió ningún mensaje 1.5");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION || receptor != mensaje_pendiente.getEmisor());

    Serial.println("----- Paso 2 recibir_comunicacion -----");
    //enviamos acks de mensajes
    if (!enviar_acks_mensajes(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los ack de mensajes");
        return;
    };
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_ACK_MENSAJE)) {
        Serial.println("Termino porque no se recibió ningún mensaje 2");
        return;
    }
    //recibimos los ack de mensajes
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_ACK_MENSAJE) {
            Mensaje_ack_mensaje msg(mensaje_pendiente);
            std::vector<uint64_t> acks = msg.obtener_acks();
            buffer.agregar_ack(acks);
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_ACK_MENSAJE)) {
                Serial.println("Termino porque no se recibió ningún mensaje 2.5");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);

    Serial.println("----- Paso 3 recibir_comunicacion -----");
    //enviamos los mensajes que ya se han recibido
    std::vector<uint64_t> hashes_textos_vistos;
    if (!enviar_textos_ya_vistos(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los mensajes ya vistos");
        return;
    }
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO_VISTO)) {
        Serial.println("Termino porque no se recibió ningún mensaje 3");
        return;
    }
    //recibimos los mensajes que la otra parte ya tiene
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO_VISTO) {
            Mensaje_textos_vistos msg(mensaje_pendiente);
            std::vector<uint64_t> hashes = msg.obtener_hashes();
            hashes_textos_vistos.insert(hashes_textos_vistos.end(), hashes.begin(), hashes.end());
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO_VISTO)) {
                Serial.println("Termino porque no se recibió ningún mensaje 3.5");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);


    Serial.println("----- Paso 4 recibir_comunicacion -----");
    // enviamos mensajes de texto para el receptor
    if (!enviar_todos_a_destinatario(receptor)) {
        Serial.println("Termino porque no se pudieron enviar todos a destinatario");
        return;
    }
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
        Serial.println("Termino porque no se recibió ningún mensaje 4");
        return;
    }
    //recibo mensajes de texto para mi
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            agregar_a_mis_mensajes(textos);
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
                Serial.println("Termino porque no se recibió ningún mensaje 4.5");
                return;
            }
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);

    Serial.println("----- Paso 5 recibir_comunicacion -----");
    //enviamos textos en modo maxprop
    if (!enviar_mensaje_texto_maxprop(receptor, 3, &hashes_textos_vistos)) {
        Serial.println("Termino porque no se pudieron enviar mensajes maxprop");
        return;
    }
    Serial.println("Flag recibir_comunicacion paso 5 1");
    if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
        Serial.println("Termino porque no se recibió ningún mensaje 5");
        return;
    }
    Serial.println("Flag recibir_comunicacion paso 5 2");
    // recibo textos en modo maxprop
    do {
        Serial.println("Flag recibir_comunicacion paso 5 3");
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Serial.println("Flag recibir_comunicacion paso 5 4");
            Mensaje_texto msg(mensaje_pendiente);
            Serial.println("Flag recibir_comunicacion paso 5 5");
            std::vector<Texto> textos = msg.obtener_textos();
            Serial.println("Flag recibir_comunicacion paso 5 6");
            buffer.agregar_texto(textos);
            Serial.println("Flag recibir_comunicacion paso 5 7");
            if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
                Serial.println("Termino porque no se recibió ningún mensaje 5.5");
                return;
            }
            Serial.println("Flag recibir_comunicacion paso 5 8");
        }
        Serial.println("Flag recibir_comunicacion paso 5 9");
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);

    Serial.println("----- Paso 6 recibir_comunicacion -----");

    if (receptor_ttr < ttr_inicio) {
        //enviamos textos en modo ttr
        Serial.println("Flag recibir_comunicacion paso 6 1");
        if (!enviar_mensaje_texto_ttr(receptor, 3, &hashes_textos_vistos)) {
            Serial.println("Termino porque no se pudieron enviar mensajes ttr");
            return;
        }
        Serial.println("Flag recibir_comunicacion paso 6 2");
        emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        Serial.println("Flag recibir_comunicacion paso 6 3");
    }
    else {
        //recibo textos en modo ttr
        Serial.println("Flag recibir_comunicacion paso 6 4");
        if (!recibir_mensaje(5)) {
            Serial.println("Termino porque no se recibió ningún mensaje 6");
            return;
        }
        Serial.println("Flag recibir_comunicacion paso 6 5");
        do {
            Serial.println("Flag recibir_comunicacion paso 6 6");
            if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
                Serial.println("Flag recibir_comunicacion paso 6 7");
                Mensaje_texto msg(mensaje_pendiente);
                Serial.println("Flag recibir_comunicacion paso 6 8");
                std::vector<Texto> textos = msg.obtener_textos();
                Serial.println("Flag recibir_comunicacion paso 6 9");
                buffer.agregar_texto(textos);
                Serial.println("Flag recibir_comunicacion paso 6 10");
                if (!emitir_ack_comunicacion_hasta_respuesta(receptor, mensaje_pendiente.getNonce(), Mensaje::PAYLOAD_TEXTO)) {
                    Serial.println("Termino porque no se recibió ningún mensaje 6.5");
                    return;
                }
                Serial.println("Flag recibir_comunicacion paso 6 11");
            }
            Serial.println("Flag recibir_comunicacion paso 6 12");
        } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
        Serial.println("Flag recibir_comunicacion paso 6 13");
    }
}

uint16_t Router::get_nonce_beacon_signal() const {
    return beacon_signal.getNonce();
}

void Router::agregar_texto(std::vector<Texto>& textos) {
    buffer.agregar_texto(textos);
}

void Router::agregar_texto(Texto& texto) {
    buffer.agregar_texto(texto);
}

void Router::set_ttr(uint32_t _ttr) {
    ttr = _ttr;
}
void Router::update_ttr(uint32_t segundos_transcurridos) {
    ttr = ttr > segundos_transcurridos + 1 ? ttr - segundos_transcurridos : 1;
}

void Router::print_buffer() const {
    buffer.print();
}

void Router::print_mapa() const {
    buffer.print_mapa();
}

void Router::print_mis_mensajes() const {
    Serial.println("----- Mis mensajes -----");
    Serial.print("Elementos en mis mensajes: ");
    Serial.println(mis_mensajes.size());
    for (unsigned i = 0; i < mis_mensajes.size(); i++) {
        mis_mensajes.at(i).print();
        Serial.println("-----");
    }
}

uint64_t Router::obtener_hash_mensaje(uint16_t nonce, uint16_t emisor, uint16_t receptor) {
    uint64_t data = 0;
    data |= ((uint64_t)nonce) << 48;
    data |= ((uint64_t)emisor) << 32;
    data |= ((uint64_t)receptor) << 16;
    return data;
}

bool Router::mensaje_ya_visto(Mensaje& mensaje) {
    uint64_t hash = obtener_hash_mensaje(mensaje.getNonce(), mensaje.getEmisor(), mensaje.getReceptor());
    return ya_vistos.find(hash) != ya_vistos.end();
}

void Router::agregar_a_ya_visto(Mensaje& mensaje) {
    if (!mensaje_ya_visto(mensaje))
        ya_vistos.insert(obtener_hash_mensaje(mensaje.getNonce(), mensaje.getEmisor(), mensaje.getReceptor()));
}

unsigned Router::get_cantidad_ya_vistos() const {
    return ya_vistos.size();
}

uint64_t Router::obtener_hash_texto(const Texto& texto) {
    return texto.hash();
}

void Router::liberar_memoria() {
    buffer.eliminar_textos_sobre_threshold();
}

uint16_t Router::get_beacon_nonce() const {
    return beacon_signal.getNonce();
}

uint16_t Router::get_id() const {
    return id;
}

/*
@brief Envia cualquier tipo de mensaje utilizando Aloha. La máxima cantidad de intentos es 8 por temas de implementación.
@return true si se recibió un ack desde la contraparte, false en caso contrario.
*/
bool Router::enviar_mensaje(const Mensaje& mensaje, unsigned intentos) {
    unsigned long tiempo_inicio;
    unsigned ciclos_a_esperar, tiempo_max_espera = 1, transmission_size = mensaje.get_transmission_size();

    unsigned char data[transmission_size];
    mensaje.parse_to_transmission(data);

    for (unsigned i = 0; i < intentos; i++) {
        if (LoRa.beginPacket()) {
            LoRa.write(data, transmission_size);
            LoRa.endPacket(); // true = async / non-blocking mode
            ciclos_a_esperar = 2; // esperamos a 2 viajes del mensaje
        }
        else {
            ciclos_a_esperar = 0;
        }
        tiempo_max_espera <<= 2;
        ciclos_a_esperar += ((unsigned)LoRa.random()) % (tiempo_max_espera);
        tiempo_inicio = millis();
        while (millis() - tiempo_inicio <= ciclos_a_esperar * tiempo_transmision) {
            if (!recibir_mensaje(1)) continue;
            if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_ACK_COMUNICACION) {
                Mensaje_ack_comunicacion msg_ack(mensaje_pendiente);
                bool confirmacion = msg_ack.confirmar_ack(
                    mensaje.getNonce(),
                    mensaje.getEmisor(),
                    mensaje.getReceptor()
                );
                if (confirmacion) return true;
            }
        }
    }
    return false;
}

bool Router::confirmar_con_ultimos_beacons(const Mensaje_ack_comunicacion& ack_com) const {
    unsigned pos;
    for (unsigned i = 0; i < size_ultimos_nonce_beacon; i++) {
        pos = (max_size_ultimos_nonce_beacon + cabeza_ultimos_nonce_beacon - i) % max_size_ultimos_nonce_beacon;
        if (ack_com.confirmar_ack(ultimos_nonce_beacon[pos], id, BROADCAST_CHANNEL_ID)) return true;
    }
    return false;
}

bool Router::emitir_ack_comunicacion_hasta_respuesta(uint16_t receptor, uint16_t nonce_original, uint8_t tipo_payload, unsigned intentos) {
    unsigned long tiempo_inicio;
    unsigned ciclos_a_esperar, tiempo_max_espera = 1, transmission_size;
    uint16_t _nonce = get_update_nonce();
    Mensaje_ack_comunicacion mensaje = Mensaje_ack_comunicacion(
        id, receptor, _nonce, nonce_original
    );
    bool confirmacion; // para saber si se pudo enviar el mensaje

    transmission_size = mensaje.get_transmission_size();
    unsigned char mensaje_a_enviar[transmission_size];
    mensaje.parse_to_transmission(mensaje_a_enviar);

    for (unsigned i = 0; i < intentos; i++) {

        Serial.println("----- Emitiendo ack comunicación en emitir_ack_comunicacion_hasta_respuesta -----");
        mensaje.print();
        Serial.println("----- Emitido ack comunicación en emitir_ack_comunicacion_hasta_respuesta -----");

        confirmacion = LoRa.beginPacket();
        if (confirmacion) {
            LoRa.write(mensaje_a_enviar, transmission_size);
            LoRa.endPacket(); // true = async / non-blocking mode
        }

        ciclos_a_esperar = confirmacion ? 2 : 0;// esperamos a 2 viajes del mensaje
        Serial.println("flag 2 emitir_ack_comunicacion_hasta_respuesta");
        tiempo_max_espera <<= 2;
        ciclos_a_esperar += ((unsigned)LoRa.random()) % (tiempo_max_espera);
        tiempo_inicio = millis();
        Serial.println("flag 3 emitir_ack_comunicacion_hasta_respuesta");
        while (millis() - tiempo_inicio <= ciclos_a_esperar * tiempo_transmision) {
            Serial.println("Escuchando en emitir_ack_comunicacion_hasta_respuesta");
            Serial.print("Receptor: ");
            Serial.println(receptor);
            if (!recibir_mensaje(1)) continue;
            Serial.println("flag 4 emitir_ack_comunicacion_hasta_respuesta");
            if (receptor != mensaje_pendiente.getEmisor()) continue;
            Serial.println("flag 5 emitir_ack_comunicacion_hasta_respuesta");
            if (tipo_payload == mensaje_pendiente.getTipoPayload()) return true;
            Serial.println("flag 6 emitir_ack_comunicacion_hasta_respuesta");
            if (Mensaje::PAYLOAD_ACK_COMUNICACION == mensaje_pendiente.getTipoPayload()) {
                Serial.println("flag 7 emitir_ack_comunicacion_hasta_respuesta");
                Mensaje_ack_comunicacion ack_com(mensaje_pendiente);
                Serial.println("flag 8 emitir_ack_comunicacion_hasta_respuesta");
                confirmacion = ack_com.confirmar_ack(_nonce, id, receptor);
                Serial.println("flag 9 emitir_ack_comunicacion_hasta_respuesta");
                if (confirmacion) return true;
            }
        }
    }
    return false;
}

bool Router::enviar_textos_ya_vistos(uint16_t receptor, unsigned periodos_espera) {
    uint16_t _nonce;
    std::vector<uint64_t> hashes = buffer.obtener_textos_ya_vistos(id);
    std::vector<Mensaje_textos_vistos> mensajes_a_enviar = Mensaje_textos_vistos::crear_mensajes(id, receptor, 0, hashes);

    for (std::vector<Mensaje_textos_vistos>::iterator mensaje = mensajes_a_enviar.begin(); mensaje != mensajes_a_enviar.end(); mensaje++) {
        _nonce = get_update_nonce();
        mensaje->setNonce(_nonce);

        Serial.println("----- Enviando mensajes ya vistos -----");
        mensaje->print();
        Serial.println("----- Enviando mensajes ya vistos -----");

        if (!enviar_mensaje(*mensaje, periodos_espera)) return false;
    }
    return true;
}