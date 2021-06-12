#ifndef _sniffer_parser_H_
#define _sniffer_parser_H_

#include <netinet/in.h>
#include <stdbool.h>

#include "utils/parser/parser_utils.h"

enum {

    /**  max length for HTTP field */
    MAX_AUTH_VALUE_LENGTH = 1 << 10,
    MAX_USER_LENGTH = 40,
    MAX_PASSWORD_LENGTH = 40,

    CLIENT_OWNED = 0,
    ORIGIN_OWNED = 1,
};

typedef enum sniffer_state {
    sniff_start,

    sniff_http_version,
    sniff_http_authorization,
    sniff_http_authorization_basic,
    sniff_http_authorization_value,

    sniff_pop3_origin_ack,
    sniff_pop3_user_username,
    sniff_pop3_user_username_value,
    sniff_pop3_origin_user_ack,
    sniff_pop3_user_pass,
    sniff_pop3_user_pass_value,
    sniff_pop3_origin_pass_ack,

    sniff_done,

    sniff_error,
} sniffer_state;

struct sniffer_parser {
    sniffer_state state;
    
    uint8_t user[MAX_USER_LENGTH + 1];
    uint8_t password[MAX_PASSWORD_LENGTH + 1];
    uint8_t auth_value[MAX_AUTH_VALUE_LENGTH + 1];

    enum protocol{
        POP3,
        HTTP,
    }sniffedProtocol;

    struct parser *stringParser;
    bool parserIsSet;

    int i;
};

/** init parser */
void sniffer_parser_init(struct sniffer_parser *p);

/** returns true if done */
enum sniffer_state
sniffer_parser_feed(uint8_t c, struct sniffer_parser *p, int dataOwner, uint16_t port);

void
modify_sniffer_state(struct sniffer_parser *p, enum sniffer_state newState);



#endif
