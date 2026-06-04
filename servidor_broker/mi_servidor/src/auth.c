#include <stdio.h>
#include <string.h>
#include "config_server.h"
#include "auth.h"

/* ===============================================================================
 * 1. VERIFICACIÓN DE CREDENCIALES BINARIAS
 * ===============================================================================
 */
int auth_verificar_credenciales(const char *usuario_entrante, const char *password_entrante) {
    FILE *archivo = fopen(RUTA_BIN_ACCESOS, "rb");
    
    // 1. Control de defensa: Verificar existencia o acceso físico al archivo binario
    if (archivo == NULL) {
        return ESTADO_ERROR;
    }

    Credencial registro;
    int estado_autenticacion = ESTADO_DENEGADO;

    // 2. Bucle de búsqueda secuencial leyendo estructuras de tamaño estático fijo
    while (fread(&registro, sizeof(Credencial), 1, archivo) == 1) {
        
        // 3. Contraste estricto bit a bit de las cadenas de texto
        if (strcmp(registro.usuario, usuario_entrante) == 0 && 
            strcmp(registro.password, password_entrante) == 0) {
            
            estado_autenticacion = ESTADO_EXITO;
            break; // Identidad confirmada. Romper el bucle de inmediato de forma defensiva
        }
    }

    // 4. Clausura del descriptor de archivo para liberar el bloqueo del sistema operativo
    fclose(archivo);
    
    return estado_autenticacion;
}