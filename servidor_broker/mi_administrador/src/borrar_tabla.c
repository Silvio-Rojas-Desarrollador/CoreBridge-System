#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "config.h"
#include "db_manager.h"

/*
 * Elimina un registro de estudiante de forma permanente mediante su ID.
 */
int db_borrar_estudiante(int id) {
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int resultado = 0;

    // 1. Validación básica de entrada
    if (id <= 0) return 0;

    // 2. Apertura de la persistencia
    if (sqlite3_open(RUTA_DB, &db) != SQLITE_OK) {
        printf("[ERROR] No se pudo abrir la BD para eliminación.\n");
        sqlite3_close(db);
        return 0;
    }

    const char *sql = "DELETE FROM estudiantes WHERE id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("[ERROR] Error al preparar DELETE: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_DONE) {
        if (sqlite3_changes(db) > 0) {
            resultado = 1;
        }
    } else {
        printf("[ERROR] Fallo en ejecución de borrado: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return resultado;
}