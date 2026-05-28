main:
	gcc main.c src/libjsonindex.c \
	-Iinclude -Wall -Wextra -g -O0 -o jqindex