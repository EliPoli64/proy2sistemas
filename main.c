#include "include/libjsonindex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int esNumerico(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    if (*str == '-') str++;
    if (*str == '\0') return 0;
    int hasDigit = 0;
    int hasDecimal = 0;
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            hasDigit = 1;
        } else if (*str == '.') {
            if (hasDecimal) return 0;
            hasDecimal = 1;
        } else if (*str == 'e' || *str == 'E') {
            str++;
            if (*str == '+' || *str == '-') str++;
            while (*str >= '0' && *str <= '9') str++;
            return (*str == '\0');
        } else {
            return 0;
        }
        str++;
    }
    return hasDigit;
}

static int esBooleano(const char* str) {
    return (strcmp(str, "true") == 0 || strcmp(str, "false") == 0);
}

static int esNull(const char* str) {
    return (strcmp(str, "null") == 0);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Uso: %s [index <archivojson>] | [search <archivojson> <caminoconregex>]\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "index") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Uso: %s index <json_file>\n", argv[0]);
      return 1;
    }
    const char* jsonFileName = argv[2];
    FILE* jsonOriginal = fopen(jsonFileName, "r");
    if (!jsonOriginal) {
      perror("error al abrir el archivo JSON original");
      return 1;
    }

    fseek(jsonOriginal, 0, SEEK_END);
    long size = ftell(jsonOriginal);
    fseek(jsonOriginal, 0, SEEK_SET);

    char* src = malloc(size + 1);
    if (!src) {
      perror("error de asignacion de memoria");
      fclose(jsonOriginal);
      return 1;
    }

    size_t readBytes = fread(src, 1, size, jsonOriginal);
    src[readBytes] = '\0';
    fclose(jsonOriginal);

    Parser p = {
      .src = src,
      .pos = 0
    };

    FILE* out = fopen("datos.jnx", "w");
    if (!out) {
      perror("error al abrir el archivo .jnx");
      free(src);
      return 1;
    }
    parsear(&p, "", out);
    imprimirBufferSalida(out);
    fclose(out);
    free(src);
  } else if (strcmp(argv[1], "search") == 0) {
    if (argc < 4) {
      fprintf(stderr, "Uso: %s search <archivojson> <caminoconregex>\n", argv[0]);
      return 1;
    }
    const char* jsonFileName = argv[2];
    const char* regexPattern = argv[3];

    char jnxFileName[256];
    snprintf(jnxFileName, sizeof(jnxFileName), "%.*s.jnx", (int)(strlen(jsonFileName) - 5), jsonFileName);

    SearchResult results = buscar(jnxFileName, jsonFileName, regexPattern);

    printf("[");
    for (int i = 0; i < results.count; i++) {
        if (esNumerico(results.values[i])) {
            printf("%s", results.values[i]);
        } else if (esBooleano(results.values[i])) {
            printf("%s", results.values[i]);
        } else if (esNull(results.values[i])) {
            printf("%s", results.values[i]);
        } else {
            printf("\"%s\"", results.values[i]);
        }
        if (i != results.count - 1) printf(", ");
    }
    printf("]\n");
  } else {
    fprintf(stderr, "Comando desconocido: %s\n", argv[1]);
    fprintf(stderr, "Uso: %s [index <archivojson>] | [search <archivojson> <caminoconregex>]\n", argv[0]);
    return 1;
  }

  return 0;
}