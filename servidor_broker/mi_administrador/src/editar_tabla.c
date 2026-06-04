#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sqlite3.h"
#include "config.h"
#include "db_manager.h"

/*
 * Modifica los datos de un estudiante existente por su ID de manera segura.
 * Implementa el patrón de conexión atómica y Parameter Binding.
 */
int db_editar_estudiante(int id, const Estudiante *estudiante_editado) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int resultado = 0;

    // 1. Validación de pre-condiciones
    if (estudiante_editado == NULL || id <= 0) {
        return 0;
    }

    // 2. Apertura de conexión
    if (sqlite3_open(RUTA_DB, &db) != SQLITE_OK) {
        printf("[ERROR] No se pudo acceder a la base de datos para editar.\n");
        sqlite3_close(db);
        return 0;
    }

    // 3. Sentencia SQL Parametrizada (Alineada con el esquema de crear_tabla.c)
    const char *sql = "UPDATE estudiantes SET nombre = ?, edad = ?, curso = ?, turno = ? WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[ERROR] Fallo de compilación SQL: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    // 4. Inyección segura de variables (Parameter Binding)
    sqlite3_bind_text(stmt, 1, estudiante_editado->nombre, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, estudiante_editado->edad);
    sqlite3_bind_text(stmt, 3, estudiante_editado->curso, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, estudiante_editado->turno, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, id);

    // 5. Ejecución y validación del impacto en la BD
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        // Verificamos si realmente se encontró y editó una fila
        if (sqlite3_changes(db) > 0) {
            resultado = 1;
        }
    } else {
        printf("[ERROR] La base de datos rechazó la modificación: %s\n", sqlite3_errmsg(db));
    }

    // 6. Liberación obligatoria de recursos del cursor preparado
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    
    return resultado;
}

int db_obtener_estudiante_por_id(int id, Estudiante *estudiante) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int encontrado = 0;

    if (sqlite3_open(RUTA_DB, &db) != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }

    const char *sql = "SELECT nombre, edad, curso, turno FROM estudiantes WHERE id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            strncpy(estudiante->nombre, (const char*)sqlite3_column_text(stmt, 0), MAX_NOMBRE);
            estudiante->edad = sqlite3_column_int(stmt, 1);
            strncpy(estudiante->curso, (const char*)sqlite3_column_text(stmt, 2), MAX_CURSO);
            strncpy(estudiante->turno, (const char*)sqlite3_column_text(stmt, 3), MAX_TURNO);
            estudiante->id = id;
            encontrado = 1;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return encontrado;
}