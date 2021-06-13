#include "args.h"

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
version(void) {
    fprintf(stderr,
            "HTTP proxy version 1.0\n"
            "ITBA Protocolos de Comunicación 2021/1 -- Grupo 10\n");
}

static void
usage(char *progname) {
    fprintf(stderr,
            "Usage: %s [OPTION]...\n\n"
            "   -h                                 Imprime la ayuda y termina.\n\n"
            "   -N                                 Deshabilita los passwords disectors.\n\n"
            "   -l <dirección-http>                Establece la dirección donde servirá el proxy HTTP.\n"
            "                                      Por defecto escucha en todas las interfaces.\n\n"
            "   -L <dirección-de-management>       Establece la dirección donde servirá el servicio de management.\n"
            "                                      Por defecto escucha únicamente en loop‐back.\n\n"
            "   -o <puerto-de-management>          Puerto donde se encuentra el servidor de management\n"
            "                                      Por defecto el valor es 9090.\n\n"
            "   -p <puerto-local>                  Puerto TCP donde escuchará por conexiones entrantes HTTP.\n"
            "                                      Por defecto el valor es 8080.\n\n"
            "   -v                                 Imprime información sobre la versión versión y termina.\n\n\n"
            "   --doh-ip dirección-doh             Establece la dirección del servidor DoH.  Por defecto 127.0.0.1.\n\n"
            "   --doh-port port                    Establece el puerto del servidor DoH.  Por defecto 8053.\n\n"
            "   --doh-host hostname                Establece el valor del header Host.  Por defecto localhost.\n\n"
            "   --doh-path path                    Establece el path del request doh.  por defecto /getnsrecord.\n\n"
            "   --doh-query query                  Establece el query string si el request DoH utiliza el método\n"
            "                                      Doh por defecto ?dns=.\n\n",
            progname);

    exit(1);
}

void parse_args(int argc, char **argv, struct http_args *args) {
    memset(args, 0, sizeof(*args));

    args->http_addr = NULL;
    args->http_port = 8080;

    args->mng_addr = NULL;
    args->mng_port = 9090;

    args->disectors_enabled = true;

    args->doh.host = "localhost";
    args->doh.ip = "127.0.0.1";
    args->doh.port = 8053;
    args->doh.path = "/getnsrecord";
    args->doh.query = "?dns=";

    int c;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            {"doh-ip", required_argument, 0, 0xD001},
            {"doh-port", required_argument, 0, 0xD002},
            {"doh-host", required_argument, 0, 0xD003},
            {"doh-path", required_argument, 0, 0xD004},
            {"doh-query", required_argument, 0, 0xD005},
            {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "hNl:L:o:p:v", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(argv[0]);
                break;
            case 'N':
                args->disectors_enabled = false;
                break;
            case 'l':
                args->http_addr = optarg;
                break;
            case 'L':
                args->mng_addr = optarg;
                break;
            case 'o':
                args->mng_port = port(optarg);
                break;
            case 'p':
                args->http_port = port(optarg);
                break;
            case 'v':
                version();
                exit(0);
                break;
            case 0xD001:
                args->doh.ip = optarg;
                break;
            case 0xD002:
                args->doh.port = port(optarg);
                break;
            case 0xD003:
                args->doh.host = optarg;
                break;
            case 0xD004:
                args->doh.path = optarg;
                break;
            case 0xD005:
                args->doh.query = optarg;
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
