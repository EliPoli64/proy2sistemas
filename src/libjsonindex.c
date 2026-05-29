#include "../include/libjsonindex.h"

Node* anadirNodo(Node* nodo, size_t charActual, int nivel, int arrayIndex) {
    Node* actual = nodo;

    while (actual->next != NULL) {
        actual = actual->next;
    }

    actual->next = (Node*)malloc(sizeof(Node));
    actual->next->next = NULL;
    actual->next->start = charActual;
    actual->next->end = charActual;
    actual->next->lexema = NULL;
    actual->next->nivel = nivel;
    actual->next->arrayIndex = arrayIndex;

    return actual->next;
}

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

void parseObject(Parser* p, const char* path, FILE* out) {
    char c = peek(p);
    int arrayIndex = 0;
    int expectKey = 0;
    char key[128] = "";
    char newPath[512];

    while (c != '\0') {
        if (c == '{') {
            size_t objStart = p->pos;
            next(p);
            fprintf(out, "%s\t%zu\t%zu\n", path, objStart, (size_t)0);
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
                    parseObject(p, newPath, out);
                } else if (c == ',') {
                    next(p);
                } else {
                    next(p);
                }
                c = peek(p);
            }
            if (c == '}') {
                next(p);
                fprintf(out, "%s\t%zu\t%zu\n", path, objStart, p->pos);
            }
            return;
        } else if (c == '[') {
            size_t arrStart = p->pos;
            next(p);
            fprintf(out, "%s\t%zu\t%zu\n", path, arrStart, (size_t)0);
            arrayIndex = 0;
            c = peek(p);
            while (c != ']' && c != '\0') {
                snprintf(newPath, sizeof(newPath), "%s/%d", path, arrayIndex);
                parseObject(p, newPath, out);
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
                fprintf(out, "%s\t%zu\t%zu\n", path, arrStart, p->pos);
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
            fprintf(out, "%s\t%zu\t%zu\n", path, strStart, p->pos);
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
            fprintf(out, "%s\t%zu\t%zu\n", path, valStart, p->pos);
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