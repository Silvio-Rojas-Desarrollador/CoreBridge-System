#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "config_server.h"
#include "sqlite3.h"
#include "database.h"
#include "auth.h"
#include "network.h"

// Descriptor global estático para el control del socket principal
static SOCKET socket_servidor = INVALID_SOCKET;

// Infraestructura del Broker: Tabla de sesiones y sincronización
static SesionCliente tabla_sesiones[MAX_CONEXIONES_COLA];
static CRITICAL_SECTION cs_sesiones;

/* ===============================================================================
 * FUNCIÓN AUXILIAR (CALLBACK): TRANSMISIÓN DE FILAS HACIA LA RED
 * ===============================================================================
 * Se dispara por cada registro encontrado en database.c. Empaqueta el alumno 
 * actual y lo inyecta directamente al flujo de red del cliente.
 */
static void enviar_estudiante_callback(int id, const char *nombre, int edad, const char *curso, const char *turno, void *contexto_red) {
    SOCKET socket_cliente = *(SOCKET *)contexto_red;
    PacketRed paquete_fila;
    
    memset(&paquete_fila, 0, sizeof(PacketRed));
    paquete_fila.tipo_operacion = OP_LEER_ESTUDIANTES;
    paquete_fila.estado = ESTADO_EXITO;
    paquete_fila.id_estudiante = id;
    paquete_fila.edad = edad;
    paquete_fila.asistencia = 0; // Valor de relleno para mantener compatibilidad binaria
    
    // Copia defensiva limitada por la macro para evitar desbordamientos
    strncpy(paquete_fila.nombre_estudiante, nombre, MAX_NOMBRE_ALUMNO - 1);
    paquete_fila.nombre_estudiante[MAX_NOMBRE_ALUMNO - 1] = '\0';

    if (curso) strncpy(paquete_fila.curso, curso, 29);
    if (turno) strncpy(paquete_fila.turno, turno, 19);
    
    // Verificar el resultado de send para detectar desconexiones prematuras
    if (send(socket_cliente, (const char *)&paquete_fila, sizeof(PacketRed), 0) == SOCKET_ERROR) {
        // En un callback, no podemos romper el bucle principal del hilo,
        // pero podemos registrar el error. El hilo principal detectará la desconexión en el siguiente recv.
        fprintf(stderr, "[ERROR] Fallo al enviar paquete de estudiante a socket %d. Posible desconexión.\n", socket_cliente);
    }
}

/* ===============================================================================
 * 1. INICIALIZACIÓN DE LA INFRAESTRUCTURA DE RED
 * ===============================================================================
 */
int red_inicializar_servidor() {
    WSADATA wsa;
    struct sockaddr_in servidor;
    int opcion_reuso = 1;

    // 1. Inicializar el subsistema Winsock de Windows
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return ESTADO_ERROR;
    }

    // Inicializar el monitor de sincronización del Broker
    sesiones_inicializar();

    // 2. Crear el socket de escucha TCP orientado a flujos (SOCK_STREAM)
    socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor == INVALID_SOCKET) {
        WSACleanup();
        return ESTADO_ERROR;
    }

    // ACTIVACIÓN DE REUSO: Permite reiniciar el servidor sin esperar a que el SO libere el puerto
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, (const char*)&opcion_reuso, sizeof(opcion_reuso));

    // 3. Configurar los parámetros de enlace del hardware de red
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = INADDR_ANY; // Escuchar en cualquier interfaz activa
    servidor.sin_port = htons(PUERTO_SERVIDOR);

    // 4. Vincular el socket al puerto físico especificado
    if (bind(socket_servidor, (struct sockaddr *)&servidor, sizeof(servidor)) == SOCKET_ERROR) {
        closesocket(socket_servidor);
        WSACleanup();
        return ESTADO_ERROR;
    }

    // 5. Establecer la cola de espera para peticiones simultáneas
    if (listen(socket_servidor, MAX_CONEXIONES_COLA) == SOCKET_ERROR) {
        closesocket(socket_servidor);
        WSACleanup();
        return ESTADO_ERROR;
    }

    return ESTADO_EXITO;
}

/* ===============================================================================
 * 2. BUCLE DE ESCUCHA CENTRAL
 * ===============================================================================
 */
