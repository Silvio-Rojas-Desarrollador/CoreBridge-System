#ifndef CONFIG_SERVER_H
#define CONFIG_SERVER_H

/* ===============================================================================
 * 1. CONFIGURACIONES DE ENTORNO Y RED
 * ===============================================================================
 * Parámetros fundamentales para el ciclo de vida del socket del servidor.
 */
#define PUERTO_SERVIDOR      8080       // Puerto TCP de escucha central
#define MAX_CONEXIONES_COLA  10         // Límite de peticiones simultáneas en espera (backlog)
#define TAMANO_BUFFER_RED    4096       // Bloque de 4KB para optimizar la lectura de sockets
#define MAX_SESIONES         MAX_CONEXIONES_COLA // Capacidad máxima del Registry del Broker

/* ===============================================================================
 * 2. LÍMITES FÍSICOS DE LAS ESTRUCTURAS DE DATOS (ESTÁNDAR DE DEFENSA)
 * ===============================================================================
 * Definir tamaños fijos en bytes evita la fragmentación de memoria y es vital 
 * para el empaquetado binario rígido que viajará por la red.
 */
#define MAX_USUARIO          50         // Longitud máxima para nombres de usuario de acceso
#define MAX_PASSWORD         64         // Longitud máxima para contraseñas
#define MAX_NOMBRE_ALUMNO    100        // Capacidad del buffer para nombres completos de estudiantes
#define MAX_MENSAJE          MAX_NOMBRE_ALUMNO // Alias para el cuerpo del mensaje en el Broker

/* ===============================================================================
 * 3. RUTAS DE PERSISTENCIA DESACOPLADAS
 * ===============================================================================
 * Al ejecutarse el binario desde la carpeta raíz del servidor, estas rutas relativas 
 * garantizan el acceso directo al almacenamiento unificado sin romper la jerarquía.
 */
#define RUTA_BD_SQLITE       "../datos/archivo.db"
#define RUTA_BIN_ACCESOS     "../datos/usuarios.dat"

/* ===============================================================================
 * 4. CÓDIGOS DE ESTADO DEL PROTOCOLO DE APLICACIÓN
 * ===============================================================================
 * Códigos numéricos puros para el flujo de control. El servidor responderá con 
 * estos estados para que el cliente interprete el resultado sin ambigüedades.
 */
#define ESTADO_ERROR         0
#define ESTADO_EXITO         1
#define ESTADO_DENEGADO      2
#define ESTADO_NO_ENCONTRADO 3

#endif // CONFIG_SERVER_H