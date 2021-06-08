#include "sniffer_parser.h"

#include <stdlib.h>
#include <string.h>

#include "utils/base64/base64.h"

static unsigned
s_http_authorization(const uint8_t c, struct sniffer_parser *p);

static unsigned
s_http_authorization_basic(const uint8_t c, struct sniffer_parser *p);

static unsigned
s_http_authorization_value(const uint8_t c, struct sniffer_parser *p);

static unsigned
decode_and_parse_auth(struct sniffer_parser *p);

static void
parse_user_and_password(uint8_t *authorization, int len, struct sniffer_parser *p);

static void
set_string_parser(struct sniffer_parser *p, char *string);

void sniffer_parser_init(struct sniffer_parser *p) {
    p->parserIsSet = false;
    set_string_parser(p, "AUTHORIZATION:");
    memset(p->auth_value, 0, MAX_AUTH_VALUE_LENGTH);
    p->i = 0;
}

enum sniffer_state
sniffer_parser_feed(const uint8_t c, struct sniffer_parser *p) {
    enum sniffer_state next;

    switch (p->state) {
        case sniff_http_authorization:
            next = s_http_authorization(c, p);
            break;
        case sniff_http_authorization_basic:
            next = s_http_authorization_basic(c, p);
            break;
        case sniff_http_authorization_value:
            next = s_http_authorization_value(c, p);
            break;

        case sniff_done:
        case sniff_error:
            next = p->state;
            break;
        default:
            next = sniff_error;
            break;
    }

    return p->state = next;
}

static unsigned
s_http_authorization(const uint8_t c, struct sniffer_parser *p) {
    struct parser *stringParser = p->stringParser;

    unsigned state = parser_feed(stringParser, c)->type;

    if (state == STRING_CMP_EQ) {
        set_string_parser(p, "BASIC");
        return sniff_http_authorization_basic;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_http_authorization;
}

static unsigned
s_http_authorization_basic(const uint8_t c, struct sniffer_parser *p) {
    struct parser *stringParser = p->stringParser;

    unsigned state = parser_feed(stringParser, c)->type;

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        return sniff_http_authorization_value;
    }

    if (c == '\r') {
        set_string_parser(p, "AUTHORIZATION:");
        return sniff_http_authorization;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_http_authorization_basic;
}

static unsigned
s_http_authorization_value(const uint8_t c, struct sniffer_parser *p) {
    if (p->i >= MAX_AUTH_VALUE_LENGTH) {
        return sniff_error;
    }

    if (IS_SPACE(c)) {
        return sniff_http_authorization_value;
    }

    if (c == '\r') {
        p->auth_value[p->i] = 0;
        return decode_and_parse_auth(p);
    }

    if (IS_BASE_64(c)) {
        p->auth_value[p->i++] = c;
        return sniff_http_authorization_value;
    }

    return sniff_error;
}

static unsigned
decode_and_parse_auth(struct sniffer_parser *p) {
    size_t len;
    uint8_t *authorization = base64_decode(p->auth_value, (size_t) p->i, &len);
    if (authorization == NULL) {
        return sniff_error;
    }

    parse_user_and_password(authorization, len, p);
    free(authorization);

    return sniff_done;
}

static void
parse_user_and_password(uint8_t *authorization, int len, struct sniffer_parser *p) {
    int i = 0;
    while (i < len && authorization[i] != ':') {
        i++;
    }
    int size = i > MAX_USER_LENGTH ? MAX_USER_LENGTH : i;
    if (i < len) {
        memcpy(p->user, authorization, size);
        p->user[size] = 0;
    }
    i++;

    size = len - i > MAX_PASSWORD_LENGTH ? MAX_PASSWORD_LENGTH : len - i;

    memcpy(p->password, authorization + i, size);
    p->password[size] = 0;
}

static void
set_string_parser(struct sniffer_parser *p, char *string) {
    struct parser_definition parserDef;
    parser_utils_strcmpi(string, &parserDef);

    if (p->parserIsSet) {
        parser_destroy(p->stringParser);
    }

    p->parserIsSet = true;
    p->stringParser = parser_init(parser_no_classes(), parserDef);
}