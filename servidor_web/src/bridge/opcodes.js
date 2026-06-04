/**
 * OPCODES - Contrato Oficial del Protocolo Binario
 * Sincronizado con servidor_broker/mi_servidor/include/network.h
 */

const Opcodes = {
    // Operaciones de Base de Datos
    OP_LOGIN: 101,
    OP_CREAR_ESTUDIANTE: 102,
    OP_LEER_ESTUDIANTES: 103,
    OP_EDITAR_ESTUDIANTE: 104,
    OP_BORRAR_ESTUDIANTE: 105,
    
    // Operaciones de Mensajería (Broker)
    OP_SEND_MESSAGE: 106,
    OP_BROADCAST_MESSAGE: 107,

    // Estados de Respuesta
    ESTADO_EXITO: 1,
    ESTADO_DENEGADO: 0,
    ESTADO_ERROR: -1
};

module.exports = Opcodes;