void red_escuchar_conexiones(sqlite3 *db) {
    struct sockaddr_in cliente;
    int c_len = sizeof(struct sockaddr_in);
    SOCKET socket_cliente;

    // Ciclo infinito del demonio de red
    while (1) {
        // Bloquea el hilo hasta que un cliente remoto inicie un apretón de manos (Handshake)
        socket_cliente = accept(socket_servidor, (struct sockaddr *)&cliente, &c_len);
        if (socket_cliente != INVALID_SOCKET) {
            printf("[INFO] Cliente conectado desde %s\n", inet_ntoa(cliente.sin_addr));
            
            // Asignar memoria para los argumentos del hilo (se libera dentro del hilo)
            ThreadArgs *args = (ThreadArgs *)malloc(sizeof(ThreadArgs));
            if (args != NULL) {
                args->socket_cliente = socket_cliente;
                memset(args->nombre_usuario, 0, MAX_USUARIO);
                // Despachar el cliente a un hilo independiente y continuar escuchando
                CreateThread(NULL, 0, red_manejar_cliente, args, 0, NULL);
            }
        }
    }
}

/* ===============================================================================
 * 3. DESPACHADOR DE PROTOCOLO DE APLICACIÓN (DISPATCHER)
 * ===============================================================================
 */
DWORD WINAPI red_manejar_cliente(LPVOID lpParam) {
    ThreadArgs *args = (ThreadArgs *)lpParam;
    SOCKET socket_cliente = args->socket_cliente;
    sqlite3 *db = NULL;
    SOCKET target_socket = INVALID_SOCKET;
    PacketRed paquete;
    int keepAlive = 1;
    int keepIdle = 10; // Segundos de inactividad antes de enviar el primer keepalive
    int keepInterval = 5; // Segundos entre keepalives
    int keepCount = 3; // Número de keepalives fallidos antes de cerrar la conexión

    int bytes_sent; // Variable para almacenar el resultado de send
    int bytes_recibidos;
    int total_leido;

    // Cada hilo abre su propia conexión a la base de datos para máxima concurrencia
    if (db_inicializar_sistema(&db) != ESTADO_EXITO) {
        closesocket(socket_cliente);
        free(args);
        return 1;
    }
    
    // Habilitar SO_KEEPALIVE para detectar clientes inactivos
    // Esto ayuda a que el servidor detecte clientes muertos sin esperar un recv fallido.
    setsockopt(socket_cliente, SOL_SOCKET, SO_KEEPALIVE, (char *)&keepAlive, sizeof(keepAlive));
    // Configuración específica de TCP Keep-Alive para Windows (opcional, pero mejora la detección)
    // Nota: Estas opciones pueden variar o no estar disponibles en todos los sistemas operativos.
    // Para Windows, se usan SIO_KEEPALIVE_VALS con WSAIoctl.
    // Por simplicidad y portabilidad, nos quedamos con SO_KEEPALIVE estándar por ahora.
    // setsockopt(socket_cliente, IPPROTO_TCP, TCP_KEEPALIVE_INTERVAL, (char *)&keepInterval, sizeof(keepInterval)); // No estándar
    // setsockopt(socket_cliente, IPPROTO_TCP, TCP_KEEPALIVE_TIME, (char *)&keepIdle, sizeof(keepIdle)); // No estándar

    // Mantiene la sesión viva mientras el cliente envíe estructuras completas
    while (1) {
        memset(&paquete, 0, sizeof(PacketRed));
        total_leido = 0;

        // BUCLE DE INTEGRIDAD: Asegura que se reciban los 230 bytes exactos de la estructura
        while (total_leido < sizeof(PacketRed)) {
            bytes_recibidos = recv(socket_cliente, (char *)&paquete + total_leido, sizeof(PacketRed) - total_leido, 0);
            
            if (bytes_recibidos <= 0) {
                // Si hay desconexión o error, cerramos la sesión del cliente
                printf("[INFO] Cliente desconectado.\n");
                sesiones_eliminar(socket_cliente);
                db_cerrar_sistema(db);
                closesocket(socket_cliente);
                free(args);
                return 0;
            }
            total_leido += bytes_recibidos;
        }
        
        // Si hay error en el canal o desconexión física (0 bytes), cerrar sesión
        if (bytes_recibidos <= 0) {
            break;
        }

        // Enrutamiento operativo según el OPCODE recibido
        switch (paquete.tipo_operacion) {
            
            case OP_LOGIN:
                paquete.estado = auth_verificar_credenciales(paquete.usuario, paquete.password);
                if (paquete.estado == ESTADO_EXITO) {
                    // Vincular socket e identidad en el Broker
                    strncpy(args->nombre_usuario, paquete.usuario, MAX_USUARIO - 1);
                    sesiones_registrar(socket_cliente, paquete.usuario);
                }
                bytes_sent = send(socket_cliente, (const char *)&paquete, sizeof(PacketRed), 0);
                if (bytes_sent == SOCKET_ERROR) {
                    fprintf(stderr, "[ERROR] Fallo al enviar respuesta de LOGIN a socket %d. Desconectando.\n", socket_cliente);
                    break; // Romper el bucle para cerrar el socket
                }
                break;

            case OP_CREAR_ESTUDIANTE:
                paquete.estado = db_insertar_estudiante(db, 
                                    paquete.nombre_estudiante, 
                                    paquete.edad, 
                                    paquete.curso, 
                                    paquete.turno);
                bytes_sent = send(socket_cliente, (const char *)&paquete, sizeof(PacketRed), 0);
                if (bytes_sent == SOCKET_ERROR) {
                    fprintf(stderr, "[ERROR] Fallo al enviar respuesta de CREAR_ESTUDIANTE a socket %d. Desconectando.\n", socket_cliente);
                    break;
                }
                break;

            case OP_LEER_ESTUDIANTES:
                // Invoca la extracción. El callback transmitirá fila por fila
                db_listar_estudiantes(db, enviar_estudiante_callback, &socket_cliente);
                
                // Enviar un paquete centinela final para avisar al cliente que la lista terminó
                memset(&paquete, 0, sizeof(PacketRed));
                paquete.tipo_operacion = OP_LEER_ESTUDIANTES;
                paquete.estado = ESTADO_EXITO;
                paquete.id_estudiante = -1; // Indicador lógico de fin de transmisión (EOF de red)
                bytes_sent = send(socket_cliente, (const char *)&paquete, sizeof(PacketRed), 0);
                if (bytes_sent == SOCKET_ERROR) {
                    fprintf(stderr, "[ERROR] Fallo al enviar centinela de LEER_ESTUDIANTES a socket %d. Desconectando.\n", socket_cliente);
                    break;
                }
                break;

            case OP_EDITAR_ESTUDIANTE:
                paquete.estado = db_editar_estudiante(db, 
                                    paquete.id_estudiante, 
                                    paquete.nombre_estudiante, 
                                    paquete.edad, 
                                    paquete.curso, 
                                    paquete.turno);
                bytes_sent = send(socket_cliente, (const char *)&paquete, sizeof(PacketRed), 0);
                if (bytes_sent == SOCKET_ERROR) {
                    fprintf(stderr, "[ERROR] Fallo al enviar respuesta de EDITAR_ESTUDIANTE a socket %d. Desconectando.\n", socket_cliente);
                    break;
                }
                break;

            case OP_BORRAR_ESTUDIANTE:
                paquete.estado = db_borrar_estudiante(db, paquete.id_estudiante);
                bytes_sent = send(socket_cliente, (const char *)&paquete, sizeof(PacketRed), 0);
                if (bytes_sent == SOCKET_ERROR) {
                    fprintf(stderr, "[ERROR] Fallo al enviar respuesta de BORRAR_ESTUDIANTE a socket %d. Desconectando.\n", socket_cliente);
                    break;
                }
                break;
            case OP_SEND_MESSAGE:
                // Lógica de Broker: Enrutamiento privado
                target_socket = sesiones_buscar_por_usuario(paquete.usuario);
                if (target_socket != INVALID_SOCKET) {
                    // Inyectamos el remitente en el campo 'usuario' para que el receptor sepa quién envía
                    strncpy(paquete.usuario, args->nombre_usuario, MAX_USUARIO - 1);
                    paquete.estado = ESTADO_EXITO;
                    bytes_sent = send(target_socket, (const char *)&paquete, sizeof(PacketRed), 0);
                    if (bytes_sent == SOCKET_ERROR) {
                        fprintf(stderr, "[ERROR] Fallo al reenviar mensaje privado a socket %d. (Destinatario)\n", target_socket);
                        // No rompemos el bucle aquí, ya que el fallo es para el destinatario, no para el emisor actual.
                    }
                } else {
                    paquete.estado = ESTADO_NO_ENCONTRADO;
                    bytes_sent = send(socket_cliente, (const char *)&paquete, sizeof(PacketRed), 0);
                    if (bytes_sent == SOCKET_ERROR) {
                        fprintf(stderr, "[ERROR] Fallo al enviar respuesta de NO_ENCONTRADO a socket %d. Desconectando.\n", socket_cliente);
                        break;
                    }
                }
                break;
            case OP_BROADCAST_MESSAGE:
                // Lógica de Broker: Difusión masiva
                strncpy(paquete.usuario, args->nombre_usuario, MAX_USUARIO - 1);
                paquete.estado = ESTADO_EXITO;
                sesiones_broadcast(socket_cliente, &paquete);
                break;

            default:
                // Manejo defensivo ante paquetes corruptos o códigos inválidos
                paquete.estado = ESTADO_ERROR;
                bytes_sent = send(socket_cliente, (const char *)&paquete, sizeof(PacketRed), 0);
                if (bytes_sent == SOCKET_ERROR) {
                    fprintf(stderr, "[ERROR] Fallo al enviar respuesta de ERROR por opcode inválido a socket %d. Desconectando.\n", socket_cliente);
                    break;
                }
                break;
        }
    }

    // Clausura del descriptor de sesión al romper el bucle
    sesiones_eliminar(socket_cliente);
    db_cerrar_sistema(db);
    closesocket(socket_cliente);
    free(args);
    return 0;
}

