/**
 * SERIALIZER - Motor de Traducción Binaria
 * Mapea objetos JS a la estructura PacketRed de C (284 bytes exactos)
 * Basado en la alineación #pragma pack(1) del servidor_broker
 */

const PACKET_SIZE = 284;

const OFFSETS = {
    TIPO_OPERACION: 0,    // int (4 bytes)
    ESTADO: 4,            // int (4 bytes)
    ID_ESTUDIANTE: 8,     // int (4 bytes)
    EDAD: 12,             // int (4 bytes)
    ASISTENCIA: 16,       // int (4 bytes)
    USUARIO: 20,          // char[50]
    PASSWORD: 70,         // char[50]
    NOMBRE_ESTUDIANTE: 120, // char[114]
    CURSO: 234,           // char[30]
    TURNO: 264            // char[20]
};

const LENGTHS = {
    USUARIO: 50,
    PASSWORD: 50,
    NOMBRE_ESTUDIANTE: 114,
    CURSO: 30,
    TURNO: 20
};

const Serializer = {
    /**
     * Convierte un objeto JSON a un Buffer binario de 284 bytes
     */
    serialize: (data) => {
        const buffer = Buffer.alloc(PACKET_SIZE, 0); // Inicializar con ceros (null-terminated)

        // Escribir Integers (Little Endian para compatibilidad x86_64)
        buffer.writeInt32LE(data.tipo_operacion || 0, OFFSETS.TIPO_OPERACION);
        buffer.writeInt32LE(data.estado || 0, OFFSETS.ESTADO);
        buffer.writeInt32LE(data.id_estudiante || 0, OFFSETS.ID_ESTUDIANTE);
        buffer.writeInt32LE(data.edad || 0, OFFSETS.EDAD);
        buffer.writeInt32LE(data.asistencia || 0, OFFSETS.ASISTENCIA);

        // Escribir Strings (UTF-8)
        if (data.usuario) buffer.write(data.usuario, OFFSETS.USUARIO, LENGTHS.USUARIO, 'utf8');
        if (data.password) buffer.write(data.password, OFFSETS.PASSWORD, LENGTHS.PASSWORD, 'utf8');
        if (data.nombre_estudiante) buffer.write(data.nombre_estudiante, OFFSETS.NOMBRE_ESTUDIANTE, LENGTHS.NOMBRE_ESTUDIANTE, 'utf8');
        if (data.curso) buffer.write(data.curso, OFFSETS.CURSO, LENGTHS.CURSO, 'utf8');
        if (data.turno) buffer.write(data.turno, OFFSETS.TURNO, LENGTHS.TURNO, 'utf8');

        return buffer;
    },

    /**
     * Convierte un Buffer binario de 284 bytes a un objeto JSON legible
     */
    deserialize: (buffer) => {
        if (buffer.length < PACKET_SIZE) return null;

        const readString = (offset, length) => 
            buffer.toString('utf8', offset, offset + length).replace(/\0/g, '').trim();

        return {
            tipo_operacion: buffer.readInt32LE(OFFSETS.TIPO_OPERACION),
            estado: buffer.readInt32LE(OFFSETS.ESTADO),
            id_estudiante: buffer.readInt32LE(OFFSETS.ID_ESTUDIANTE),
            edad: buffer.readInt32LE(OFFSETS.EDAD),
            asistencia: buffer.readInt32LE(OFFSETS.ASISTENCIA),
            usuario: readString(OFFSETS.USUARIO, LENGTHS.USUARIO),
            password: readString(OFFSETS.PASSWORD, LENGTHS.PASSWORD),
            nombre_estudiante: readString(OFFSETS.NOMBRE_ESTUDIANTE, LENGTHS.NOMBRE_ESTUDIANTE),
            curso: readString(OFFSETS.CURSO, LENGTHS.CURSO),
            turno: readString(OFFSETS.TURNO, LENGTHS.TURNO)
        };
    }
};

module.exports = Serializer;