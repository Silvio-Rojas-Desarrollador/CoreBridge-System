#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

/* ===============================================================================
 * CONSTANTES DEL PROTOCOLO (Sincronización con el Servidor)
 * ===============================================================================
 */
#define MAX_USUARIO         50
#define MAX_PASSWORD        50
#define MAX_NOMBRE_ALUMNO   114
#define PUERTO_SERVIDOR     8080
#define ESTADO_EXITO        1
#define ESTADO_DENEGADO     0
#define ESTADO_ERROR       -1

/* 
 * Directiva de enlace para MSVC: Evita advertencias en GCC al compilar con PowerShell.
 */
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

/* ===============================================================================
 * DEFINICIÓN DE OPCODES (CONTRATO OFICIAL DEL SERVIDOR)
 * ===============================================================================
 * Valores extraídos de network.h para sincronización binaria.
 */
#define OP_LOGIN            101
#define OP_CREAR_ESTUDIANTE 102
#define OP_LEER_ESTUDIANTES 103
#define OP_EDITAR_ESTUDIANTE 104
#define OP_BORRAR_ESTUDIANTE 105
#define OP_SEND_MESSAGE      106
#define OP_BROADCAST_MESSAGE 107

/* ===============================================================================
 * DEFINICIÓN DEL PROTOCOLO DE RED (ESPEJO DEL SERVIDOR)
 * ===============================================================================
 * Forzamos alineación de 1 byte para que la estructura sea idéntica en red.
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
 * FUNCIÓN DEL HILO DE ESCUCHA (ASYNCHRONOUS LISTENER)
 * ===============================================================================
 */
DWORD WINAPI escuchar_servidor(LPVOID lpParam) {
    SOCKET socket_cliente = *(SOCKET *)lpParam;
    PacketRed paquete_entrante;
    int bytes;
    int total;

    while (1) {
        total = 0;
        memset(&paquete_entrante, 0, sizeof(PacketRed));

        // Bucle de integridad para recepción de paquetes del broker
        while (total < sizeof(PacketRed)) {
            bytes = recv(socket_cliente, (char *)&paquete_entrante + total, sizeof(PacketRed) - total, 0);
            if (bytes <= 0) return 0;
            total += bytes;
        }

        // Procesar solo si es un mensaje (Privado o Broadcast)
        if (paquete_entrante.tipo_operacion == OP_SEND_MESSAGE) {
            printf("\n\a[MENSAJE PRIVADO DE %s]: %s\n", paquete_entrante.usuario, paquete_entrante.nombre_estudiante);
            printf("► Seleccione una opcion: "); // Reimprimir prompt para UX
            fflush(stdout);
        } 
        else if (paquete_entrante.tipo_operacion == OP_BROADCAST_MESSAGE) {
            printf("\n\a[ANUNCIO GLOBAL DE %s]: %s\n", paquete_entrante.usuario, paquete_entrante.nombre_estudiante);
            printf("► Seleccione una opcion: ");
            fflush(stdout);
        }
        // Nota: OP_LEER_ESTUDIANTES se maneja en el hilo principal por ser síncrono a la consulta
    }
    return 0;
}

/* ===============================================================================
 * CLIENTE DE PRUEBA DE CONEXIÓN BINARIA
 * ===============================================================================
 */

