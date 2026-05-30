#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <regex.h>

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

extern BufferSalida bufferSalida;

typedef struct {
    char** values;
    int count;
    int capacity;
} SearchResult;

/** @brief avanza sobre espacios en blanco y retorna el siguiente caracter */
char peek(Parser* p);

/** @brief avanza al siguiente caracter en el parser */
char next(Parser* p);

/** @brief parsea recursivamente un json y registra todas las rutas y posiciones */
void parsear(Parser* p, const char* path, FILE* out);

/** @brief ordena e imprime todas las lineas de salida al archivo */
void imprimirBufferSalida(FILE* out);

/** @brief busca en el archivo .jnx usando una expresion regular y retorna los valores del JSON original */
SearchResult searchJson(const char* jnxFilePath, const char* jsonFilePath, const char* regexPattern);

/** @brief libera la memoria de un SearchResult */
void freeSearchResult(SearchResult* result);