#include <stdio.h>
#include <stdlib.h>

#include "../include/libjsonindex.h"

char peek(Parser* p) {
    return p->src[p->pos];
}

char next(Parser* p) {
    return p->src[p->pos++];
}

void parseObject(Parser* p) {
    next(p);
    while (peek(p) != '}') {
        // una llave
        if (peek(p) == '"') {
            next(p);

            while(peek(p) != '"') {
                printf("%c", next(p));
            }

            next(p);

            printf("\n");
        }


        while (
            peek(p) != '{' && peek(p) != '}' && peek(p) != '\0'
        ) {
            next(p);
        }

        // ...
    }
    next(p);
}