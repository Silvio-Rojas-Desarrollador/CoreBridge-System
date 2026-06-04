#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "config_server.h"
#include "database.h"

/* ===============================================================================
 * 1. INICIALIZACIÓN DEL SISTEMA RELACIONAL
 * ===============================================================================
 */
int db_inicializar_sistema(sqlite3 **db) {
    char *err_msg = NULL;
    
    // 1. Intentar abrir la conexión física con el archivo en disco
    int rc = sqlite3_open(RUTA_BD_SQLITE, db);
    if (rc != SQLITE_OK) {
        // Cierre defensivo de recursos asignados parcialmente en fallo
        sqlite3_close(*db);
        return ESTADO_ERROR;
    }

    // 2. Sentencia DDL para garantizar la existencia del esquema de datos
    // Agregamos edad, curso y turno para mantener la compatibilidad con el administrador
    const char *sql_crear_tabla = 
        "CREATE TABLE IF NOT EXISTS estudiantes ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "nombre TEXT NOT NULL, "
        "edad INTEGER, "
        "curso TEXT, "
        "turno TEXT);";

    // 3. Ejecución directa segura para comandos estructurales de arranque
    rc = sqlite3_exec(*db, sql_crear_tabla, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        if (err_msg != NULL) {
            sqlite3_free(err_msg);
        }
        sqlite3_close(*db);
        return ESTADO_ERROR;
    }

    return ESTADO_EXITO;
}

/* ===============================================================================
 * 2. OPERACIONES DE MUTACIÓN PARAMETRIZADAS (INSERCIÓN, EDICIÓN, BORRADO)
 * ===============================================================================
 */

int db_insertar_estudiante(sqlite3 *db, const char *nombre, int edad, const char *curso, const char *turno) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO estudiantes (nombre, edad, curso, turno) VALUES (?, ?, ?, ?);";

    // Paso 1: Compilación previa de la estructura SQL
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return ESTADO_ERROR;
    }

    // Paso 2: Binding rígido de los tipos de datos entrantes
    sqlite3_bind_text(stmt, 1, nombre, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, edad);
    sqlite3_bind_text(stmt, 3, curso, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, turno, -1, SQLITE_STATIC);

    // Paso 3: Ejecución física en el motor
    rc = sqlite3_step(stmt);
    
    // Paso 4: Liberación obligatoria del statement para prevenir fugas de memoria
    sqlite3_finalize(stmt);

    // Paso 5: Evaluación del resultado de la operación
    if (rc != SQLITE_DONE) {
        return ESTADO_ERROR;
    }

    return ESTADO_EXITO;
}

int db_editar_estudiante(sqlite3 *db, int id, const char *nuevo_nombre, int edad, const char *curso, const char *turno) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE estudiantes SET nombre = ?, edad = ?, curso = ?, turno = ? WHERE id = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return ESTADO_ERROR;
    }

    sqlite3_bind_text(stmt, 1, nuevo_nombre, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, edad);
    sqlite3_bind_text(stmt, 3, curso, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, turno, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return ESTADO_ERROR;
    }

    // Auditoría de impacto: Verificar si realmente se modificó una fila existente
    int cambios = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (cambios == 0) {
        return ESTADO_NO_ENCONTRADO;
    }

    return ESTADO_EXITO;
}

int db_borrar_estudiante(sqlite3 *db, int id) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM estudiantes WHERE id = ?;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return ESTADO_ERROR;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return ESTADO_ERROR;
    }

    // Auditoría de impacto: Verificar si el registro a eliminar realmente existía
    int cambios = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (cambios == 0) {
        return ESTADO_NO_ENCONTRADO;
    }

    return ESTADO_EXITO;
}

/* ===============================================================================
 * 3. EXTRACCIÓN SECUENCIAL CON INVERSIÓN DE CONTROL (CALLBACK)
 * ===============================================================================
 */
int db_listar_estudiantes(sqlite3 *db, DbEstudianteCallback callback, void *contexto_red) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, nombre, edad, curso, turno FROM estudiantes;";

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return ESTADO_ERROR;
    }

    // Bucle de extracción secuencial (Fetch Loop)
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // Mapeo directo y seguro de tipos nativos por índice de columna
        int id = sqlite3_column_int(stmt, 0);
        const char *nombre = (const char *)sqlite3_column_text(stmt, 1);
        int edad = sqlite3_column_int(stmt, 2);
        const char *curso = (const char *)sqlite3_column_text(stmt, 3);
        const char *turno = (const char *)sqlite3_column_text(stmt, 4);

        // Disparar la función de callback inyectando los datos hacia la capa de red
        callback(id, nombre, edad, curso, turno, contexto_red);
    }

    sqlite3_finalize(stmt);
    return ESTADO_EXITO;
}

/* ===============================================================================
 * 4. CLAUSURA LIMPIA DEL MOTOR
 * ===============================================================================
 */
void db_cerrar_sistema(sqlite3 *db) {
    if (db != NULL) {
        sqlite3_close(db);
    }
}