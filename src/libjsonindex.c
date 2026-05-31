#include "../include/libjsonindex.h"

char peek(Parser* p) {
    while (p->src[p->pos] == ' '  || p->src[p->pos] == '\t' || p->src[p->pos] == '\n' || p->src[p->pos] == '\r') {
        p->pos++;
    }

    return p->src[p->pos];
}

char next(Parser* p) {
    char c = peek(p);

    if (c != '\0') {
        p->pos++;
    }

    return c;
}

void imprimirLista(Node* nodo, char* ruta, FILE* jnx) {
    Node* actual = nodo;
    char* rutaActual = (char*)malloc(256);
    strcpy(rutaActual, ruta);
    if (actual == NULL) return;
    if (actual->lexema != NULL) {
      if (actual->arrayIndex == -1) {
        strncat(rutaActual, actual->lexema, 128);
        strncat(rutaActual, "/", 2);
        fprintf(jnx, "%s, Inicio: %zu, Final: %zu\n", rutaActual, actual->start, actual->end);
      } else {
        char buffer[3];
        snprintf(buffer, sizeof(buffer), "%d", actual->arrayIndex);
        strncat(rutaActual, "/", 2);
        strncat(rutaActual, buffer, 128);
        fprintf(jnx, "%s, Inicio: %zu, Final: %zu\n", rutaActual, actual->start, actual->end);
      }
    }
    imprimirLista(actual->next, rutaActual, jnx);
}

void imprimirPath(const char* path, size_t start, size_t end, FILE* out) {
    fprintf(out, "%s\t%zu\t%zu\n", path, start, end);
}

