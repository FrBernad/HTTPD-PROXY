#ifndef _sniffer_parser_H_
#define _sniffer_parser_H_

#include <netinet/in.h>
#include <stdbool.h>

#include "utils/parser/parser_utils.h"

enum {
 
    /**  max length for HTTP field */
    MAX_AUTH_VALUE_LENGTH = 1 << 10,
    MAX_USER_LENGTH = 50,
    MAX_PASSWORD_LENGTH = 50,
};

typedef enum sniffer_state {
    sniff_http_authorization,
    sniff_http_authorization_basic,
    sniff_http_authorization_value,

    sniff_done,

    sniff_error,
} sniffer_state;

struct sniffer_parser {
    sniffer_state state;
    uint8_t user[MAX_USER_LENGTH+1];
    uint8_t password[MAX_PASSWORD_LENGTH+1];
    uint8_t auth_value[MAX_AUTH_VALUE_LENGTH+1];
    struct parser *stringParser;
    bool parserIsSet;
    int i;
};

/** init parser */
void 
sniffer_parser_init(struct sniffer_parser *p);

/** returns true if done */
enum sniffer_state
sniffer_parser_feed(const uint8_t c, struct sniffer_parser *p);

#endif
