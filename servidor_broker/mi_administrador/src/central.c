#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "db_manager.h"

// Sub-módulos auxiliares de captura segura (Evitan desbordamientos y colapsos)
static int leer_entero_seguro(const char *mensaje, int min, int max);
static void leer_cadena_segura(const char *mensaje, char *destino, int tamano_maximo);
static void limpiar_pantalla(void);
static void pausar(void);

int main(void) {
    int opcion = 0;
    Estudiante nuevo_alumno;
    int id_busqueda;

    // Inicialización diferida de la infraestructura física del archivo .db
    db_inicializar_esquema();

    // ========================================================================
    // BUCLE DE CONTROL PRINCIPAL: INTERFAZ DE USUARIO (Anti-colapso)
    // ========================================================================
    while (opcion != 5) {
        limpiar_pantalla();
        printf("\n==================================================\n");
        printf("   SISTEMA DE GESTIÓN ESCOLAR (MODO ADMINISTRADOR) \n");
        printf("==================================================\n");
        printf("1. Registrar nuevo estudiante (Create)\n");
        printf("2. Visualizar lista de estudiantes (Read)\n");
        printf("3. Modificar datos de estudiante (Update)\n");
        printf("4. Dar de baja estudiante (Delete)\n");
        printf("5. Salir del sistema\n");
        printf("--------------------------------------------------\n");

        // El bucle de contención garantiza que 'opcion' solo sea un número entre 1 y 5
        opcion = leer_entero_seguro("Seleccione una opción: ", 1, 5);

        switch (opcion) {
            case 1:
                limpiar_pantalla();
                printf("\n--- REGISTRO DE NUEVO ESTUDIANTE ---\n");
                
                // Captura segura protegiendo las fronteras de memoria estática
                leer_cadena_segura("Nombre completo: ", nuevo_alumno.nombre, MAX_NOMBRE);
                nuevo_alumno.edad = leer_entero_seguro("Edad (5 a 100 años): ", 5, 100);
                leer_cadena_segura("Curso / Grado: ", nuevo_alumno.curso, MAX_CURSO);
                
                // Bucle de validación específico para el campo 'Turno'
                int turno_valido = 0;
                while (turno_valido == 0) {
                    leer_cadena_segura("Turno (Mañana/Tarde/Noche): ", nuevo_alumno.turno, MAX_TURNO);
                    
                    if (strcmp(nuevo_alumno.turno, "Mañana") == 0 || 
                        strcmp(nuevo_alumno.turno, "Tarde") == 0 || 
                        strcmp(nuevo_alumno.turno, "Noche") == 0) {
                        turno_valido = 1;
                    } else {
                        printf("[Error] Entrada rechazada. Escriba exactamente: Mañana, Tarde o Noche.\n");
                    }
                }

                // Invocación a la capa de persistencia atómica
                if (db_crear_estudiante(&nuevo_alumno) == 1) {
                    printf("[Éxito] Registro consolidado correctamente en el disco.\n");
                } else {
                    printf("[Fallo] No se pudo guardar el registro.\n");
                }
                pausar();
                break;

            case 2:
                limpiar_pantalla();
                // Despliegue de datos directo
                db_leer_estudiantes();
                pausar();
                break;

            case 3:
                limpiar_pantalla();
                printf("\n--- MODIFICAR DATOS DE ESTUDIANTE ---\n");
                
                // Visualización preventiva para identificar el ID objetivo
                db_leer_estudiantes();

                id_busqueda = leer_entero_seguro("Ingrese el ID del estudiante a editar: ", 1, 999999);
                
                // Recuperamos datos actuales para no perder información en los campos no editados
                if (db_obtener_estudiante_por_id(id_busqueda, &nuevo_alumno)) {
                    printf("\nValores actuales: [%s, %d años, %s, %s]\n", 
                            nuevo_alumno.nombre, nuevo_alumno.edad, nuevo_alumno.curso, nuevo_alumno.turno);
                    printf("¿Qué campo desea modificar?\n");
                    printf("1. Nombre\n2. Edad\n3. Curso\n4. Turno\n5. Cancelar\n");
                    int sub_opcion = leer_entero_seguro("Seleccione una opción: ", 1, 5);

                    if (sub_opcion == 5) break;

                    if (sub_opcion == 1) leer_cadena_segura("Nuevo Nombre: ", nuevo_alumno.nombre, MAX_NOMBRE);
                    if (sub_opcion == 2) nuevo_alumno.edad = leer_entero_seguro("Nueva Edad (5-100): ", 5, 100);
                    if (sub_opcion == 3) leer_cadena_segura("Nuevo Curso: ", nuevo_alumno.curso, MAX_CURSO);
                    if (sub_opcion == 4) {
                        int turno_ok = 0;
                        while (!turno_ok) {
                            leer_cadena_segura("Nuevo Turno (Mañana/Tarde/Noche): ", nuevo_alumno.turno, MAX_TURNO);
                            if (strcmp(nuevo_alumno.turno, "Mañana") == 0 || 
                                strcmp(nuevo_alumno.turno, "Tarde") == 0 || 
                                strcmp(nuevo_alumno.turno, "Noche") == 0) turno_ok = 1;
                            else printf("[Error] Turno inválido.\n");
                        }
                    }

                    if (db_editar_estudiante(id_busqueda, &nuevo_alumno) == 1) {
                        printf("[Éxito] Campo actualizado correctamente.\n");
                    } else {
                        printf("[Fallo] No se pudo actualizar la base de datos.\n");
                    }
                } else {
                    printf("[Error] El ID %d no existe en el sistema.\n", id_busqueda);
                }
                pausar();
                break;

            case 4:
                limpiar_pantalla();
                printf("\n--- ELIMINAR ESTUDIANTE DEL REGISTRO ---\n");
                
                // 1. Mostrar lista para identificar ID
                db_leer_estudiantes();

                id_busqueda = leer_entero_seguro("Ingrese el ID del estudiante a dar de baja: ", 1, 999999);
                
                // 2. Recuperar datos para confirmación visual
                if (db_obtener_estudiante_por_id(id_busqueda, &nuevo_alumno)) {
                    char confirm[10];
                    printf("¿Está seguro que desea eliminar permanentemente a [%s]? (S/N): ", nuevo_alumno.nombre);
                    leer_cadena_segura("", confirm, 10);

                    if (confirm[0] == 'S' || confirm[0] == 's') {
                        if (db_borrar_estudiante(id_busqueda) == 1) {
                            printf("[Éxito] Registro eliminado físicamente del archivo.\n");
                        } else {
                            printf("[Fallo] No se pudo ejecutar la baja.\n");
                        }
                    } else {
                        printf("[Aviso] Operación cancelada por el usuario.\n");
                    }
                } else {
                    printf("[Error] El ID %d no existe en el sistema.\n", id_busqueda);
                }

                // 3. Mensaje final y pausa
                pausar();
                break;

            case 5:
                printf("\nCerrando el sistema de administración de forma segura. Adiós.\n");
                break;
        }
    }

    return 0;
}

