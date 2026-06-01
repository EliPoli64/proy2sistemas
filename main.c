#include "include/libjsonindex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/** @brief revisa si un string es un numero para colocar quotes o no */
int esNumerico(const char* str) {
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

/** @brief sanitiza char* quitando espacios */
char* cortarEspacio(const char *s) {
  if (!s) return NULL;
  const char *start = s;
  while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
  const char *end = s + strlen(s) - 1;
  while (end >= start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
  size_t len = (end >= start) ? (end - start + 1) : 0;
  char *out = malloc(len + 1);
  if (!out) return NULL;
  if (len) memcpy(out, start, len);
  out[len] = '\0';
  return out;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Uso: %s [index <archivojson>] | [search <archivojson> <caminoconregex>]\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "index") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Uso: %s index <archivojson>\n", argv[0]);
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

    ResultadoBusq results = buscar("datos.jnx", jsonFileName, regexPattern);

    printf("[");
    for (int i = 0; i < results.count; i++) {
      char* trimmed = cortarEspacio(results.values[i]);
      if (!trimmed) trimmed = results.values[i];
      if (trimmed[0] == '{' || trimmed[0] == '[') {
        printf("%s", trimmed);
      } else if (strchr(trimmed, ':') != NULL) {
        printf("{%s}", trimmed);
      } else if (esNumerico(trimmed) || strcmp(trimmed, "true") == 0 ||
                strcmp(trimmed, "false") == 0 || strcmp(trimmed, "null") == 0) {
        printf("%s", trimmed);
      } else {
        printf("\"%s\"", trimmed);
      }
      if (trimmed != results.values[i]) free(trimmed);
      if (i != results.count - 1) printf(", ");
    }
    printf("]\n");
    liberarResultado(&results);
  } else {
    fprintf(stderr, "Comando desconocido: %s\n", argv[1]);
    fprintf(stderr, "Uso: %s [index <archivojson>] | [search <archivojson> <caminoconregex>]\n", argv[0]);
    return 1;
  }

  return 0;
}