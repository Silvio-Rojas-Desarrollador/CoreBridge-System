#ifndef DATABASE_H
#define DATABASE_H

#include "sqlite3.h"
#include "config_server.h"

/* ===============================================================================
 * 1. DEFINICIÓN DE CALLBACKS PARA DESACOPLAMIENTO
 * ===============================================================================
 * Puntero a función que permite a la base de datos "notificar" o "entregar" un 
 * registro de estudiante a cualquier módulo (como la capa de red) sin que este 
 * archivo sepa cómo se van a enviar esos datos por el cable.
 */
typedef void (*DbEstudianteCallback)(int id, const char *nombre, int edad, const char *curso, const char *turno, void *contexto_red);

/* ===============================================================================
 * 2. PROTOTIPOS DE LAS FUNCIONES DEL MOTOR DE PERSISTENCIA
 * ===============================================================================
 * Contrato operativo para la gestión relacional con SQLite.
 */

/* * Abre la conexión física con el archivo de base de datos y asegura la 
 * existencia de la tabla 'estudiantes' con su esquema industrial básico.
 * * Retorna: ESTADO_EXITO o ESTADO_ERROR.
 */
int db_inicializar_sistema(sqlite3 **db);

/* * Inserta un nuevo registro de estudiante de forma parametrizada.
 * * Retorna: ESTADO_EXITO o ESTADO_ERROR.
 */
int db_insertar_estudiante(sqlite3 *db, const char *nombre, int edad, const char *curso, const char *turno);

/* * Modifica un registro existente buscando por su ID único.
 * * Retorna: 
 * - ESTADO_EXITO: Si el registro se modificó correctamente.
 * - ESTADO_NO_ENCONTRADO: Si la query se ejecutó pero el ID no existía.
 * - ESTADO_ERROR: Si ocurrió una falla en el motor SQLite.
 */
int db_editar_estudiante(sqlite3 *db, int id, const char *nuevo_nombre, int edad, const char *curso, const char *turno);

/* * Elimina de forma permanente un registro de la tabla mediante su ID.
 * * Retorna: ESTADO_EXITO, ESTADO_NO_ENCONTRADO o ESTADO_ERROR.
 */
int db_borrar_estudiante(sqlite3 *db, int id);

/* * Ejecuta una consulta secuencial de toda la tabla 'estudiantes'. Por cada fila 
 * encontrada, invoca al 'DbEstudianteCallback' provisto, inyectándole los datos.
 * * Parámetros:
 * - db: Instancia activa de la base de datos.
 * - callback: Función externa que procesará la fila (ej. enviar el paquete por red).
 * - contexto_red: Puntero genérico (void*) que transportará el socket del cliente.
 * * Retorna: ESTADO_EXITO o ESTADO_ERROR.
 */
int db_listar_estudiantes(sqlite3 *db, DbEstudianteCallback callback, void *contexto_red);

/* * Libera de forma segura los recursos del manejador y clausura la conexión a la BD.
 */
void db_cerrar_sistema(sqlite3 *db);

#endif // DATABASE_H