/* ===============================================================================
 * 4. DESMONTAJE SEGURO DE RED
 * ===============================================================================
 */
void red_limpiar_sistema() {
    if (socket_servidor != INVALID_SOCKET) {
        closesocket(socket_servidor);
        socket_servidor = INVALID_SOCKET;
    }
    // Liberar los recursos de red asignados por el sistema operativo
    DeleteCriticalSection(&cs_sesiones);
    WSACleanup();
}

/* ===============================================================================
 * 5. IMPLEMENTACIÓN DEL CORE DEL BROKER (SESSION MANAGEMENT)
 * ===============================================================================
 */

void sesiones_inicializar() {
    InitializeCriticalSection(&cs_sesiones);
    memset(tabla_sesiones, 0, sizeof(tabla_sesiones));
}

void sesiones_registrar(SOCKET s, const char *usuario) {
    EnterCriticalSection(&cs_sesiones);
    for (int i = 0; i < MAX_CONEXIONES_COLA; i++) {
        if (!tabla_sesiones[i].activa) {
            tabla_sesiones[i].socket = s;
            strncpy(tabla_sesiones[i].nombre_usuario, usuario, MAX_USUARIO - 1);
            tabla_sesiones[i].activa = 1;
            break;
        }
    }
    LeaveCriticalSection(&cs_sesiones);
}

