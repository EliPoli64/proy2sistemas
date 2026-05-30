#include "../include/libjsonindex.h"
#include <regex.h>
#include <stdlib.h>
#include <string.h>

BufferSalida bufferSalida = {NULL, 0, 0};

/* ─── helpers de salida ─────────────────────────────────────────── */

/** @brief agrega una linea de salida al buffer dinamico */
void agregarLineaSalida(const char *path, size_t start, size_t end) {
  if (bufferSalida.count >= bufferSalida.capacity) {
    bufferSalida.capacity =
        bufferSalida.capacity == 0 ? 100 : bufferSalida.capacity * 2;
    bufferSalida.lines = (LineaSalida *)realloc(
        bufferSalida.lines, bufferSalida.capacity * sizeof(LineaSalida));
    if (!bufferSalida.lines) {
      perror("Fallo la reasignacion de memoria para bufferSalida.lines");
      exit(EXIT_FAILURE);
    }
  }
  strncpy(bufferSalida.lines[bufferSalida.count].path, path,
          sizeof(bufferSalida.lines[0].path) - 1);
  bufferSalida.lines[bufferSalida.count]
      .path[sizeof(bufferSalida.lines[0].path) - 1] = '\0';
  bufferSalida.lines[bufferSalida.count].start = start;
  bufferSalida.lines[bufferSalida.count].end = end;
  bufferSalida.count++;
}

/** @brief compara dos lineas de salida por posicion inicial para ordenamiento
 */
int compararLineasSalida(const void *a, const void *b) {
  const LineaSalida *lineA = (const LineaSalida *)a;
  const LineaSalida *lineB = (const LineaSalida *)b;
  if (lineA->start != lineB->start)
    return lineA->start - lineB->start;
  return lineA->end - lineB->end;
}

/** @brief ordena e imprime todas las lineas de salida al archivo */
void imprimirBufferSalida(FILE *out) {
  qsort(bufferSalida.lines, bufferSalida.count, sizeof(LineaSalida),
        compararLineasSalida);
  fprintf(out, "%-39s\t%-14s\t%-14s\n", "Ruta (path)", "Inicio (byte)",
          "Fin (byte)");
  for (int i = 0; i < 67; i++)
    fprintf(out, "-");
  fprintf(out, "\n");
  for (int i = 0; i < bufferSalida.count; i++) {
    if (strcmp(bufferSalida.lines[i].path, ""))
      fprintf(out, "%-39s\t%-14zu\t%-14zu\n", bufferSalida.lines[i].path,
              bufferSalida.lines[i].start, bufferSalida.lines[i].end);
  }
}

/* ─── peek y next del parser ─────────────────────────────────────────────── */

/** @brief avanza sobre espacios en blanco y retorna el siguiente caracter */
char peek(Parser *p) {
  while (p->src[p->pos] == ' ' || p->src[p->pos] == '\t' ||
         p->src[p->pos] == '\n' || p->src[p->pos] == '\r') {
    p->pos++;
  }
  return p->src[p->pos];
}

/** @brief avanza al siguiente caracter en el parser */
char next(Parser *p) {
  char c = peek(p);
  if (c != '\0') {
    p->pos++;
  }
  return c;
}

/* ─── parsers dependiendo del tipo ──────────────────────────────────────── */

/** @brief parsea la clave entrecomillada de un objeto y la escribe en keyOut */
static void parsearClave(Parser *p, char *keyOut, size_t keySize) {
  next(p); /* consume las comillas que abren '"' */
  size_t k = 0;
  while (p->src[p->pos] != '"' && p->src[p->pos] != '\0') {
    if (k < keySize - 1)
      keyOut[k++] = p->src[p->pos];
    p->pos++;
  }
  keyOut[k] = '\0';
  if (p->src[p->pos] == '"')
    p->pos++; /* consume las comillas que cierran '"' */
}