// ========================================================================
// IMPLEMENTACIÓN DE SUB-MÓDULOS DE SEGURIDAD (Mecanismos de Defensa)
// ========================================================================

static void limpiar_pantalla(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void pausar(void) {
    printf("\nPresione ENTER para continuar...");
    getchar();
}

/* Lee la consola como cadena de texto y la transforma a entero. 
   Anula cualquier posibilidad de desbordamiento por ingreso de letras. */
static int leer_entero_seguro(const char *mensaje, int min, int max) {
    char buffer[32];
    long valor_convertido;
    char *endptr;
    int valido = 0;

    while (valido == 0) {
        printf("%s", mensaje);
        
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            // Limpieza del salto de línea residual (\n)
            buffer[strcspn(buffer, "\n")] = '\0';

            // Conversión matemática estricta de base 10
            valor_convertido = strtol(buffer, &endptr, 10);

            // Condicional de defensa: Si el puntero final no avanzó o hay letras residuales
            if (endptr == buffer || *endptr != '\0') {
                printf("[Error] Entrada no numérica detectada. Intente de nuevo.\n");
                continue;
            }

            // Condicional de rango operativo
            if (valor_convertido >= min && valor_convertido <= max) {
                valido = 1; // Rompe el bucle de captura de forma segura
            } else {
                printf("[Error] El valor numérico debe estar entre %d y %d.\n", min, max);
            }
        }
    }
    return (int)valor_convertido;
}

/* Captura texto delimitando el espacio físico exacto asignado a los arrays de memoria,
   previniendo de raíz ataques o colapsos por Buffer Overflow. */
static void leer_cadena_segura(const char *mensaje, char *destino, int tamano_maximo) {
    int valido = 0;

    while (valido == 0) {
        printf("%s", mensaje);
        
        if (fgets(destino, tamano_maximo, stdin) != NULL) {
            // Eliminar el salto de línea que fgets captura por defecto
            destino[strcspn(destino, "\n")] = '\0';

            // Condicional de defensa: Evitar registros fantasmas o vacíos
            if (strlen(destino) > 0) {
                valido = 1;
            } else {
                printf("[Error] El campo de texto no puede ser procesado si está vacío.\n");
            }
        }
    }
}