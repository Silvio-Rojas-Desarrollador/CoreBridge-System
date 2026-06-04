/**
 * TCP CLIENT - Motor de Comunicación con el Broker en C
 * Gestiona el ciclo de vida del socket, la integridad de los paquetes de 284 bytes
 * y la recolección de flujos de datos (streams) para la lista de estudiantes.
 */

const net = require('net');
const Serializer = require('../bridge/serializer');
const Opcodes = require('../bridge/opcodes');

// Configuración (Posteriormente se integrará con variables de entorno .env)
const BROKER_CONFIG = {
    host: process.env.BROKER_HOST || '127.0.0.1',
    port: parseInt(process.env.BROKER_PORT) || 8080,
    packetSize: 284
};

class TcpClient {
    constructor() {
        this.client = new net.Socket();
        this.isConnected = false;
        this.dataBuffer = Buffer.alloc(0);

        // Manejo global de cierre de socket para evitar estados inconsistentes
        this.client.on('close', () => {
            this.isConnected = false;
            this.dataBuffer = Buffer.alloc(0);
            console.warn('[TCP] Conexión cerrada con el Broker Central.');
        });
    }

    /**
     * Establece el enlace físico con el servidor_broker
     */
    async connect() {
        return new Promise((resolve, reject) => {
            if (this.isConnected) return resolve();

            this.client.connect(BROKER_CONFIG.port, BROKER_CONFIG.host, () => {
                this.isConnected = true;
                console.log(`[TCP] Enlace establecido con el Broker en ${BROKER_CONFIG.host}:${BROKER_CONFIG.port}`);
                resolve();
            });

            this.client.once('error', (err) => {
                console.error('[TCP] Error de conexión:', err.message);
                reject(err);
            });
        });
    }

    /**
     * Orquesta el envío de una petición y la captura de la respuesta.
     * Implementa un bucle de integridad para manejar la fragmentación de bytes.
     */
    async request(payload) {
        if (!this.isConnected) await this.connect();

        return new Promise((resolve, reject) => {
            const results = [];
            
            const onData = (chunk) => {
                // Acumulamos los bytes recibidos en el buffer de la sesión
                this.dataBuffer = Buffer.concat([this.dataBuffer, chunk]);

                // Procesar mientras haya al menos un paquete completo de 284 bytes
                while (this.dataBuffer.length >= BROKER_CONFIG.packetSize) {
                    const packetBuffer = this.dataBuffer.subarray(0, BROKER_CONFIG.packetSize);
                    this.dataBuffer = this.dataBuffer.subarray(BROKER_CONFIG.packetSize);

                    const decodedResponse = Serializer.deserialize(packetBuffer);

                    // Lógica para OP_LEER_ESTUDIANTES: Recolectar hasta el paquete centinela (ID -1)
                    if (payload.tipo_operacion === Opcodes.OP_LEER_ESTUDIANTES) {
                        if (decodedResponse.id_estudiante === -1) {
                            this.client.removeListener('data', onData);
                            return resolve(results);
                        }
                        results.push(decodedResponse);
                    } else {
                        // Respuesta atómica (Login)
                        this.client.removeListener('data', onData);
                        return resolve(decodedResponse);
                    }
                }
            };

            this.client.on('data', onData);
            this.client.write(Serializer.serialize(payload));
        });
    }
}

module.exports = new TcpClient();