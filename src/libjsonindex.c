#include "../include/libjsonindex.h"
#include <stdlib.h>
#include <string.h>

BufferSalida bufferSalida = {NULL, 0, 0};

/** @brief agrega una linea de salida al buffer dinamico */
void agregarLineaSalida(const char* path, size_t start, size_t end) {
    if (bufferSalida.count >= bufferSalida.capacity) {
        bufferSalida.capacity = bufferSalida.capacity == 0 ? 100 : bufferSalida.capacity * 2;
        bufferSalida.lines = (LineaSalida*)realloc(bufferSalida.lines, bufferSalida.capacity * sizeof(LineaSalida));
    }
    strcpy(bufferSalida.lines[bufferSalida.count].path, path);
    bufferSalida.lines[bufferSalida.count].start = start;
    bufferSalida.lines[bufferSalida.count].end = end;
    bufferSalida.count++;
}

/** @brief compara dos lineas de salida por posicion inicial para ordenamiento */
int compararLineasSalida(const void* a, const void* b) {
    const LineaSalida* lineA = (const LineaSalida*)a;
    const LineaSalida* lineB = (const LineaSalida*)b;
    if (lineA->start != lineB->start) return lineA->start - lineB->start;
    return lineA->end - lineB->end;
}

/** @brief ordena e imprime todas las lineas de salida al archivo */
void imprimirBufferSalida(FILE* out) {
    qsort(bufferSalida.lines, bufferSalida.count, sizeof(LineaSalida), compararLineasSalida);
    fprintf(out, "%-39s\t%-14s\t%-14s\n", "Ruta (path)", "Inicio (byte)", "Fin (byte)");
    for (int i = 0; i < 67; i++) fprintf(out, "-");
    fprintf(out, "\n");
    for (int i = 0; i < bufferSalida.count; i++) {
      if (strcmp(bufferSalida.lines[i].path, ""))
        fprintf(out, "%-39s\t%-14zu\t%-14zu\n", bufferSalida.lines[i].path, bufferSalida.lines[i].start, bufferSalida.lines[i].end);
    }
}

/** @brief avanza sobre espacios en blanco y retorna el siguiente caracter */
char peek(Parser* p) {
    while (p->src[p->pos] == ' '  || p->src[p->pos] == '\t' || p->src[p->pos] == '\n' || p->src[p->pos] == '\r') {
        p->pos++;
    }

    return p->src[p->pos];
}

/** @brief avanza al siguiente caracter en el parser */
char next(Parser* p) {
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

    while (c != '\0') {
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
                    if (c == ':') next(p);
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
                next(p);
                agregarLineaSalida(path, objStart + 1, p->pos + 1);
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
                } else if (c != ']') { // no esperado???
                    next(p);
                    c = peek(p);
                }
            }
            if (c == ']') {
                next(p);
                agregarLineaSalida(path, arrStart + 1, p->pos + 1);
            }
            return;
        } else if (c == '"') {
            size_t strStart = p->pos;
            next(p);
            while (p->src[p->pos] != '"' && p->src[p->pos] != '\0') {
                p->pos++;
            }
            if (p->src[p->pos] == '"') p->pos++;
            agregarLineaSalida(path, strStart + 1, p->pos + 1);
            return;
        } else if ((c >= '0' && c <= '9') || c == '-' || c == 't' || c == 'f' || c == 'n') { // numero, letra, true, false, null
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
            agregarLineaSalida(path, valStart + 1, p->pos + 1);
            return;
        } else if (c == ',') {
            next(p);
        } else if (c == ':') {
            next(p);
        } else if (c == '}' || c == ']') { // fin array
            return;
        } else {
            next(p);
        }
        c = peek(p);
    }
}