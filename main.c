#include "include/libjsonindex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Uso: %s [index <json_file>] | [search <json_file> <regex_path>]\n", argv[0]);
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

    char jnxFileName[256];
    snprintf(jnxFileName, sizeof(jnxFileName), "%.*s.jnx", (int)(strlen(jsonFileName) - 5), jsonFileName); // Remove .json extension
    FILE* out = fopen(jnxFileName, "w");
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
      fprintf(stderr, "Uso: %s search <json_file> <regex_path>\n", argv[0]);
      return 1;
    }
    const char* jsonFileName = argv[2];
    const char* regexPattern = argv[3];

    char jnxFileName[256];
    snprintf(jnxFileName, sizeof(jnxFileName), "%.*s.jnx", (int)(strlen(jsonFileName) - 5), jsonFileName); // Remove .json extension

    SearchResult results = searchJson(jnxFileName, jsonFileName, regexPattern);

    printf("[");
    for (int i = 0; i < results.count; i++) {
        printf("%s%s", results.values[i], (i == results.count - 1 ? "" : ", "));
    }
    printf("]\n");

    freeSearchResult(&results);
  } else {
    fprintf(stderr, "Comando desconocido: %s\n", argv[1]);
    fprintf(stderr, "Uso: %s [index <json_file>] | [search <json_file> <regex_path>]\n", argv[0]);
    return 1;
  }

  return 0;
}