#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include "config.h"

/* ============================================================================
 * ESTRUCTURA DE DATOS UNIFICADA
 * ============================================================================
 * Define el registro de un estudiante en memoria utilizando los límites fijos
 * establecidos en config.h para prevenir desbordamientos.
 */
typedef struct {
    int id;
    char nombre[MAX_NOMBRE];
    int edad;
    char curso[MAX_CURSO];
    char turno[MAX_TURNO];
} Estudiante;

/* ============================================================================
 * CONTRATO DE FUNCIONES PÚBLICAS (API del Módulo de Datos)
 * ============================================================================
 */

/**
 * @brief Inicializa el entorno físico y el esquema de la base de datos.
 * Crea el archivo de persistencia y la tabla si no existen en el almacenamiento.
 */
void db_inicializar_esquema(void);

/**
 * @brief CREATE: Inserta un nuevo registro de estudiante en la base de datos.
 * @param estudiante Puntero constante al registro estructurado en memoria.
 * @return int Retorna 1 si la transacción fue consolidada con éxito, o 0 si falló.
 */
int db_crear_estudiante(const Estudiante *estudiante);

/**
 * @brief READ: Extrae y despliega en la consola todos los registros de la tabla.
 */
void db_leer_estudiantes(void);

/**
 * @brief UPDATE: Modifica los campos de un estudiante existente buscando por su ID.
 * @param id Identificador único del registro a modificar.
 * @param estudiante_editado Puntero con los nuevos valores a persistir.
 * @return int Retorna 1 si la actualización fue exitosa, o 0 si el ID no existe o falló.
 */
int db_editar_estudiante(int id, const Estudiante *estudiante_editado);

/**
 * @brief DELETE: Elimina físicamente un registro de la tabla según su ID.
 * @param id Identificador único del estudiante a remover.
 * @return int Retorna 1 si el borrado fue ejecutado y consolidado, o 0 si falló.
 */
int db_borrar_estudiante(int id);

/**
 * @brief Recupera un registro único por ID para permitir edición parcial.
 * @param id ID del estudiante.
 * @param estudiante Puntero donde se cargarán los datos actuales.
 * @return int 1 si se encontró, 0 si no existe.
 */
int db_obtener_estudiante_por_id(int id, Estudiante *estudiante);

#endif /* DB_MANAGER_H */