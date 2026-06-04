#ifndef CONFIG_H
#define CONFIG_H

/* ============================================================================
 * CONFIGURACIÓN GLOBAL DEL SISTEMA
 * PROYECTO: Sistema de Gestión de Estudiantes
 * ============================================================================
 */

// Ruta relativa del archivo de persistencia física (SQLite 3)
// El administrador buscará la carpeta 'datos' un nivel arriba de la ejecución
#define RUTA_DB "../datos/archivo.db"

/* ============================================================================
 * LÍMITES ESTRUCTURALES DE MEMORIA (Buffers Fijos)
 * ============================================================================
 */
#define MAX_NOMBRE 50
#define MAX_CURSO  30
#define MAX_TURNO  15

#endif /* CONFIG_H */