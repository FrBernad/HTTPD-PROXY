
#include "client_args.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h> /* LONG_MIN et al */
#include <stdio.h>  /* for printf */
#include <stdlib.h> /* for exit */
#include <string.h> /* memset */

static unsigned short
port(char *s) {
    char *end = 0;
    long sl = strtol(s, &end, 10);

    if (end == s || '\0' != *end || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) || sl < 0 || sl > USHRT_MAX) {
        fprintf(stderr, "Port should in in the range of 1-65536: %s\n", s);
        exit(1);
        return 1;
    }
    return (unsigned short)sl;
}

static void
usage(char *progname) {
    fprintf(stderr,
            "Usage: %s [OPTION]...\n\n"
            "   -h                                 Imprime la ayuda y termina.\n\n"
            "   -L <dirección-de-management>       Establece la dirección donde esta sirviendo el servicio de management.\n"
            "                                      Por defecto utiliza loop‐back.\n\n"
            "   -o <puerto-de-management>          Puerto donde se encuentra el servidor de management a conectarse\n"
            "                                      Por defecto el valor es 9090.\n\n",
            progname);

    exit(1);
}

void
parse_args(int argc, char **argv, struct client_args *args){

    memset(args, 0, sizeof(*args));

    args->client_addr = NULL;
    args->client_port = 9090;

    int c;

    while (true) {
        int option_index = 0;

        c = getopt_long(argc, argv, "hL:o:", NULL, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'L':
                args->client_addr = optarg;
                break;
            case 'o':
                args->client_port = port(optarg);
                break;
            default:
                fprintf(stderr, "unknown argument %d.\n", c);
                exit(1);
        }
    }
    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
}
