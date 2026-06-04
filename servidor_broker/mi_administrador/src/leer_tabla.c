#include <stdio.h>
#include <string.h>
#include "sqlite3.h"
#include "config.h"
#include "db_manager.h"

/* * READ: Lee y despliega en la consola local todos los registros válidos.
 * Implementa un bucle de contención por ocupación física del archivo y 
 * condicionales de control para evitar desbordamientos o lecturas sucias.
 */
void db_leer_estudiantes(void) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int codigo_estado;
    int intentos = 0;
    const int MAX_INTENTOS = 5;
    int contador_registros = 0;

    // ========================================================================
    // BUCLE DE DEFENSA: CONTROL DE CONCURRENCIA (Anti-colapso)
    // ========================================================================
    // Evita fallos si el administrador intenta leer mientras el servidor altera el disco
    while (intentos < MAX_INTENTOS) {
        codigo_estado = sqlite3_open(RUTA_DB, &db);
        
        if (codigo_estado == SQLITE_OK) {
            break; // Conexión establecida, salimos del bucle de reintentos
        }

        if (codigo_estado == SQLITE_BUSY) {
            intentos++;
            printf("Base de datos ocupada por otro proceso. Reintentando lectura (%d/%d)...\n", intentos, MAX_INTENTOS);
            // Retardo pasivo para dar tiempo de liberación al hardware
            for (volatile int i = 0; i < 10000000; i++); 
        } else {
            printf("Error crítico de hardware/infraestructura al abrir la base de datos para lectura.\n");
            sqlite3_close(db);
            return;
        }
    }

    // Condicional de escape si el archivo físico está inaccesible temporalmente
    if (codigo_estado != SQLITE_OK) {
        printf("Error de aislamiento (ACID): No se pudo leer el archivo. Origen ocupado.\n");
        return;
    }

    // Preparación de la consulta de selección limpia
    const char *sql_seleccionar = "SELECT id, nombre, edad, curso, turno FROM estudiantes;";

    codigo_estado = sqlite3_prepare_v2(db, sql_seleccionar, -1, &stmt, NULL);
    if (codigo_estado != SQLITE_OK) {
        printf("Error de compilación SQL: Fallo al preparar la sentencia de extracción.\n");
        printf("Detalle técnico: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    // ========================================================================
    // BUCLE DE EXTRACCIÓN Y DESPLIEGUE DE REGISTROS
    // ========================================================================
    // Este bucle avanza fila por fila (registro por registro) de forma secuencial
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        
        // CONDICIONAL DE CONTROL: Formateo estético del encabezado solo en el primer registro
        if (contador_registros == 0) {
            printf("\n=================================================================================\n");
            printf("%-6s | %-30s | %-5s | %-15s | %-12s\n", "ID", "NOMBRE DEL ESTUDIANTE", "EDAD", "CURSO", "TURNO");
            printf("---------------------------------------------------------------------------------\n");
        }

        // Extracción directa desde los buffers de la DLL mapeando los tipos de datos nativos
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char *nombre_raw = sqlite3_column_text(stmt, 1);
        int edad = sqlite3_column_int(stmt, 2);
        const unsigned char *curso_raw = sqlite3_column_text(stmt, 3);
        const unsigned char *turno_raw = sqlite3_column_text(stmt, 4);

        // CONDICIONAL DE SEGURIDAD: Protección contra campos NULL accidentales en el archivo db
        const char *nombre = (nombre_raw != NULL) ? (const char *)nombre_raw : "[Vacío]";
        const char *curso = (curso_raw != NULL) ? (const char *)curso_raw : "[Vacío]";
        const char *turno = (turno_raw != NULL) ? (const char *)turno_raw : "[Vacío]";

        // Imprimir fila utilizando alineación por anchos fijos de cadenas literales tradicionales
        printf("%-6d | %-30s | %-5d | %-15s | %-12s\n", id, nombre, edad, curso, turno);
        
        contador_registros++;
    }

    // ========================================================================
    // CONDICIONAL DE CONTROL DE ENTORNO VACÍO
    // ========================================================================
    if (contador_registros == 0) {
        printf("\n[Sistema] La consulta se ejecutó con éxito pero la tabla está vacía.\n");
    } else {
        printf("=================================================================================\n");
        printf("Total de registros recuperados: %d\n\n", contador_registros);
    }

    // FASE 5: Liberación absoluta de memoria y descriptores bajo demanda (Lazy)
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}