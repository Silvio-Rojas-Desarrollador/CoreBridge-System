/**
 * AUTH.JS - Middleware de Seguridad (Capa 2)
 * Este componente actúa como el "firewall" de identidad.
 * Valida los tokens JWT emitidos por el Gateway para asegurar que solo 
 * peticiones legítimas lleguen a consumir recursos del Broker en C.
 */

const jwt = require('jsonwebtoken');

const authMiddleware = (req, res, next) => {
    const authHeader = req.headers['authorization'];
    
    // Formato esperado: "Bearer <JWT_TOKEN>"
    const token = authHeader && authHeader.split(' ')[1];

    if (!token) {
        return res.status(401).json({ 
            estado: -1, 
            mensaje: 'Acceso no autorizado: Se requiere un token de sesión.' 
        });
    }

    try {
        // En entorno industrial, el secreto debe estar en el archivo .env
        const secret = process.env.JWT_SECRET || 'MIT_LEVEL_SUPER_SECRET_KEY';
        const decoded = jwt.verify(token, secret);
        
        // Inyectamos la identidad decodificada en el objeto request para los controladores
        req.user = decoded;
        next();
    } catch (error) {
        return res.status(403).json({ 
            estado: -1, 
            mensaje: 'Token inválido o sesión expirada.' 
        });
    }
};

module.exports = authMiddleware;