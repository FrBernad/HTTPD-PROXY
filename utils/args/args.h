#ifndef _ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define _ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdbool.h>

struct doh {
    char           *host;
    char           *ip;
    unsigned short  port;
    char           *path;
    char           *query;
};

struct http_args{
    char           *http_addr;
    unsigned short  http_port;

    char *          mng_addr;
    unsigned short  mng_port;

    bool            disectors_enabled;

    struct doh      doh;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecucion.
 */
void 
parse_args(int argc, char **argv, struct http_args *args);

#endif
