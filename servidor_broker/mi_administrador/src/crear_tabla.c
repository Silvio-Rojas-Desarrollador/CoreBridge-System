#include <stdio.h>
#include <string.h>
#include "sqlite3.h"
#include "config.h"
#include "db_manager.h"

/* * Inicializa de forma segura la estructura física de la base de datos.
 * Abre el archivo en la ruta configurada, aplica los parámetros de integridad
 * y crea la tabla estudiantes si no existe en el almacenamiento.
 */
void db_inicializar_esquema(void) {
    sqlite3 *db = NULL;
    char *mensaje_error = NULL;
    int codigo_estado;

    // FASE 1: Apertura diferida (Lazy Loading) del archivo físico en la ruta asignada
    // RUTA_DB debe apuntar a "../datos/archivo.db" definida en config.h
    codigo_estado = sqlite3_open(RUTA_DB, &db);

    if (codigo_estado != SQLITE_OK) {
        printf("Error de infraestructura: No se pudo abrir o crear el archivo de base de datos.\n");
        printf("Detalle: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    // FASE 2: Declaración del Script de Definición de Datos (DDL) con restricciones ACID
    // Se definen los campos obligatorios: nombre, edad, curso y turno con sus respectivos CHECK
    const char *sql_crear_tabla = 
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS estudiantes ("
        "   id     INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   nombre TEXT NOT NULL CHECK(length(trim(nombre)) > 0),"
        "   edad   INTEGER NOT NULL CHECK(edad >= 5 AND edad <= 100),"
        "   curso  TEXT NOT NULL CHECK(length(trim(curso)) > 0),"
        "   turno  TEXT NOT NULL CHECK(turno IN ('Mañana', 'Tarde', 'Noche'))"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_estudiantes_id ON estudiantes(id);";

    // FASE 3: Ejecución directa y atómica del bloque SQL
    codigo_estado = sqlite3_exec(db, sql_crear_tabla, NULL, NULL, &mensaje_error);

    // FASE 4: Verificación de estados y manejo de consistencia
    if (codigo_estado != SQLITE_OK) {
        printf("Error de Consistencia (ACID): Fallo al inicializar el esquema de la tabla.\n");
        printf("Detalle del motor: %s\n", mensaje_error);
        sqlite3_free(mensaje_error);
    }

    // FASE 5: Liberación absoluta del recurso físico (Cierre inmediato de la conexión)
    sqlite3_close(db);
}