/** @brief parsea un objeto JSON { ... } */
static void parsearObjeto(Parser *p, const char *path) {
  size_t objStart = p->pos;
  next(p); /* consume '{' */

  char key[128];
  char newPath[512];
  char c = peek(p);

  while (c != '}' && c != '\0') {
    if (c == '"') {
      parsearClave(p, key, sizeof(key));
      c = peek(p);
      if (c == ':')
        next(p);
      snprintf(newPath, sizeof(newPath), "%s/%s", path, key);
      parsear(p, newPath, NULL);
    } else if (c == ',') {
      next(p);
    } else {
      next(p);
    }
    c = peek(p);
  }

  if (c == '}') {
    next(p);
    agregarLineaSalida(path, objStart, p->pos - 1);
  }
}

/** @brief parsea un arreglo JSON [ ... ] */
static void parsearArreglo(Parser *p, const char *path) {
  size_t arrStart = p->pos;
  next(p); /* consume '[' */

  int arrayIndex = 0;
  char newPath[512];
  char c = peek(p);

  while (c != ']' && c != '\0') {
    snprintf(newPath, sizeof(newPath), "%s/%d", path, arrayIndex);
    parsear(p, newPath, NULL);
    c = peek(p);
    if (c == ',') {
      next(p);
      arrayIndex++;
      c = peek(p);
    } else if (c != ']') {
      next(p);
      c = peek(p);
    }
  }

  if (c == ']') {
    next(p);
    agregarLineaSalida(path, arrStart, p->pos - 1);
  }
}

/** @brief parsea una cadena JSON "..." */
static void parsearCadena(Parser *p, const char *path) {
  size_t strStart = p->pos;
  next(p); /* consume comillas que abren '"' */
  while (p->src[p->pos] != '"' && p->src[p->pos] != '\0') {
    p->pos++;
  }
  if (p->src[p->pos] == '"')
    p->pos++;
  agregarLineaSalida(path, strStart, p->pos - 1);
}

/** @brief retorna 1 si c inicia un valor primitivo (numero, true, false, null)
 */
static int esPrimitivo(char c) {
  return (c >= '0' && c <= '9') || c == '-' || c == 't' || c == 'f' || c == 'n';
}