void parseObjeto(Parser* p, const char* path, FILE* out) {
    char c = peek(p);
    int arrayIndex = 0;
    int expectKey = 0;
    char key[128] = "";
    char newPath[512];

    while (c != '\0') {
        if (c == '{') {
            size_t objStart = p->pos;
            next(p);
            expectKey = 1;
            c = peek(p);
            while (c != '}' && c != '\0') {
                if (c == '"') {
                    size_t keyStart = p->pos;
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
                    parseObjeto(p, newPath, out);
                } else if (c == ',') {
                    next(p);
                } else {
                    next(p);
                }
                c = peek(p);
            }
            if (c == '}') {
                next(p);
                if (strlen(path) > 0) { // si esta vacio entonces no imprime
                    fprintf(out, "%s\t%zu\t%zu\n", path, objStart, p->pos);
                }
            }
            return;
        } else if (c == '[') {
            size_t arrStart = p->pos;
            next(p);
            arrayIndex = 0;
            c = peek(p);
            while (c != ']' && c != '\0') {
                snprintf(newPath, sizeof(newPath), "%s/%d", path, arrayIndex);
                parseObjeto(p, newPath, out);
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
                if (strlen(path) > 0) { // si esta vacio entonces no imprime
                    fprintf(out, "%s\t%zu\t%zu\n", path, arrStart, p->pos);
                }
            }
            return;
        } else if (c == '"') {
            size_t strStart = p->pos;
            next(p);
            size_t valStart = p->pos;
            while (p->src[p->pos] != '"' && p->src[p->pos] != '\0') {
                p->pos++;
            }
            size_t valEnd = p->pos;
            if (p->src[p->pos] == '"') p->pos++;
            if (strlen(path) > 0) {
                fprintf(out, "%s\t%zu\t%zu\n", path, strStart, p->pos);
            }
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
            if (strlen(path) > 0) {
                fprintf(out, "%s\t%zu\t%zu\n", path, valStart, p->pos);
            }
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

/*
Lee el .jnx y devuelve un arreglo de entries que contiene.
El JnxEntry tiene la ruta, el byte de inicio y fin, para no recuperarlos dos veces.q
*/
JnxEntry* parsearJnx(const char* pathJNX, int* numeroEntradas) {
    FILE* file = fopen(pathJNX, "r");

    // saltarse las dos primeras lineas del jnx
    char line[1024];
    fgets(line, sizeof(line), file);
    fgets(line, sizeof(line), file);

    // inicialmente son 10 entradas, si se ocupan mas se duplican.
    int capacidad = 10;
    *numeroEntradas = 0;
    JnxEntry* entries  = (JnxEntry*)malloc(sizeof(JnxEntry) * capacidad);

    while (fgets(line, sizeof(line), file) != NULL) {
        char* pathStr       = strtok(line, "\t");
        char* byteInicioStr = strtok(NULL, "\t");
        char* byteFinStr    = strtok(NULL, "\n");

        if (*numeroEntradas >= capacidad) {
            capacidad *= 2;
            entries = (JnxEntry*)realloc(entries, sizeof(JnxEntry) * capacidad);
        }

        entries[*numeroEntradas].path      = strdup(pathStr);
        entries[*numeroEntradas].byteInicio = (size_t)atol(byteInicioStr);
        entries[*numeroEntradas].byteFin   = (size_t)atol(byteFinStr);
        (*numeroEntradas)++;
    }

    fclose(file);
    return entries;
}

JnxEntry* filtrarEntradasRegex(JnxEntry* entries, int numeroEntradas, const char* patronRegex, int* totalCoincidencias) {
    regex_t regex;
    int resultadoComp;

    int capacidad = 10;
    *totalCoincidencias = 0;
    JnxEntry* listaFiltrada  = (JnxEntry*)malloc(sizeof(JnxEntry) * capacidad);

    resultadoComp = regcomp(&regex, patronRegex, REG_EXTENDED);

    // el regex puede venir mal
    if (resultadoComp) {
        regfree(&regex);
        printf("Regex mal formado: %s\n", patronRegex);
        return NULL;
    }


    for (int i = 0; i < numeroEntradas; ++i) {
        // se ejecuta el regex contra cada path
        resultadoComp = regexec(&regex, entries[i].path, 0, NULL, 0);

        if (!resultadoComp) { // 0 = match
            if (*totalCoincidencias >= capacidad) {
                capacidad *= 2;
                listaFiltrada = (JnxEntry*)realloc(listaFiltrada, sizeof(JnxEntry) * capacidad);
            }
            listaFiltrada[*totalCoincidencias].path       = strdup(entries[i].path);
            listaFiltrada[*totalCoincidencias].byteInicio = entries[i].byteInicio;
            listaFiltrada[*totalCoincidencias].byteFin    = entries[i].byteFin;
            (*totalCoincidencias)++;
        }
    }

    regfree(&regex);
    return listaFiltrada;
}

char* leerValorJson(const char* pathJson, size_t byteInicio, size_t byteFin) {
    FILE* json = fopen(pathJson, "r");

    size_t tamanoValor = byteFin - byteInicio;
    if (tamanoValor == 0) { // si son iguales no hay nada (no deberia pasar pero aja)
        fclose(json);
        return strdup("");
    }

    char* valor = (char*)malloc(tamanoValor + 1);

    // mueve el puntero al byte de inicio
    fseek(json, byteInicio, SEEK_SET);

    // lee el valor
    fread(valor, 1, tamanoValor, json);

    valor[tamanoValor] = '\0'; // termina la cadena con null para no tener problema

    fclose(json);
    return valor;
}

char** recuperarValoresJson(const char* pathJNX, const char* pathJson, const char* patronRegex, int* totalCoincidencias) {
    
    int totalEntradas = 0;
    int totalEntradasFiltradas = 0;

    JnxEntry* entradas = parsearJnx(pathJNX, &totalEntradas);
    JnxEntry* entradasFiltradas = filtrarEntradasRegex(entradas, totalEntradas, patronRegex, &totalEntradasFiltradas);

    // liberar memoria
    for (int i = 0; i < totalEntradas; ++i) {
        free(entradas[i].path);
    }
    free(entradas);

    char** valoresJson = (char**)malloc(sizeof(char*) * totalEntradasFiltradas);

    for (int i = 0; i < totalEntradasFiltradas; ++i) {
        valoresJson[i] = leerValorJson(pathJson, entradasFiltradas[i].byteInicio, entradasFiltradas[i].byteFin);
    }

    for (int i = 0; i < totalEntradasFiltradas; ++i) {
        free(entradasFiltradas[i].path);
    }
    free(entradasFiltradas);

    *totalCoincidencias = totalEntradasFiltradas;
    return valoresJson;
}