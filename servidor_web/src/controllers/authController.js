/**
 * AUTH CONTROLLER - Gestión de Identidad Digital
 * Actúa como intermediario entre las peticiones HTTP y el Broker de C.
 * Valida credenciales binariamente y emite tokens JWT para la sesión web.
 */

const TcpClient = require('../network/tcpClient');
const Opcodes = require('../bridge/opcodes');
const jwt = require('jsonwebtoken');

const authController = {
    /**
     * Maneja el flujo de inicio de sesión.
     * Traduce el JSON de la web a un paquete PacketRed para validación en C.
     */
    login: async (req, res) => {
        const { usuario, password } = req.body;

        if (!usuario || !password) {
            return res.status(400).json({ 
                estado: Opcodes.ESTADO_ERROR, 
                mensaje: 'Usuario y contraseña son obligatorios.' 
            });
        }

        try {
            // Preparar el payload para el Broker (Capa 3)
            const payload = {
                tipo_operacion: Opcodes.OP_LOGIN,
                usuario: usuario,
                password: password
            };

            // Invocación al Bridge TCP (Espera bloqueante de la respuesta binaria)
            const response = await TcpClient.request(payload);

            if (response && response.estado === Opcodes.ESTADO_EXITO) {
                // Generación de Token JWT industrial si el motor de C dio el visto bueno
                const secret = process.env.JWT_SECRET || 'MIT_LEVEL_SUPER_SECRET_KEY';
                const token = jwt.sign(
                    { usuario: usuario }, 
                    secret, 
                    { expiresIn: '12h' } // Sesión extendida para entorno de trabajo
                );

                return res.json({
                    estado: Opcodes.ESTADO_EXITO,
                    mensaje: 'Sesión autorizada por el Servidor Central.',
                    token: token
                });
            } else {
                return res.status(401).json({
                    estado: Opcodes.ESTADO_DENEGADO,
                    mensaje: 'Usuario o contraseña incorrectos.'
                });
            }

        } catch (error) {
            console.error('[AUTH-CTRL] Error en comunicación con Broker:', error.message);
            return res.status(500).json({
                estado: Opcodes.ESTADO_ERROR,
                mensaje: 'El servicio de autenticación central no responde.'
            });
        }
    }
};

module.exports = authController;