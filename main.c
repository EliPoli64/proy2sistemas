#include "include/libjsonindex.h"
#include <string.h>

void imprimirArregloJson(char** valores, int cantidadValores) {
    printf("[");
    for (int i = 0; i < cantidadValores; ++i) {
        printf("%s", valores[i]);
        if (i < cantidadValores - 1) {
            printf(", ");
        }
    }
    printf("]\n");
}

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        fprintf(stderr, "Comandos:\n");
        fprintf(stderr, "  build <archivoJson>\n");
        fprintf(stderr, "  search <archivoJson> <patronRegex>\n");
        return 1;
    }

    // generar .jnx
    if (strcmp(argv[1], "build") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Uso: %s build <archivoJson>\n", argv[0]);
            return 1;
        }
        const char* pathJson = argv[2];
        char pathJNX[256];
        // se guarda el path del jnx a crear
        snprintf(pathJNX, sizeof(pathJNX), "%.*s.jnx", (int)(strlen(pathJson) - 5), pathJson);

        FILE* archivoJson = fopen(pathJson, "r");
        if (!archivoJson) {
            perror("Error abriendo archivo JSON");
            return 1;
        }

        // se saca el tamano del json
        fseek(archivoJson, 0, SEEK_END);
        long tamanoJson = ftell(archivoJson);
        fseek(archivoJson, 0, SEEK_SET);

        // este es el archivo json completo
        char* src = (char*)malloc(tamanoJson + 1);
        if (!src) {
            perror("Error asignando memoria para JSON");
            fclose(archivoJson);
            return 1;
        }

        size_t bytesJson = fread(src, 1, tamanoJson, archivoJson);
        src[bytesJson] = '\0';
        fclose(archivoJson);

        Parser p = {
            .src = src,
            .pos = 0
        };

        FILE* archivoJNX = fopen(pathJNX, "w");
        if (!archivoJNX) {
            perror("Error abriendo el archivo JNX");
            free(src);
            return 1;
        }
        fprintf(archivoJNX, "Ruta (path)\tInicio (byte)\tFin (byte)\n");
        fprintf(archivoJNX, "--------------------------------------------------\n"); 

        parseObjeto(&p, "", archivoJNX);
        fclose(archivoJNX);
        free(src);
        printf("Indexado %s a %s\n", pathJson, pathJNX);

    } else if (strcmp(argv[1], "search") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Uso: %s search <archivoJson> <patronRegex>\n", argv[0]);
            return 1;
        }
        const char* pathJson = argv[2];
        const char* patronRegex = argv[3];

        if (patronRegex[0] == '$') {
            patronRegex++;
        }

        char pathJNX[256];
        snprintf(pathJNX, sizeof(pathJNX), "%.*s.jnx", (int)(strlen(pathJson) - 5), pathJson);

        int cantidadValores = 0;
        char** valores = recuperarValoresJson(pathJNX, pathJson, patronRegex, &cantidadValores);

        if (valores == NULL) {
            fprintf(stderr, "No se encontraron valores.\n");
            return 1;
        }

        imprimirArregloJson(valores, cantidadValores);

        for (int i = 0; i < cantidadValores; ++i) {
            free(valores[i]);
        }
        free(valores);

    } else {
        fprintf(stderr, "Comando desconocido: %s\n", argv[1]);
        return 1;
    }

    return 0;
}