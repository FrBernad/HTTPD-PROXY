#include "sniffer_parser.h"

#include <stdlib.h>
#include <string.h>

#include "utils/base64/base64.h"

static unsigned 
decode_and_parse_auth(struct sniffer_parser *p);

static int 
parse_user_and_password(char *authorization, int len, struct headers_parser *p);

void sniffer_parser_init(struct sniffer_parser *p) {
    p->authorization.user = NULL;
    p->authorization.password = NULL;
    p->stringParser = parser_utils_strcmpi("AUTHORIZATION:");
    memset(p->auth_value, 0, MAX_HEADER_FIELD_VALUE_LENGTH);
    p->i = 0;
}

enum sniffer_state
sniffer_parser_feed(struct sniffer_parser *p, const uint8_t c) {
    enum sniffer_state next;

    switch (p->state) {
        case http_authorization:
            next = s_http_authorization(c, p);
            break;
        case http_authorization_basic:
            next = s_http_authorization_basic(c, p);
            break;
        case http_authorization_value:
            next = s_http_authorization_value(c, p);
            break;

        case done:
        case error:
            next = p->state;
            break;
        default:
            next = error;
            break;
    }

    return p->state = next;
}

unsigned
s_http_authorization(struct sniffer_parser *p, const uint8_t c) {
    struct parser_definition *stringParser = &p->stringParser;

    unsigned state = parser_feed(stringParser, c);

    if (state == STRING_CMP_EQ) {
        parser_utils_strcmpi_destroy(stringParser);
        p->stringParser = parser_utils_strcmpi("BASIC");
        return http_authorization_basic;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return http_authorization;
}

unsigned
s_http_authorization_basic(struct sniffer_parser *p, const uint8_t c) {
    struct parser_definition *stringParser = &p->stringParser;

    unsigned state = parser_feed(stringParser, c);

    if (state == STRING_CMP_EQ) {
        parser_utils_strcmpi_destroy(stringParser);
        return http_authorization_value;
    }

    if (state == STRING_CMP_NEQ) {
        parser_utils_strcmpi_destroy(stringParser);
        p->stringParser = parser_utils_strcmpi("AUTHORIZATION:");
        p->i = 0;
        p->i = MAX_HEADER_FIELD_VALUE_LENGTH;
        return http_authorization;
    }

    return http_authorization_basic;
}

unsigned
s_http_authorization_value(struct sniffer_parser *p, const uint8_t c) {
    if (p->i >= p->n) {
        return error;
    }

    if (IS_SPACE(c)) {
        return http_authorization_value;
    }

    if (c == '\r') {
        p->auth_value[p->i] = 0;
        return decode_and_parse_auth(sniffer_parser);
    }

    if (IS_BASE_64(x)) {
        p->auth_value[p->i++] = c;
        return http_authorization_value;
    }

    return error;
}

static unsigned 
decode_and_parse_auth(struct sniffer_parser *p) {
    int len;
    char *authorization = base64_decode(p->auth_value, p->i, (size_t*)&len);
    if (authorization == NULL) {
        return;
    }

    int ret = parse_user_and_password(authorization, len, p);
    free(authorization);
    if(ret < 0 ){
        return error;
    }
    return done;
}

static int 
parse_user_and_password(char *authorization, int len, struct sniffer_parser *p) {
    int i = 0;
    while (i < len && authorization[i] != ':') {
        i++;
    }

    if (i < len) {
        p->authorization.user = malloc(i + 1);
        if(p->authorization.user == NULL){
            return -1;
        }
        memcpy(p->authorization.user, authorization, i);
        p->authorization.user[i] = 0;
    }
    i++;

    p->authorization.password = malloc(len - i + 1);
    if(p->authorization.password == NULL){
        free(p->authorization.user);
        p->authorization.user = NULL;
        return -1;
    }
    memcpy(p->authorization.password, authorization + i, len - i);
    p->authorization.password[len - i] = 0;
}