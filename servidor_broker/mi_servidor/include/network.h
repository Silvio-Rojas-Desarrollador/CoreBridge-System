#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include "sqlite3.h"
#include "config_server.h"

/* ===============================================================================
 * 1. CÓDIGOS DE OPERACIÓN DEL PROTOCOLO (OPCODES)
 * ===============================================================================
 * Define qué acción está solicitando el cliente remoto.
 */
#define OP_LOGIN             101
#define OP_CREAR_ESTUDIANTE  102
#define OP_LEER_ESTUDIANTES  103
#define OP_EDITAR_ESTUDIANTE 104
#define OP_BORRAR_ESTUDIANTE 105
#define OP_SEND_MESSAGE      106
#define OP_BROADCAST_MESSAGE 107

/* ===============================================================================
 * 2. ESTRUCTURA UNIFICADA DE PAQUETE DE RED
 * ===============================================================================
 * Bloque binario rígido. Toda comunicación entre cliente y servidor usará este 
 * molde exacto, evitando problemas de alineación de memoria en el socket.
 */
#pragma pack(push, 1)
typedef struct {
    int tipo_operacion;
    int estado;
    int id_estudiante;
    int edad;
    int asistencia;
    char usuario[MAX_USUARIO];
    char password[MAX_PASSWORD];
    char nombre_estudiante[MAX_NOMBRE_ALUMNO];
    char curso[30];
    char turno[20];
} PacketRed;
#pragma pack(pop)

/* ===============================================================================
 * 3. GESTIÓN DE SESIONES Y CONTEXTO DE HILOS
 * ===============================================================================
 */

/* Estructura para el registro global de clientes conectados (Broker Registry) */
typedef struct {
    SOCKET socket;
    char nombre_usuario[MAX_USUARIO];
    int activa;
} SesionCliente;

typedef struct {
    SOCKET socket_cliente;
    char nombre_usuario[MAX_USUARIO]; // Identidad vinculada al hilo tras el login
} ThreadArgs;

/* ===============================================================================
 * 3. PROTOTIPOS DE LAS FUNCIONES DE RED
 * ===============================================================================
 * Contrato de ejecución del demonio del servidor.
 */

/* * Inicializa la API de Winsock de Windows y monta el socket principal.
 * Retorna ESTADO_EXITO si el hardware de red responde o ESTADO_ERROR en fallo.
 */
int red_inicializar_servidor();

/* * Coloca al servidor en modo de escucha activa (bucle infinito).
 * Acepta las conexiones entrantes y las despacha para su procesamiento.
 */
void red_escuchar_conexiones(sqlite3 *db);

/* * Gestiona de forma aislada la sesión de un cliente conectado.
 * Recibe sus paquetes, valida credenciales y procesa las peticiones SQL.
 */
DWORD WINAPI red_manejar_cliente(LPVOID lpParam);

/* * Apaga los sockets activos y libera los recursos del sistema operativo.
 */
void red_limpiar_sistema();

/* * Funciones de gestión de sesiones (Broker Core)
 */
void sesiones_inicializar();
void sesiones_registrar(SOCKET s, const char *usuario);
void sesiones_eliminar(SOCKET s);
SOCKET sesiones_buscar_por_usuario(const char *usuario);
void sesiones_broadcast(SOCKET emisor, PacketRed *paquete);

#endif // NETWORK_H