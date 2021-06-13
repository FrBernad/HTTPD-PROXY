#ifndef _CLIENT_ARGS_H_ec6740ffe72b70522687907e4596f151a01ec90d5fa9a23e8c972702
#define _CLIENT_ARGS_H_ec6740ffe72b70522687907e4596f151a01ec90d5fa9a23e8c972702

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
