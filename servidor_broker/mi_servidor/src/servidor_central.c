#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "config_server.h"
#include "database.h"
#include "network.h"

/* ===============================================================================
 * PUNTO DE ENTRADA CENTRAL DEL SERVIDOR
 * ===============================================================================
 */
int main() {
    // Instancia central de la base de datos (manejador opaco de SQLite)
    sqlite3 *db = NULL;
    int estado_sistema;

    printf("[SISTEMA] Iniciando secuencia de arranque del Servidor Central...\n");

    /* ---------------------------------------------------------------------------
     * FASE 1: Inicialización del Motor Relacional (SQLite)
     * ---------------------------------------------------------------------------
     * La persistencia es prioritaria. Si la base de datos no puede asegurar su
     * esquema físico en disco, el servidor aborta inmediatamente.
     */
    estado_sistema = db_inicializar_sistema(&db);
    if (estado_sistema == ESTADO_ERROR) {
        printf("[ERROR CRITICO] No se pudo inicializar la base de datos relacional.\n");
        printf("[SISTEMA] Abortando arranque para proteger la integridad de los datos.\n");
        return EXIT_FAILURE;
    }
    printf("[OK] Motor de base de datos inicializado y tabla verificado.\n");

    /* ---------------------------------------------------------------------------
     * FASE 2: Inicialización de la Infraestructura de Red (Winsock2)
     * ---------------------------------------------------------------------------
     * Montaje del socket de escucha TCP. Si el hardware de red o el puerto están
     * bloqueados, se liberan los recursos de la Fase 1 antes de salir.
     */
    estado_sistema = red_inicializar_servidor();
    if (estado_sistema == ESTADO_ERROR) {
        printf("[ERROR CRITICO] Fallo al inicializar el subsistema de red Winsock.\n");
        printf("[SISTEMA] Clausurando base de datos antes del cierre de emergencia...\n");
        db_cerrar_sistema(db);
        return EXIT_FAILURE;
    }
    
    // Notificación formal de servicio activo utilizando las macros de configuración
    printf("[OK] Servicio de red montado exitosamente.\n");
    printf("[INFO] Motor de enrutamiento (Broker) habilitado.\n");
    printf("[INFO] Servidor escuchando peticiones en el PUERTO %d...\n", PUERTO_SERVIDOR);
    printf("-------------------------------------------------------------------\n");

    /* ---------------------------------------------------------------------------
     * FASE 3: Bucle de Ejecución Permanente (Modo Demonio)
     * ---------------------------------------------------------------------------
     * La ejecución entra en un ciclo infinito bloqueante dentro de network.c,
     * actuando como broker de mensajes y despachador de consultas SQL.
     */
    red_escuchar_conexiones(db);

    /* ---------------------------------------------------------------------------
     * FASE 4: Clausura y Desmontaje Seguro del Sistema
     * ---------------------------------------------------------------------------
     * Este bloque de código garantiza el apagado limpio del servidor en caso de
     * interrupciones controladas del sistema operativo, evitando la corrupción.
     */
    printf("\n-------------------------------------------------------------------\n");
    printf("[SISTEMA] Iniciando protocolo de apagado seguro...\n");
    
    red_limpiar_sistema();
    printf("[OK] Sockets de red cerrados y subsistema Winsock liberado.\n");
    
    db_cerrar_sistema(db);
    printf("[OK] Conexiones a la base de datos clausuradas y buffers volcados.\n");
    
    printf("[SISTEMA] Servidor Central finalizado correctamente.\n");
    return EXIT_SUCCESS;
}