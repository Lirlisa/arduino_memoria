#include <router/router.hpp>
#include <mensaje/mensaje/mensaje.hpp>
#include <mensaje/mensaje_texto/mensaje_texto.hpp>
#include <texto/texto.hpp>
#include <mensaje/mensaje_ack_comunicacion/mensaje_ack_comunicacion.hpp>
#include <mensaje/mensaje_vector/mensaje_vector.hpp>
#include <mensaje/mensaje_ack_mensaje/mensaje_ack_mensaje.hpp>
#include <mensaje/ack.hpp>
#include <LoRa.h>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <vector>
#include <cmath>


Router::Router(uint16_t _id, uint32_t _ttr, unsigned int initial_capacity) : buffer(_id) {
    id = _id;
    ttr = _ttr;
    nonce_actual = 0;
    mensajes_enviados = 0;
    mis_mensajes = new Texto[initial_capacity];
    capacidad_mis_mensajes = initial_capacity;
    size_mis_mensajes = 0;
    beacon_signal = Mensaje(
        ttr, id, BROADCAST_CHANNEL_ID, get_update_nonce(), Mensaje::PAYLOAD_BEACON, (unsigned char*)nullptr, 0
    );
}

Router::~Router() {
    if (capacidad_mis_mensajes > 0)
        delete[] mis_mensajes;
    mis_mensajes = nullptr;
}

bool Router::agregar_a_mis_mensajes(Texto& texto) {
    if (texto.destinatario != id) return false;

    if (!hay_espacio_en_mis_mensajes())
        if (!realocar_mis_mensajes())
            return false;

    mis_mensajes[size_mis_mensajes++] = texto;
    agregar_a_acks(texto);

    return true;
}

bool Router::agregar_a_mis_mensajes(std::vector<Texto>& textos) {
    if (!hay_espacio_en_mis_mensajes(textos.size()))
        if (!realocar_mis_mensajes(textos.size()))
            return false;

    for (std::vector<Texto>::iterator texto = textos.begin(); texto != textos.end(); texto++)
        mis_mensajes[size_mis_mensajes++] = *texto;

    buffer.agregar_ack(textos);
    return true;
}

bool Router::realocar_mis_mensajes(unsigned n) {
    Texto* new_mis_mensajes = new Texto[capacidad_mis_mensajes * 2 + n];
    for (unsigned i = 0; i < size_mis_mensajes; i++)
        new_mis_mensajes[i] = Texto(mis_mensajes[i]);

    delete[] mis_mensajes;
    mis_mensajes = new_mis_mensajes;
    capacidad_mis_mensajes = capacidad_mis_mensajes * 2 + n;
    return true;
}

bool Router::hay_espacio_en_mis_mensajes(unsigned n) {
    return capacidad_mis_mensajes - size_mis_mensajes >= n;
}

bool Router::agregar_a_acks(Texto& texto) {
    uint64_t llave = texto.hash();
    buffer.agregar_ack(llave);
    return true;
}

bool Router::enviar_mensaje_texto_maxprop(uint16_t receptor, unsigned periodos_espera) {
    if (!buffer.hay_mensajes_bajo_threshold()) return true;

    std::vector<Texto> textos = buffer.obtener_textos_bajo_threshold();
    std::vector<Mensaje_texto> mensajes = Mensaje_texto::crear_mensajes(ttr, id, receptor, 0, textos);
    uint16_t nonce;
    for (std::vector<Mensaje_texto>::iterator mensaje = mensajes.begin(); mensaje != mensajes.end(); mensaje++) {
        nonce = get_update_nonce();
        mensaje->setNonce(nonce);

        Serial.println("----- Enviando mensaje texto maxprop -----");
        mensaje->print();
        Serial.println("----- Enviando mensaje texto maxprop -----");

        if (!enviar_mensaje(*mensaje, periodos_espera)) return false;
    }
    return true;
}

