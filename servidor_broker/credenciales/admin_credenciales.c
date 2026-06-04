#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USER 50
#define MAX_PASS 64
#define RUTA_DATOS "../datos/usuarios.dat"

// Estructura fija para persistencia binaria estructurada
#pragma pack(push, 1)
typedef struct {
    char usuario[MAX_USER];
    char password[MAX_PASS];
} Credencial;
#pragma pack(pop)

/* * Función de defensa: Limpia residuos en el flujo de entrada de la consola
 * para evitar saltos de línea automáticos o lecturas corruptas.
 */
void limpiar_flujo_entrada() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* * Registra una nueva credencial escribiéndola directamente en el archivo binario.
 * Utiliza modo de adición segura ("ab").
 */
void registrar_credencial() {
    FILE *archivo = fopen(RUTA_DATOS, "ab");
    
    // 1. Condicional de defensa: Verificar acceso físico a la ruta destino
    if (archivo == NULL) {
        printf("[ERROR] No se pudo acceder a la ruta '%s'.\n", RUTA_DATOS);
        printf("[AYUDA] Asegúrate de que la carpeta 'datos/' exista en la raíz del servidor.\n");
        return;
    }

    Credencial nueva_cuenta;
    memset(&nueva_cuenta, 0, sizeof(Credencial)); // Limpieza defensiva de la estructura en memoria

    printf("\n=========================================\n");
    printf("        ALTA DE NUEVAS CREDENCIALES      \n");
    printf("=========================================\n");

    printf("► Ingrese el nombre de usuario: ");
    if (fgets(nueva_cuenta.usuario, sizeof(nueva_cuenta.usuario), stdin) != NULL) {
        // Eliminación del salto de línea residual de fgets
        nueva_cuenta.usuario[strcspn(nueva_cuenta.usuario, "\n")] = '\0';
    }

    printf("► Ingrese la contraseña de acceso: ");
    if (fgets(nueva_cuenta.password, sizeof(nueva_cuenta.password), stdin) != NULL) {
        nueva_cuenta.password[strcspn(nueva_cuenta.password, "\n")] = '\0';
    }

    // 2. Control de defensa: Validar que los campos no se hayan enviado vacíos
    if (strlen(nueva_cuenta.usuario) == 0 || strlen(nueva_cuenta.password) == 0) {
        printf("[ALERTA] Operación cancelada. El usuario o contraseña no pueden estar vacíos.\n");
        fclose(archivo);
        return;
    }

    // 3. Escritura en bloque binario directo al disco
    size_t escritos = fwrite(&nueva_cuenta, sizeof(Credencial), 1, archivo);
    
    if (escritos == 1) {
        printf("[ÉXITO] Credencial para '%s' guardada correctamente en usuarios.dat.\n", nueva_cuenta.usuario);
    } else {
        printf("[ERROR] Falla crítica al escribir en el almacenamiento.\n");
    }

    fclose(archivo);
}

int main() {
    int opcion = 0;

    // Bucle defensivo de control de flujo principal
    do {
        printf("\n=========================================\n");
        printf("       SISTEMA CENTRAL DE ACCESOS        \n");
        printf("=========================================\n");
        printf("  1. Registrar nuevo usuario\n");
        printf("  2. Salir del sistema\n");
        printf("-----------------------------------------\n");
        printf("► Seleccione una opción: ");

        if (scanf("%d", &opcion) != 1) {
            printf("[ALERTA] Entrada inválida. Seleccione una opción numérica.\n");
            limpiar_flujo_entrada();
            continue;
        }
        limpiar_flujo_entrada();

        switch (opcion) {
            case 1:
                registrar_credencial();
                break;
            case 2:
                printf("[INFO] Cerrando el gestor de credenciales de forma segura.\n");
                break;
            default:
                printf("[ALERTA] Opción fuera de rango. Intente nuevamente.\n");
                break;
        }
    } while (opcion != 2);

    return 0;
}