/** @brief retorna 1 si c es parte del cuerpo de un valor primitivo */
static int esCaracterPrimitivo(char c) {
  return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == 'e' ||
         c == 'E' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/** @brief parsea un valor primitivo (numero, true, false, null) */
static void parsearPrimitivo(Parser *p, const char *path) {
  size_t valStart = p->pos;
  while (esCaracterPrimitivo(p->src[p->pos])) {
    p->pos++;
  }
  agregarLineaSalida(path, valStart, p->pos - 1);
}

/* ─── funcion principal del parser ───────────────────────────────────────────
 */

/** @brief parsea recursivamente un json y registra todas las rutas y posiciones
 */
void parsear(Parser *p, const char *path, FILE *out) {
  char c = peek(p);

  if (c == '{') {
    parsearObjeto(p, path);
  } else if (c == '[') {
    parsearArreglo(p, path);
  } else if (c == '"') {
    parsearCadena(p, path);
  } else if (esPrimitivo(c)) {
    parsearPrimitivo(p, path);
  } else if (c == '}' || c == ']') {
    return; /* finaliza el objeto o arreglo */
  } else if (c == ',' || c == ':') {
    next(p); /* saltar delimitadores */
  } else if (c != '\0') {
    next(p); /* cualquier cosa que no sea reconocida */
  }
}

/* ─── helpers para la busqueda ──────────────────────────────────────── */

/** @brief construye el patron regex efectivo ('$' inicial -> '^') */
static char *buildRegexPattern(const char *regexPattern) {
  if (regexPattern[0] == '$') {
    char *pat = (char *)malloc(strlen(regexPattern) + 1);
    if (!pat)
      return NULL;
    pat[0] = '^';
    strcpy(pat + 1, regexPattern + 1);
    return pat;
  }
  return strdup(regexPattern);
}

/** @brief lee el contenido completo de un archivo en un buffer con terminador
 * nulo */
static char *readFileContents(const char *filePath) {
  FILE *f = fopen(filePath, "r");
  if (!f) {
    perror("Error al abrir el archivo");
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = (char *)malloc(size + 1);
  if (!buf) {
    perror("Error de asignacion de memoria");
    fclose(f);
    return NULL;
  }

  size_t readBytes = fread(buf, 1, size, f);
  buf[readBytes] = '\0';
  fclose(f);
  return buf;
}

/** @brief salta las dos primeras lineas (encabezado) del archivo .jnx */
static int skipJnxHeader(FILE *jnxFile) {
  char line[1024];
  if (fgets(line, sizeof(line), jnxFile) == NULL)
    return -1;
  if (fgets(line, sizeof(line), jnxFile) == NULL)
    return -1;
  return 0;
}

/** @brief extrae un fragmento del contenido JSON entre [start, end] */
static char *extractMatchedValue(const char *jsonContent, size_t start,
                                 size_t end) {
  size_t len = end - start + 1;
  char *value = (char *)malloc(len + 1);
  if (!value) {
    perror("Error de asignacion de memoria para el valor extraido");
    return NULL;
  }
  strncpy(value, jsonContent + start, len);
  value[len] = '\0';
  return value;
}

/** @brief agrega un valor al SearchResult, crece el arreglo si es necesario */
static void appendSearchResult(SearchResult *result, char *value) {
  if (result->count >= result->capacity) {
    result->capacity = result->capacity == 0 ? 10 : result->capacity * 2;
    result->values =
        (char **)realloc(result->values, result->capacity * sizeof(char *));
    if (!result->values) {
      perror("Fallo la reasignacion de memoria para SearchResult.values");
      exit(EXIT_FAILURE);
    }
  }
  result->values[result->count++] = value;
}

/* ─── funcion principal del search ───────────────────────────────────────────
 */

/** @brief busca en el archivo .jnx usando una expresion regular y retorna los
 * valores del JSON original */
SearchResult searchJson(const char *jnxFilePath, const char *jsonFilePath,
                        const char *regexPattern) {
  SearchResult result = {NULL, 0, 0};
  regex_t regex;
  int regexCompiled = 0;
  char *effectivePattern = NULL;
  FILE *jnxFile = NULL;
  char *jsonContent = NULL;

  /* 1. construye el regex */
  effectivePattern = buildRegexPattern(regexPattern);
  if (!effectivePattern)
    goto cleanup;

  char msgbuf[100];
  int reti = regcomp(&regex, effectivePattern, REG_EXTENDED);
  if (reti) {
    regerror(reti, &regex, msgbuf, sizeof(msgbuf));
    fprintf(stderr, "Error de compilacion de RegEx: %s\n", msgbuf);
    goto cleanup;
  }
  regexCompiled = 1;

  /* 2. abre el .jnx y se salta el encabezado (dos lineas primeras) */
  jnxFile = fopen(jnxFilePath, "r");
  if (!jnxFile) {
    perror("Error al abrir el archivo .jnx");
    goto cleanup;
  }
  if (skipJnxHeader(jnxFile) < 0) {
    fprintf(stderr, "Archivo .jnx vacio o error de lectura.\n");
    goto cleanup;
  }

  /* 3. lee todo el contenido del json */
  jsonContent = readFileContents(jsonFilePath);
  if (!jsonContent)
    goto cleanup;

  /* 4. escanea las lineas del .jnx, coincide con regex y extrae valores */
  char line[1024];
  while (fgets(line, sizeof(line), jnxFile) != NULL) {
    char path[512];
    size_t start, end;
    if (sscanf(line, "%s\t%zu\t%zu", path, &start, &end) != 3)
      continue;

    if (regexec(&regex, path, 0, NULL, 0) != 0)
      continue;

    char *value = extractMatchedValue(jsonContent, start, end);
    if (!value)
      continue;

    appendSearchResult(&result, value);
  }

cleanup:
  free(jsonContent);
  if (jnxFile)
    fclose(jnxFile);
  if (regexCompiled)
    regfree(&regex);
  free(effectivePattern);
  return result;
}

/** @brief libera la memoria de un SearchResult */
void freeSearchResult(SearchResult *result) {
  if (result) {
    for (int i = 0; i < result->count; i++) {
      free(result->values[i]);
    }
    free(result->values);
    result->values = NULL;
    result->count = 0;
    result->capacity = 0;
  }
}