/*
@brief Enviar en ttr significa que se envían los mensajes que estén sobre el threshold de saltos,
y que tengan una probabilidad de entrega mayor a la que este propio nodo tiene. Luego se eliminan.
*/
bool Router::enviar_mensaje_texto_ttr(uint16_t receptor, unsigned periodos_espera) {
    if (!buffer.hay_mensajes_sobre_threshold()) return true;

    std::vector<Texto> textos = buffer.obtener_textos_sobre_threshold();
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

bool Router::recibir_mensaje(unsigned periodos_espera) {
    for (unsigned intentos = 0; intentos < periodos_espera; intentos++) {
        if (!hay_paquete_LoRa(1)) continue;
        unsigned char data[Mensaje::raw_message_max_size];
        unsigned largo = 0;
        for (unsigned i = 0; i < Mensaje::raw_message_max_size && LoRa.available() > 0; i++) {
            data[i] = LoRa.read();
            largo++;
        }
        Mensaje* msg = Mensaje::parse_from_transmission(data, largo);
        if (mensaje_ya_visto(*msg) || msg->getEmisor() == id) { //evitar mensajes reflejados
            delete msg;
            continue;
        }
        agregar_a_ya_visto(*msg);

        Serial.println("----- Mensaje recibido -----");
        msg->print();
        Serial.println("----- Fin Mensaje recibido -----");

        if (msg->getReceptor() == id || msg->getReceptor() == BROADCAST_CHANNEL_ID) {
            mensaje_pendiente = Mensaje(*msg);
            hay_mensaje_pendiente = true;
            delete msg;
            return true;
        }
        else {
            if (msg->getTipoPayload() == Mensaje::PAYLOAD_VECTOR) {
                std::vector<par_costo_id> pares = Mensaje_vector(*msg).get_pares();
                buffer.agregar_probabilidades(msg->getEmisor(), pares); //eavesdropping por defecto
                hay_mensaje_pendiente = false;
            }
            else if (msg->getTipoPayload() == Mensaje::PAYLOAD_ACK_MENSAJE) {
                Mensaje_ack_mensaje msg(mensaje_pendiente);
                std::vector<uint64_t> acks = msg.obtener_acks();
                buffer.agregar_ack(acks); //eavesdropping por defecto
            }
        }

        delete msg;
    }
    return false;
}

uint16_t Router::get_update_nonce() {
    return nonce_actual++;
}

bool Router::emitir_beacon_signal() {
    beacon_signal.setNonce(get_update_nonce());
    beacon_signal.setTTR(ttr);
    unsigned transmission_size = beacon_signal.get_transmission_size();
    unsigned char data[transmission_size];
    beacon_signal.parse_to_transmission(data);

    if (LoRa.beginPacket()) {
        LoRa.write(data, transmission_size);
        LoRa.endPacket(); // true = async / non-blocking mode
        return true;
    }
    return false;
}

bool Router::emitir_ack_comunicacion(uint16_t receptor, uint16_t nonce_original) {
    Mensaje_ack_comunicacion mensaje = Mensaje_ack_comunicacion(
        id, receptor, get_update_nonce(), nonce_original
    );
    Serial.println("----- Emitiendo ack comunicación -----");
    mensaje.print();
    Serial.println("----- Emitido ack comunicación -----");

    unsigned char mensaje_a_enviar[mensaje.get_transmission_size()];
    mensaje.parse_to_transmission(mensaje_a_enviar);

    if (LoRa.beginPacket()) {
        LoRa.write(mensaje_a_enviar, mensaje.get_transmission_size());
        LoRa.endPacket(); // true = async / non-blocking mode
        return true;
    }
    return false;
}

/*
@brief Envia todos los mensajes dirigidos al destinatario
*/
bool Router::enviar_todos_a_destinatario(uint16_t destinatario) {
    std::vector<Texto> textos_para_destinatario = buffer.obtener_textos_destinatario(destinatario);
    std::vector<Mensaje_texto> mensajes_a_enviar = Mensaje_texto::crear_mensajes(
        ttr, id, destinatario, 0, textos_para_destinatario
    );

    std::vector<Mensaje_texto>::iterator mensaje;
    uint16_t nonce;
    unsigned textos_enviados = 0;
    for (mensaje = mensajes_a_enviar.begin(); mensaje != mensajes_a_enviar.end(); mensaje++) {
        nonce = get_update_nonce();
        mensaje->setNonce(nonce);

        Serial.println("----- Enviando mensaje todos a destinatario -----");
        mensaje->print();
        Serial.println("----- Fin Enviando mensaje todos a destinatario -----");
        if (!enviar_mensaje(*mensaje)) return false;

        textos_enviados += mensaje->get_cantidad_textos();
        std::vector<Texto> textos_a_borrar(
            textos_para_destinatario.begin() + textos_enviados - mensaje->get_cantidad_textos(),
            textos_para_destinatario.begin() + textos_enviados + 1
        );
        Serial.println("flag enviar_todos_a_destinatario 1");
        buffer.agregar_ack(textos_a_borrar);
        Serial.println("flag enviar_todos_a_destinatario 2");
    }
    return true;
}

bool Router::enviar_vectores_de_probabilidad(uint16_t receptor) {
    unsigned char data[buffer.get_size_vector_probabilidad()];
    buffer.obtener_vector_probabilidad(data);
    uint16_t cantidad_pares_restantes;
    std::memcpy(&cantidad_pares_restantes, data, 2);

    uint16_t cantidad_pares_a_enviar, cantidad_pares_enviados, cantidad_pares_enviados_totales = 0;
    uint8_t cantidad_bytes;
    unsigned char data_a_enviar[Mensaje_vector::payload_max_size];
    while (cantidad_pares_restantes > 0) {
        cantidad_pares_a_enviar = std::min((unsigned)(Mensaje_vector::payload_max_size - 1) / 6, (unsigned)cantidad_pares_restantes);
        cantidad_bytes = cantidad_pares_a_enviar * 6;
        data_a_enviar[0] = cantidad_bytes;
        cantidad_pares_enviados = 0;
        for (uint8_t i = 0; i < cantidad_pares_a_enviar; i++) {
            memcpy(
                data_a_enviar + 1 + cantidad_pares_enviados * 6,
                data + 2 + (cantidad_pares_enviados_totales + cantidad_pares_enviados) * 6,
                6
            );
            cantidad_pares_enviados++;
        }
        uint16_t nonce = get_update_nonce();
        Mensaje_vector mensaje = Mensaje_vector(
            id, receptor, nonce, data_a_enviar, 1 + cantidad_bytes
        );
        Serial.println("----- Enviando vector -----");
        mensaje.print();
        Serial.println("----- Enviado vector -----");

        if (!enviar_mensaje(mensaje)) return false;

        cantidad_pares_restantes -= cantidad_pares_a_enviar;
        cantidad_pares_enviados_totales += cantidad_pares_enviados;
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
    Serial.println("----- Paso 0 iniciar_comunicacion -----");
    buffer.actualizar_propias_probabilidades(receptor);

    int contador = 5;
    while (contador-- > 0) {
        if (!recibir_mensaje()) {
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
            continue;
        }
        if (receptor != mensaje_pendiente.getEmisor()) continue;
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_BEACON) {
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
            continue;
        }
        else if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_VECTOR) break;
        else {
            Serial.print("Termino porque recibí algo que no eran ni beacon ni vector, era un ");
            Serial.println(mensaje_pendiente.getTipoPayload());
            return;
        }
    }
    if (contador <= 0) {
        Serial.println("Termino porque no se recibió ningún mensaje 1");
        return;
    }

    uint32_t receptor_ttr = mensaje_pendiente.getTTR(), ttr_inicio = ttr; // para no comparar valores pasados con futuros

    Serial.println("----- Paso 1 iniciar_comunicacion -----");
    //esperamos los vectores de maxprop
    do {
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_VECTOR) {
            Serial.println("Recibí un vector");
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
            Mensaje_vector msg(mensaje_pendiente);
            std::vector<par_costo_id> pares = msg.get_pares();
            buffer.agregar_probabilidades(receptor, pares);
        }
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 2");
            return;
        };
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    // enviamos nuestros vectores
    if (!enviar_vectores_de_probabilidad(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los vectores");
        return;
    };
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());

    Serial.println("----- Paso 2 iniciar_comunicacion -----");
    //recibimos los ack de mensajes
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 3");
            return;
        }
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_ACK_MENSAJE) {
            Mensaje_ack_mensaje msg(mensaje_pendiente);
            std::vector<uint64_t> acks = msg.obtener_acks();
            buffer.agregar_ack(acks);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    //enviamos acks de mensajes
    if (!enviar_acks_mensajes(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los ack de mensajes");
        return;
    }
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());

    Serial.println("----- Paso 3 iniciar_comunicacion -----");
    //recibo mensajes de texto para mi
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 4");
            return;
        }
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            // buffer.agregar_ack(textos);
            agregar_a_mis_mensajes(textos);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    // enviamos mensajes de texto para el receptor
    if (!enviar_todos_a_destinatario(receptor)) {
        Serial.println("Termino porque no se pudieron enviar todos los mensajes para el destinatario");
        return;
    }
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());

    Serial.println("----- Paso 4 iniciar_comunicacion -----");
    // recibo textos en modo maxprop
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 5");
            return;
        };
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            buffer.agregar_texto(textos);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    //enviamos textos en modo maxprop
    if (!enviar_mensaje_texto_maxprop(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los mensajes en maxprop");
        return;
    };
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());

    Serial.println("----- Paso 5 iniciar_comunicacion -----");
    //recibo textos en modo ttr
    do {
        if (!recibir_mensaje()) return;
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            buffer.agregar_texto(textos);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
    //enviar textos en modo ttr
    if (receptor_ttr < ttr_inicio) {
        if (!enviar_mensaje_texto_ttr(receptor)) return;
    }
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
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
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
    //esperamos los vectores de maxprop
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 1");
            return;
        }
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_VECTOR) {
            Serial.println("Recibí un vector");
            Mensaje_vector msg(mensaje_pendiente);
            std::vector<par_costo_id> pares = msg.get_pares();
            buffer.agregar_probabilidades(receptor, pares);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);

    Serial.println("----- Paso 2 recibir_comunicacion -----");
    //enviamos acks de mensajes
    if (!enviar_acks_mensajes(receptor)) {
        Serial.println("Termino porque no se pudieron enviar los ack de mensajes");
        return;
    };
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
    //recibimos los ack de mensajes
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 2");
            return;
        }
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_ACK_MENSAJE) {
            Mensaje_ack_mensaje msg(mensaje_pendiente);
            std::vector<uint64_t> acks = msg.obtener_acks();
            buffer.agregar_ack(acks);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);

    Serial.println("----- Paso 3 recibir_comunicacion -----");
    // enviamos mensajes de texto para el receptor
    if (!enviar_todos_a_destinatario(receptor)) {
        Serial.println("Termino porque no se pudieron enviar todos a destinatario");
        return;
    }
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
    //recibo mensajes de texto para mi
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 3");
            return;
        }
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            // buffer.agregar_ack(textos);
            agregar_a_mis_mensajes(textos);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);

    Serial.println("----- Paso 4 recibir_comunicacion -----");
    //enviamos textos en modo maxprop
    if (!enviar_mensaje_texto_maxprop(receptor)) {
        Serial.println("Termino porque no se pudieron enviar mensajes maxprop");
        return;
    }
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
    // recibo textos en modo maxprop
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 4");
            return;
        }
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            buffer.agregar_texto(textos);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);

    Serial.println("----- Paso 5 recibir_comunicacion -----");
    //enviamos textos en modo ttr
    if (receptor_ttr < ttr_inicio) {
        if (!enviar_mensaje_texto_ttr(receptor)) {
            Serial.println("Termino porque no se pudieron enviar mensajes ttr");
            return;
        }
    }
    emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
    //recibo textos en modo ttr
    do {
        if (!recibir_mensaje()) {
            Serial.println("Termino porque no se recibió ningún mensaje 5");
            return;
        }
        if (mensaje_pendiente.getTipoPayload() == Mensaje::PAYLOAD_TEXTO) {
            Mensaje_texto msg(mensaje_pendiente);
            std::vector<Texto> textos = msg.obtener_textos();
            buffer.agregar_texto(textos);
            emitir_ack_comunicacion(receptor, mensaje_pendiente.getNonce());
        }
    } while (mensaje_pendiente.getTipoPayload() != Mensaje::PAYLOAD_ACK_COMUNICACION);
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
    ttr = ttr > segundos_transcurridos ? ttr - segundos_transcurridos : 0;
}

void Router::print_buffer() {
    buffer.print();
}

void Router::print_mapa() {
    buffer.print_mapa();
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

void Router::eliminar_de_ya_vistos(const std::vector<Texto>& textos) {
    std::unordered_set<uint64_t>::iterator pos;
    for (std::vector<Texto>::const_iterator texto = textos.begin(); texto != textos.end(); texto++) {
        pos = ya_vistos.find(texto->hash());
        if (pos != ya_vistos.end())
            ya_vistos.erase(pos);
    }
}

void Router::liberar_memoria() {
    eliminar_de_ya_vistos(buffer.obtener_textos_sobre_threshold());
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
                bool confirmacion = Mensaje_ack_comunicacion(mensaje_pendiente).confirmar_ack(
                    mensaje.getNonce(),
                    mensaje.getEmisor(),
                    mensaje.getReceptor()
                );
                Serial.print("Confirmacion: ");
                Serial.println(confirmacion ? "true" : "false");
                if (confirmacion) return true;
            }
        }
    }
    return false;
}