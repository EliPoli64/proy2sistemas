#pragma once

#include <stddef.h>
typedef struct {
    const char* src;
    size_t pos;
} Parser;

char peek(Parser* p);
char next(Parser* p);

void parseObject(Parser* p);