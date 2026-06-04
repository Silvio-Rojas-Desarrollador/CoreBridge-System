#ifndef AUTH_H
#define AUTH_H

#include "config_server.h"

/* ===============================================================================
 * 1. ESTRUCTURA DE PERSISTENCIA DE ACCESOS
 * ===============================================================================
 * Espejo estructural exacto del bloque binario guardado en disco. Su tamaño 
 * estático está determinado estrictamente por las macros de config_server.h.
 */
#pragma pack(push, 1)
typedef struct {
    char usuario[MAX_USUARIO];
    char password[MAX_PASSWORD];
} Credencial;
#pragma pack(pop)

/* ===============================================================================
 * 2. PROTOTIPOS DE LAS FUNCIONES DE AUTENTICACIÓN
 * ===============================================================================
 * Contrato operativo que gobernará la lógica de control de accesos.
 */

/* * Valida si las credenciales provistas por un cliente remoto son legítimas.
 * Ejecuta una búsqueda secuencial defensiva dentro de "usuarios.dat".
 * * Parámetros:
 * - usuario_entrante: Cadena de texto con el nombre enviado por la red.
 * - password_entrante: Cadena de texto con la contraseña enviada por la red.
 * * Retorna un código numérico de estado según las directrices globales:
 * - ESTADO_EXITO: Credenciales correctas, sesión autorizada.
 * - ESTADO_DENEGADO: El usuario no existe o la contraseña es incorrecta.
 * - ESTADO_ERROR: El archivo físico de credenciales no es accesible.
 */
int auth_verificar_credenciales(const char *usuario_entrante, const char *password_entrante);

#endif // AUTH_H