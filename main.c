#include "include/libjsonindex.h"

int main() {
  FILE* jsonFile = fopen("sample.json", "r");
  if (!jsonFile) {
    perror("error al abrir sample.json");
    return 1;
  }

  // Get file size
  fseek(jsonFile, 0, SEEK_END);
  long size = ftell(jsonFile);
  fseek(jsonFile, 0, SEEK_SET);

  char* src = malloc(size + 1);
  if (!src) {
    perror("error de asignacion de memoria");
    fclose(jsonFile);
    return 1;
  }

  size_t readBytes = fread(src, 1, size, jsonFile);
  src[readBytes] = '\0';
  fclose(jsonFile);

  Parser p = {
    .src = src,
    .pos = 0
  };

  FILE* out = fopen("datos.jnx", "w");
  if (!out) {
    perror("error al abrir datos.jnx");
    free(src);
    return 1;
  }
  parseObject(&p, "", out);
  fclose(out);
  free(src);
  return 0;
}