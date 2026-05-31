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

typedef struct node {
    size_t start;
    size_t end;
    char* lexema;
    int arrayIndex; // -1 si no esta en array, 0 en adelante si lo esta
    int nivel;
    struct node* next;
    
} Node;

char peek(Parser* p);
char next(Parser* p);
void imprimirLista(Node* nodo, char* ruta, FILE* jnx);
void parseObjeto(Parser* p, const char* path, FILE* out);

typedef struct {
    char* path;
    size_t byteInicio;
    size_t byteFin;
} JnxEntry;

JnxEntry* parsearJnx(const char* pathJNX, int* numeroEntradas);
JnxEntry* filtrarEntradasRegex(JnxEntry* entries, int numeroEntradas, const char* patronRegex, int* totalCoincidencias);
char* leerValorJson(const char* pathJson, size_t byteInicio, size_t byteFin);
char** recuperarValoresJson(const char* pathJNX, const char* pathJson, const char* patronRegex, int* totalCoincidencias);