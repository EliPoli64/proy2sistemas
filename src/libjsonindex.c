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

/** @brief parsea recursivamente un json y registra todas las rutas y posiciones */
void parsear(Parser* p, const char* path, FILE* out) {
    char c = peek(p);
    int arrayIndex = 0;
    char key[128] = "";
    char newPath[512];

    if (c == '{') {
        size_t objStart = p->pos;
        next(p);
        c = peek(p);
        
        while (c != '}' && c != '\0') {
            if (c == '"') {
                next(p);
                size_t k = 0;
                while (p->src[p->pos] != '"' && p->src[p->pos] != '\0') {
                    if (k < sizeof(key) - 1) key[k++] = p->src[p->pos];
                    p->pos++;
                }
                key[k] = '\0';
                if (p->src[p->pos] == '"') p->pos++;
                
                c = peek(p);
                if (c == ':') next(p);  // consume ':'
                
                snprintf(newPath, sizeof(newPath), "%s/%s", path, key);
                parsear(p, newPath, out);
            } else if (c == ',') {
                next(p);
            } else {
                next(p);
            }
            c = peek(p);
        }
        
        if (c == '}') {
            next(p);  // consume '}'
            agregarLineaSalida(path, objStart, p->pos - 1);
        }
        return;
        
    } else if (c == '[') {
        size_t arrStart = p->pos;
        next(p);
        arrayIndex = 0;
        c = peek(p);
        
        while (c != ']' && c != '\0') {
            snprintf(newPath, sizeof(newPath), "%s/%d", path, arrayIndex);
            parsear(p, newPath, out);
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
            next(p);  // consume ']'
            agregarLineaSalida(path, arrStart, p->pos - 1);
        }
        return;
        
    } else if (c == '"') {
        size_t strStart = p->pos;
        next(p);
        while (p->src[p->pos] != '\0' && p->src[p->pos] != '"') {
            p->pos++;
        }
        if (p->src[p->pos] == '"') {
            p->pos++;
        }
        agregarLineaSalida(path, strStart + 1, p->pos - 2);
        return;
        
    } else if ((c >= '0' && c <= '9') || c == '-' || c == 't' || c == 'f' || c == 'n') {
        size_t valStart = p->pos;
        while (
            (p->src[p->pos] >= '0' && p->src[p->pos] <= '9') ||
            p->src[p->pos] == '-' || p->src[p->pos] == '+' ||
            p->src[p->pos] == 'e' || p->src[p->pos] == 'E' ||
            (p->src[p->pos] >= 'a' && p->src[p->pos] <= 'z') ||
            (p->src[p->pos] >= 'A' && p->src[p->pos] <= 'Z')
        ) {
            p->pos++;
        }
        agregarLineaSalida(path, valStart, p->pos - 1);
        return;
        
    } else if (c == '}' || c == ']') {
        return;
        
    } else if (c == ',' || c == ':') {
        next(p);
        
    } else if (c != '\0') {
        next(p);
    }
}

/** @brief busca en el archivo .jnx usando una expresion regular y retorna los
 * valores del JSON original */
ResultadoBusq buscar(const char *jnxFilePath, const char *jsonFilePath,
                        const char *regexPattern) {
  ResultadoBusq result = {NULL, 0, 0};
  regex_t regex;
  int regexCompiled = 0;
  char *effectivePattern = NULL;
  FILE *jnxFile = NULL;
  char *jsonContent = NULL;

  /* 1. construye el regex */
  if (regexPattern[0] == '$') {
    effectivePattern = (char *)malloc(strlen(regexPattern) + 1);
    if (!effectivePattern) goto cleanup;
    effectivePattern[0] = '^';
    strcpy(effectivePattern + 1, regexPattern + 1);
  } else {
    effectivePattern = strdup(regexPattern);
  }
  if (!effectivePattern) goto cleanup;

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
  char line[1024];
  if (fgets(line, sizeof(line), jnxFile) == NULL) goto cleanup;
  if (fgets(line, sizeof(line), jnxFile) == NULL) goto cleanup;

  /* 3. lee todo el contenido del json */
  FILE *f = fopen(jsonFilePath, "r");
  if (!f) {
    perror("Error al abrir el archivo");
    goto cleanup;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  jsonContent = (char *)malloc(size + 1);
  if (!jsonContent) {
    perror("Error de asignacion de memoria");
    fclose(f);
    goto cleanup;
  }
  size_t readBytes = fread(jsonContent, 1, size, f);
  jsonContent[readBytes] = '\0';
  fclose(f);

  /* 4. escanea las lineas del .jnx, coincide con regex y extrae valores */
  while (fgets(line, sizeof(line), jnxFile) != NULL) {
    char path[512];
    size_t start, end;
    if (sscanf(line, "%s\t%zu\t%zu", path, &start, &end) != 3) continue;
    if (regexec(&regex, path, 0, NULL, 0) != 0) continue;

    /* extrae el valor */
    size_t len = end - start + 1;
    char *value = (char *)malloc(len + 1);
    if (!value) continue;
    strncpy(value, jsonContent + start, len);
    value[len] = '\0';
    
    /* para objetos o arrays, elimina whitespace pero mantiene llaves */
    if (value[0] == '{' || value[0] == '[') {
      char *clean = (char *)malloc(len + 1);
      if (!clean) {
        free(value);
        continue;
      }
      int j = 0;
      int inString = 0;
      for (size_t i = 0; i < len; i++) {
        char c = value[i];
        if (c == '"') {
          inString = !inString;
          clean[j++] = c;
        } else if (!inString && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
          continue; /* salta whitespace */
        } else {
          clean[j++] = c;
        }
      }
      clean[j] = '\0';
      free(value);
      value = clean;
    }
    
    if (result.count >= result.capacity) {
      result.capacity = result.capacity == 0 ? 10 : result.capacity * 2;
      result.values = (char **)realloc(result.values, result.capacity * sizeof(char *));
      if (!result.values) {
        perror("Fallo la reasignacion de memoria para ResultadoBusq.values");
        free(value);
        goto cleanup;
      }
    }
    result.values[result.count++] = value;
  }

cleanup:
  free(jsonContent);
  if (jnxFile) fclose(jnxFile);
  if (regexCompiled) regfree(&regex);
  free(effectivePattern);
  return result;
}

/** @brief libera la memoria de un ResultadoBusq */
void liberarResultado(ResultadoBusq* result) {
  if (!result) return;
  for (int i = 0; i < result->count; i++) {
    free(result->values[i]);
  }
  free(result->values);
  result->values = NULL;
  result->count = 0;
  result->capacity = 0;
}