#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    const char* src;
    size_t pos;
} Parser;

typedef struct {
    char path[512];
    size_t start;
    size_t end;
} LineaSalida;

typedef struct {
    LineaSalida* lines;
    int count;
    int capacity;
} BufferSalida;

/** @brief avanza sobre espacios en blanco y retorna el siguiente caracter */
char peek(Parser* p);

/** @brief avanza al siguiente caracter en el parser */
char next(Parser* p);

/** @brief parsea recursivamente un json y registra todas las rutas y posiciones */
void parsear(Parser* p, const char* path, FILE* out);

/** @brief ordena e imprime todas las lineas de salida al archivo */
void imprimirBufferSalida(FILE* out);