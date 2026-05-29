#include "include/libjsonindex.h"

int main() {
  FILE* jsonOriginal = fopen("sample.json", "r");
  if (!jsonOriginal) {
    perror("error al abrir sample.json");
    return 1;
  }

  fseek(jsonOriginal, 0, SEEK_END);
  long size = ftell(jsonOriginal);
  fseek(jsonOriginal, 0, SEEK_SET); // obtener tamanno archivo

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
    perror("error al abrir datos.jnx");
    free(src);
    return 1;
  }
  parsear(&p, "", out);
  imprimirBufferSalida(out);
  fclose(out);
  free(src);
  return 0;
}