void sesiones_eliminar(SOCKET s) {
    EnterCriticalSection(&cs_sesiones);
    for (int i = 0; i < MAX_CONEXIONES_COLA; i++) {
        if (tabla_sesiones[i].activa && tabla_sesiones[i].socket == s) {
            tabla_sesiones[i].activa = 0;
            break;
        }
    }
    LeaveCriticalSection(&cs_sesiones);
}

SOCKET sesiones_buscar_por_usuario(const char *usuario) {
    SOCKET s = INVALID_SOCKET;
    EnterCriticalSection(&cs_sesiones);
    for (int i = 0; i < MAX_CONEXIONES_COLA; i++) {
        if (tabla_sesiones[i].activa && strcmp(tabla_sesiones[i].nombre_usuario, usuario) == 0) {
            s = tabla_sesiones[i].socket;
            break;
        }
    }
    LeaveCriticalSection(&cs_sesiones);
    return s;
}

void sesiones_broadcast(SOCKET emisor, PacketRed *paquete) {
    EnterCriticalSection(&cs_sesiones);
    for (int i = 0; i < MAX_CONEXIONES_COLA; i++) {
        if (tabla_sesiones[i].activa && tabla_sesiones[i].socket != emisor) {
            send(tabla_sesiones[i].socket, (const char *)paquete, sizeof(PacketRed), 0);
        }
    }
    LeaveCriticalSection(&cs_sesiones);
}