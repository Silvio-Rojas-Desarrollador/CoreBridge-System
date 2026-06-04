/**
 * STUDENT CONTROLLER - Gestión de Datos de Estudiantes
 * Implementa acceso de solo lectura al motor de base de datos en C.
 */

const TcpClient = require('../network/tcpClient');
const Opcodes = require('../bridge/opcodes');

const studentController = {
    /**
     * Recupera la lista completa de estudiantes desde el Broker.
     * El TcpClient se encarga de recolectar el flujo binario hasta el centinela.
     */
    getAll: async (req, res) => {
        try {
            const payload = {
                tipo_operacion: Opcodes.OP_LEER_ESTUDIANTES
            };

            const students = await TcpClient.request(payload);

            return res.json({
                estado: Opcodes.ESTADO_EXITO,
                data: students
            });
        } catch (error) {
            console.error('[STUDENT-CTRL] Error al obtener estudiantes:', error.message);
            return res.status(500).json({
                estado: Opcodes.ESTADO_ERROR,
                mensaje: 'Error de comunicación con el motor central de datos.'
            });
        }
    }
};

module.exports = studentController;