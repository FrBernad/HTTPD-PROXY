#include "sniffer_parser.h"

#include <stdlib.h>
#include <string.h>

#include "utils/base64/base64.h"

// SNIFFER INTI
static unsigned
s_start(const uint8_t c, struct sniffer_parser *p, int dataOwner);

// HTTP SNIFFING
static unsigned
s_http_version(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_http_authorization(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_http_authorization_basic(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_http_authorization_value(const uint8_t c, struct sniffer_parser *p, int dataOwner);

// HTTP SNIFFING UTILS
static unsigned
decode_and_parse_auth(struct sniffer_parser *p);

static void
parse_user_and_password(uint8_t *authorization, int len, struct sniffer_parser *p);

static void
set_string_parser(struct sniffer_parser *p, char *string);

// POP3 SNIFFING

static unsigned
s_pop3_origin_ack(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_pop3_user_username(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_pop3_user_username_value(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_pop3_origin_user_ack(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_pop3_user_pass(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_pop3_user_pass_value(const uint8_t c, struct sniffer_parser *p, int dataOwner);

static unsigned
s_pop3_origin_pass_ack(const uint8_t c, struct sniffer_parser *p, int dataOwner);

void sniffer_parser_init(struct sniffer_parser *p) {
    p->parserIsSet = false;
    p->state = sniff_start;
    memset(p->auth_value, 0, MAX_AUTH_VALUE_LENGTH + 1);
    memset(p->user, 0, MAX_USER_LENGTH + 1);
    memset(p->password, 0, MAX_PASSWORD_LENGTH + 1);
    p->i = 0;
}

void modify_sniffer_state(struct sniffer_parser *p, enum sniffer_state newState) {
    p->state = newState;
    set_string_parser(p, "AUTHORIZATION:");
}

enum sniffer_state
sniffer_parser_feed(uint8_t c, struct sniffer_parser *p, int dataOwner) {
    enum sniffer_state next;

    switch (p->state) {
        case sniff_start:
            next = s_start(c, p, dataOwner);
            break;

        case sniff_http_version:
            next = s_http_version(c, p, dataOwner);
            break;
        case sniff_http_authorization:
            next = s_http_authorization(c, p, dataOwner);
            break;
        case sniff_http_authorization_basic:
            next = s_http_authorization_basic(c, p, dataOwner);
            break;
        case sniff_http_authorization_value:
            next = s_http_authorization_value(c, p, dataOwner);
            break;

        case sniff_pop3_origin_ack:
            next = s_pop3_origin_ack(c, p, dataOwner);
            break;
        case sniff_pop3_user_username:
            next = s_pop3_user_username(c, p, dataOwner);
            break;
        case sniff_pop3_user_username_value:
            next = s_pop3_user_username_value(c, p, dataOwner);
            break;
        case sniff_pop3_origin_user_ack:
            next = s_pop3_origin_user_ack(c, p, dataOwner);
            break;
        case sniff_pop3_user_pass:
            next = s_pop3_user_pass(c, p, dataOwner);
            break;
        case sniff_pop3_user_pass_value:
            next = s_pop3_user_pass_value(c, p, dataOwner);
            break;
        case sniff_pop3_origin_pass_ack:
            next = s_pop3_origin_pass_ack(c, p, dataOwner);
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
s_start(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        set_string_parser(p, "OK+");
        return sniff_pop3_origin_ack;
    }

    set_string_parser(p, "HTTP/1");
    return sniff_http_version;
}

static unsigned
s_http_version(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

    if (c == '\r') {
        return sniff_error;
    }

    struct parser *stringParser = p->stringParser;
    unsigned state = parser_feed(stringParser, c)->type;

    if (state == STRING_CMP_EQ) {
        set_string_parser(p, "AUTHORIZATION:");
        return sniff_http_authorization;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_http_version;
}

static unsigned
s_http_authorization(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

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
s_http_authorization_basic(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

    if (c == '\r') {
        return sniff_error;
    }

    struct parser *stringParser = p->stringParser;
    unsigned state = parser_feed(stringParser, c)->type;

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        return sniff_http_authorization_value;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_http_authorization_basic;
}

static unsigned
s_http_authorization_value(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

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
s_pop3_origin_ack(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == CLIENT_OWNED) {
        return sniff_error;
    }

    struct parser *stringParser = p->stringParser;

    unsigned state = parser_feed(stringParser, c)->type;

    if (state == STRING_CMP_EQ) {
        set_string_parser(p, "USER");
        return sniff_pop3_user_username;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_pop3_origin_ack;
}

static unsigned
s_pop3_user_username(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

    struct parser *stringParser = p->stringParser;

    unsigned state = parser_feed(stringParser, c)->type;

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        return sniff_pop3_user_username_value;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_pop3_user_username;
}

static unsigned
s_pop3_user_username_value(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

    if (p->i >= MAX_USER_LENGTH) {
        return sniff_error;
    }

    if (IS_SPACE(c)) {
        return sniff_pop3_user_username_value;
    }

    if (c == '\r') {
        p->user[p->i] = 0;
        set_string_parser(p, "OK+");
        return sniff_pop3_origin_user_ack;
    }

    if (IS_VCHAR(c)) {
        p->user[p->i++] = c;
        return sniff_pop3_user_username_value;
    }

    return sniff_error;
}

static unsigned
s_pop3_origin_user_ack(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == CLIENT_OWNED) {
        return sniff_error;
    }

    struct parser *stringParser = p->stringParser;

    unsigned state = parser_feed(stringParser, c)->type;

    if (c == '-') {
        return sniff_error;
    }

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        set_string_parser(p, "PASS");
        return sniff_pop3_user_pass;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_pop3_origin_user_ack;
}

static unsigned
s_pop3_user_pass(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

    struct parser *stringParser = p->stringParser;

    unsigned state = parser_feed(stringParser, c)->type;

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        return sniff_pop3_user_pass_value;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_pop3_user_pass;
}

static unsigned
s_pop3_user_pass_value(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == ORIGIN_OWNED) {
        return sniff_error;
    }

    if (p->i >= MAX_PASSWORD_LENGTH) {
        return sniff_error;
    }

    if (IS_SPACE(c)) {
        return sniff_pop3_user_pass_value;
    }

    if (c == '\r') {
        p->password[p->i] = 0;
        set_string_parser(p, "OK+");
        return sniff_pop3_origin_pass_ack;
    }

    if (IS_VCHAR(c)) {
        p->password[p->i++] = c;
        return sniff_pop3_user_pass_value;
    }

    return sniff_error;
}

static unsigned
s_pop3_origin_pass_ack(const uint8_t c, struct sniffer_parser *p, int dataOwner) {
    if (dataOwner == CLIENT_OWNED) {
        return sniff_error;
    }

    struct parser *stringParser = p->stringParser;

    unsigned state = parser_feed(stringParser, c)->type;

    if (c == '-') {
        return sniff_error;
    }

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        p->sniffedProtocol = POP3;
        return sniff_done;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(stringParser);
    }

    return sniff_pop3_origin_pass_ack;
}

static unsigned
decode_and_parse_auth(struct sniffer_parser *p) {
    size_t len;
    uint8_t *authorization = base64_decode(p->auth_value, (size_t)p->i, &len);
    if (authorization == NULL) {
        return sniff_error;
    }

    parse_user_and_password(authorization, len, p);
    free(authorization);
    p->sniffedProtocol = HTTP;

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