static void limpiar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
    WSADATA wsa;
    SOCKET socket_cliente = INVALID_SOCKET;
    struct sockaddr_in servidor;
    PacketRed paquete_prueba;
    int bytes_recibidos;

    printf("[CLIENTE] Iniciando subsistema de red Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[ERROR CRITICO] No se pudo inicializar Winsock.\n");
        return EXIT_FAILURE;
    }

    // 1. Creación del socket simétrico de flujo (TCP)
    socket_cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_cliente == INVALID_SOCKET) {
        printf("[ERROR CRITICO] Error al crear el socket del cliente.\n");
        WSACleanup();
        return EXIT_FAILURE;
    }

    // 2. Configurar la dirección de destino (Tu propio servidor local)
    servidor.sin_family = AF_INET;
    servidor.sin_addr.s_addr = inet_addr("127.0.0.1"); // Dirección de bucle local (Loopback)
    servidor.sin_port = htons(PUERTO_SERVIDOR);       // Extraído automáticamente de config_server.h

    printf("[CLIENTE] Intentando establecer enlace TCP con el servidor en 192.168.1.1:%d...\n", PUERTO_SERVIDOR);
    
    // 3. Bloque de conexión activa (Apretón de manos)
    if (connect(socket_cliente, (struct sockaddr *)&servidor, sizeof(servidor)) == SOCKET_ERROR) {
        printf("[ERROR DE RED] No se pudo conectar al servidor.\n");
        printf("[INFO] Verificá que 'servidor_central.exe' esté corriendo en la otra consola.\n");
        closesocket(socket_cliente);
        WSACleanup();
        return EXIT_FAILURE;
    }
    
    printf("[OK] ¡Enlace establecido con el servidor con éxito!\n");
    printf("-------------------------------------------------------------------\n");

    // 4. Construcción defensiva del paquete binario rígido
    memset(&paquete_prueba, 0, sizeof(PacketRed));
    paquete_prueba.tipo_operacion = OP_LOGIN; // Indicamos al dispatcher qué queremos hacer
    
    printf("\n--- INICIO DE SESION REMOTA ---\n");
    printf("► Ingrese nombre de usuario: ");
    if (fgets(paquete_prueba.usuario, MAX_USUARIO, stdin) != NULL)
        paquete_prueba.usuario[strcspn(paquete_prueba.usuario, "\n")] = '\0';

    printf("► Ingrese clave de acceso: ");
    if (fgets(paquete_prueba.password, MAX_PASSWORD, stdin) != NULL)
        paquete_prueba.password[strcspn(paquete_prueba.password, "\n")] = '\0';

    printf("[CLIENTE] Empujando estructura PacketRed (%zu bytes) hacia el canal...\n", sizeof(PacketRed));
    send(socket_cliente, (const char *)&paquete_prueba, sizeof(PacketRed), 0);

    // 5. Espera bloqueante de la respuesta del servidor
    printf("[CLIENTE] Esperando el veredicto del Servidor Central...\n");
    memset(&paquete_prueba, 0, sizeof(PacketRed)); // Limpiamos el molde para recibir datos limpios
    
    bytes_recibidos = recv(socket_cliente, (char *)&paquete_prueba, sizeof(PacketRed), 0);

    if (bytes_recibidos > 0) {
        printf("-------------------------------------------------------------------\n");
        printf("[RESPUESTA DEL SERVIDOR DELIVERADA]\n");
        printf("[INFO] Operación devuelta: %d\n", paquete_prueba.tipo_operacion);
        printf("[INFO] Estado devuelto por el motor: %d\n", paquete_prueba.estado);
        
        if (paquete_prueba.estado == ESTADO_EXITO) {
            printf("[OK] ¡Credenciales validadas! El servidor aceptó la identidad.\n");
            printf("[INFO] Acceso concedido en modo: SOLO LECTURA.\n");

            // LANZAMIENTO DEL MOTOR DE ESCUCHA (Nivel Industrial)
            CreateThread(NULL, 0, escuchar_servidor, &socket_cliente, 0, NULL);

            int opcion_permanencia = 0;
            // BUCLE DE PERSISTENCIA: Mantiene el proceso activo y el socket abierto,
            // evitando que el programa llegue a la sección de clausura de red.
            while (1) {
                printf("\n=========================================\n");
                printf("       SISTEMA BROKER - PANEL ONLINE      \n");
                printf("=========================================\n");
                printf("  1. Ver lista de estudiantes (Solo Lectura)\n");
                printf("  2. Enviar mensaje privado\n");
                printf("  3. Enviar mensaje global (Broadcast)\n");
                printf("  4. Desconectarse y salir\n");
                printf("-----------------------------------------\n");
                printf("► Seleccione una opcion: ");

                if (scanf("%d", &opcion_permanencia) != 1) {
                    limpiar_buffer();
                    continue;
                }
                limpiar_buffer();

                if (opcion_permanencia == 1) {
                    // SOLICITUD DE LECTURA AL SERVIDOR
                    memset(&paquete_prueba, 0, sizeof(PacketRed));
                    paquete_prueba.tipo_operacion = OP_LEER_ESTUDIANTES;
                    
                    send(socket_cliente, (const char *)&paquete_prueba, sizeof(PacketRed), 0);
                    
                    printf("\n[CONSULTA] Recuperando registros de la base de datos central...\n");
                    printf("-------------------------------------------------------------------\n");
                    printf("%-4s | %-20s | %-4s | %-10s | %-10s | %-6s\n", "ID", "NOMBRE", "EDAD", "CURSO", "TURNO", "ASIST.");
                    printf("-------------------------------------------------------------------\n");

                    // BUCLE DE RECEPCIÓN DE FLUJO (Stream Loop)
                    while (1) {
                        int total_leido = 0;
                        memset(&paquete_prueba, 0, sizeof(PacketRed));

                        // BUCLE DE INTEGRIDAD: Asegura la llegada del bloque binario de 230 bytes
                        while (total_leido < sizeof(PacketRed)) {
                            bytes_recibidos = recv(socket_cliente, (char *)&paquete_prueba + total_leido, sizeof(PacketRed) - total_leido, 0);
                            if (bytes_recibidos <= 0) break;
                            total_leido += bytes_recibidos;
                        }

                        if (total_leido < sizeof(PacketRed)) {
                            printf("[ERROR] Se perdió la integridad del flujo de datos.\n");
                            break;
                        }

                        // CONDICIONAL CENTINELA: ¿Es el fin de la lista?
                        if (paquete_prueba.id_estudiante == -1) {
                            printf("-------------------------------------------------------------------\n");
                            printf("[INFO] Fin de la transmision de datos.\n");
                            break;
                        }

                        printf("%-4d | %-20.20s | %-4d | %-10.10s | %-10.10s | %-5d%%\n", 
                               paquete_prueba.id_estudiante, paquete_prueba.nombre_estudiante, 
                               paquete_prueba.edad, paquete_prueba.curso, paquete_prueba.turno, 
                               paquete_prueba.asistencia);
                    }
                    continue; // Regresa al menú de permanencia
                }

                if (opcion_permanencia == 2) {
                    memset(&paquete_prueba, 0, sizeof(PacketRed));
                    paquete_prueba.tipo_operacion = OP_SEND_MESSAGE;
                    
                    printf("► Destinatario (Usuario): ");
                    fgets(paquete_prueba.usuario, MAX_USUARIO, stdin);
                    paquete_prueba.usuario[strcspn(paquete_prueba.usuario, "\n")] = 0;
                    
                    printf("► Mensaje: ");
                    fgets(paquete_prueba.nombre_estudiante, MAX_NOMBRE_ALUMNO, stdin);
                    paquete_prueba.nombre_estudiante[strcspn(paquete_prueba.nombre_estudiante, "\n")] = 0;
                    
                    send(socket_cliente, (const char *)&paquete_prueba, sizeof(PacketRed), 0);
                    printf("[SISTEMA] Mensaje enviado al broker para su ruteo.\n");
                    continue;
                }

                if (opcion_permanencia == 3) {
                    memset(&paquete_prueba, 0, sizeof(PacketRed));
                    paquete_prueba.tipo_operacion = OP_BROADCAST_MESSAGE;
                    
                    printf("► Mensaje para todos: ");
                    fgets(paquete_prueba.nombre_estudiante, MAX_NOMBRE_ALUMNO, stdin);
                    paquete_prueba.nombre_estudiante[strcspn(paquete_prueba.nombre_estudiante, "\n")] = 0;
                    
                    send(socket_cliente, (const char *)&paquete_prueba, sizeof(PacketRed), 0);
                    printf("[SISTEMA] Difusion enviada con éxito.\n");
                    continue;
                }

                if (opcion_permanencia == 4) break; // Sale del bucle para cerrar el socket de forma segura
                printf("[INFO] El canal TCP sigue abierto. El servidor central esta a la espera...\n");
            }
        } else if (paquete_prueba.estado == ESTADO_DENEGADO) {
            printf("[ALERTA] Conexión exitosa, pero el servidor denegó el acceso (usuario/clave incorrectos).\n");
        } else {
            printf("[ERROR] El servidor reportó un fallo interno o base de datos inaccesible.\n");
        }
    } else {
        printf("[ERROR DE RED] El servidor cerró la conexión abruptamente o no respondió.\n");
    }

    // 6. Clausura limpia de la infraestructura de prueba
    printf("-------------------------------------------------------------------\n");
    printf("[CLIENTE] Desmantelando socket y saliendo de forma segura...\n");
    closesocket(socket_cliente);
    WSACleanup();

    return EXIT_SUCCESS;
}