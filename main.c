#include "include/libjsonindex.h"

int main() {
  
  Parser p = {
        .src = "{\"users\":1}",
        .pos = 0
  };

  parseObject(&p);

  return 0;
}