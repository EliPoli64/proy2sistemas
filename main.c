#include "include/libjsonindex.h"

int main() {
  Parser p = {
    .src = "{\n"
      "  \"usuarios\": [\n"
      "    {\n"
      "      \"id\": 1,\n"
      "      \"nombre\": \"Ana\",\n"
      "      \"activo\": true\n"
      "    },\n"
      "    {\n"
      "      \"id\": 2,\n"
      "      \"nombre\": \"Luis\",\n"
      "      \"activo\": false\n"
      "    }\n"
      "  ],\n"
      "  \"config\": {\n"
      "    \"version\": \"1.0\",\n"
      "    \"opciones\": {\n"
      "      \"modo\": \"auto\"\n"
      "    }\n"
      "  }\n"
      "}",
    .pos = 0
  };

  FILE* out = fopen("datos.jnx", "w");
  if (!out) {
    perror("error al abrir datos.jnx");
    return 1;
  }
  parseObject(&p, "", out);
  fclose(out);
  return 0;
}