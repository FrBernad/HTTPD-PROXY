CFLAGS+=-std=c11 

CFLAGS+=-D_POSIX_C_SOURCE=200112L -Wall -g -pedantic -pthread -pedantic-errors -Wextra -Werror -fsanitize=address

CFLAGS+=-O3 

CFLAGS+=-Wno-unused-parameter -Wno-implicit-fallthrough -Wno-newline-eof