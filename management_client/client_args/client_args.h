#ifndef _CLIENT_ARGS_H_
#define _CLIENT_ARGS_H_

#include <stdbool.h>

struct client_args{
    char           *client_addr;
    unsigned short  client_port;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecucion.
 */
void 
parse_args(int argc, char **argv, struct client_args *args);

#endif
