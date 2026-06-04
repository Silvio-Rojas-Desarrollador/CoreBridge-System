#include <stdio.h>
#include <string.h>
#include "sqlite3.h"
#include "config.h"
#include "db_manager.h"

/* * CREATE: Inserta un nuevo registro de estudiante en archivo.db.
 * Implementa condicionales de pre-validación de datos y un bucle de contención 
 * anti-colapso para entornos de alta concurrencia.
 * Retorna 1 si la transacción fue exitosa (COMMIT), o 0 si falló o fue rechazada.
 */
int db_crear_estudiante(const Estudiante *estudiante) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int codigo_estado;
    int intentos = 0;
    const int MAX_INTENTOS = 5;
    int resultado_final = 0;

    // ========================================================================
    // CONDICIONALES DE DEFENSA: VALIDACIÓN DE DATOS (Muro de Contención)
    // ========================================================================
    if (estudiante == NULL) {
        printf("Error de protección: Intento de registrar un puntero nulo.\n");
        return 0;
    }

    // Defensa contra cadenas vacías o desbordamientos lógicos
    if (strlen(estudiante->nombre) == 0 || estudiante->edad < 5 || estudiante->edad > 100 || strlen(estudiante->curso) == 0) {
        printf("Error de consistencia: Datos escolares fuera de los límites permitidos.\n");
        return 0; 
    }

    // Defensa estricta para el campo Turno (Garantía de integridad ACID)
    if (strcmp(estudiante->turno, "Mañana") != 0 && 
        strcmp(estudiante->turno, "Tarde") != 0 && 
        strcmp(estudiante->turno, "Noche") != 0) {
        printf("Error de consistencia: El turno asignado no es válido para el sistema.\n");
        return 0;
    }

    // ========================================================================
    // BUCLE DE DEFENSA: CONTROL DE CONCURRENCIA (Anti-colapso)
    // ========================================================================
    // Este bucle impide que el programa caiga si el archivo está bloqueado por el servidor
    while (intentos < MAX_INTENTOS) {
        codigo_estado = sqlite3_open(RUTA_DB, &db);
        
        if (codigo_estado == SQLITE_OK) {
            break; // Conexión establecida con éxito, rompemos el bucle de espera
        }

        if (codigo_estado == SQLITE_BUSY) {
            intentos++;
            printf("Base de datos ocupada. Reintentando acceso (Intento %d de %d)...\n", intentos, MAX_INTENTOS);
            // Reemplazar por una función nativa de retardo (ej. Sleep) en producción si es necesario
            for (volatile int i = 0; i < 10000000; i++); 
        } else {
            // Si es un error físico diferente a ocupado, abortamos inmediatamente
            printf("Error crítico de hardware/infraestructura al abrir la base de datos.\n");
            sqlite3_close(db);
            return 0;
        }
    }

    // Condicional de escape si el bucle de intentos se agotó sin éxito
    if (codigo_estado != SQLITE_OK) {
        printf("Error de aislamiento (ACID): El archivo permanece bloqueado por otro proceso.\n");
        return 0;
    }

    // ========================================================================
    // EJECUCIÓN DE TRANSACCIÓN ATÓMICA
    // ========================================================================
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    const char *sql_insertar = 
        "INSERT INTO estudiantes (nombre, edad, curso, turno) VALUES (?, ?, ?, ?);";

    codigo_estado = sqlite3_prepare_v2(db, sql_insertar, -1, &stmt, NULL);
    if (codigo_estado != SQLITE_OK) {
        printf("Error de compilación SQL: Fallo al preparar la sentencia de inserción.\n");
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
        sqlite3_close(db);
        return 0;
    }

    // Enlazado seguro de buffers fijos
    sqlite3_bind_text(stmt, 1, estudiante->nombre, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, estudiante->edad);
    sqlite3_bind_text(stmt, 3, estudiante->curso, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, estudiante->turno, -1, SQLITE_TRANSIENT);

    codigo_estado = sqlite3_step(stmt);

    // Condicional de confirmación o descarte
    if (codigo_estado == SQLITE_DONE) {
        sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
        resultado_final = 1; // Registro exitoso y duradero en disco
    } else {
        printf("Transacción rechazada por el motor. Ejecutando reversión de datos.\n");
        printf("Detalle técnico: %s\n", sqlite3_errmsg(db));
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    }

    // Liberación absoluta de memoria y descriptores
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return resultado_final;
}