#ifndef _sniffer_parser_H_
#define _sniffer_parser_H_

#include <netinet/in.h>

#include "utils/parser/parser_utils.h"

enum {
 
    /**  max length for HTTP field */
    MAX_HEADER_FIELD_VALUE_LENGTH = 1 << 10,
};

typedef enum sniffer_state {
    
    http_authorization,
    http_authorization_basic,
    http_authorization_value,

    done,

    error,
} sniffer_state;

struct sniffer_parser {
    sniffer_state state;
    char * user;
    char * password;
    char auth_value[MAX_HEADER_FIELD_VALUE_LENGTH];
    struct parser_definition stringParser;
    int i;
    int n;
};

/** init parser */
void 
sniffer_parser_init(struct sniffer_parser *p);

/** returns true if done */
enum sniffer_state
sniffer_parser_feed(struct sniffer_parser *p, const uint8_t c);

#endif
