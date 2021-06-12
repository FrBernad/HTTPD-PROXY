#include "sniffer_parser.h"

#include <stdlib.h>
#include <string.h>

#include "utils/base64/base64.h"

// SNIFFER INTI
static unsigned
s_start(uint8_t c, sniffer_parser_t *p, int data_owner, uint16_t port);

// HTTP SNIFFING
static unsigned
s_http_version(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_http_authorization(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_http_authorization_basic(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_http_authorization_value(uint8_t c, sniffer_parser_t *p, int data_owner);

// HTTP SNIFFING UTILS
static unsigned
decode_and_parse_auth(sniffer_parser_t *p);

static void
parse_user_and_password(uint8_t *authorization, size_t len, sniffer_parser_t *p);

static void
set_string_parser(sniffer_parser_t *p, char *string);

// POP3 SNIFFING

static unsigned
s_pop3_origin_ack(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_pop3_user_username(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_pop3_user_username_value(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_pop3_origin_user_ack(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_pop3_user_pass(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_pop3_user_pass_value(uint8_t c, sniffer_parser_t *p, int data_owner);

static unsigned
s_pop3_origin_pass_ack(uint8_t c, sniffer_parser_t *p, int data_owner);

void sniffer_parser_init(sniffer_parser_t *p) {
    p->parser_is_set = false;
    p->state = sniff_start;
    memset(p->auth_value, 0, MAX_AUTH_VALUE_LENGTH + 1);
    memset(p->user, 0, MAX_USER_LENGTH + 1);
    memset(p->password, 0, MAX_PASSWORD_LENGTH + 1);
    p->i = 0;
}

void modify_sniffer_state(sniffer_parser_t *p, enum sniffer_state new_state) {
    p->state = new_state;
    set_string_parser(p, "AUTHORIZATION:");
}

enum sniffer_state
sniffer_parser_feed(uint8_t c, sniffer_parser_t *p, int data_owner, uint16_t port) {
    enum sniffer_state next;

    switch (p->state) {
        case sniff_start:
            next = s_start(c, p, data_owner, port);
            break;

        case sniff_http_version:
            next = s_http_version(c, p, data_owner);
            break;
        case sniff_http_authorization:
            next = s_http_authorization(c, p, data_owner);
            break;
        case sniff_http_authorization_basic:
            next = s_http_authorization_basic(c, p, data_owner);
            break;
        case sniff_http_authorization_value:
            next = s_http_authorization_value(c, p, data_owner);
            break;

        case sniff_pop3_origin_ack:
            next = s_pop3_origin_ack(c, p, data_owner);
            break;
        case sniff_pop3_user_username:
            next = s_pop3_user_username(c, p, data_owner);
            break;
        case sniff_pop3_user_username_value:
            next = s_pop3_user_username_value(c, p, data_owner);
            break;
        case sniff_pop3_origin_user_ack:
            next = s_pop3_origin_user_ack(c, p, data_owner);
            break;
        case sniff_pop3_user_pass:
            next = s_pop3_user_pass(c, p, data_owner);
            break;
        case sniff_pop3_user_pass_value:
            next = s_pop3_user_pass_value(c, p, data_owner);
            break;
        case sniff_pop3_origin_pass_ack:
            next = s_pop3_origin_pass_ack(c, p, data_owner);
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
s_start(uint8_t c, sniffer_parser_t *p, int data_owner, uint16_t port) {
    if (port == 110) {
        set_string_parser(p, "+OK");
        return s_pop3_origin_ack(c, p, data_owner);
    } else if (port == 80) {
        set_string_parser(p, "HTTP/1");
        return s_http_version(c, p, data_owner);
    }
    return sniff_start;
}

static unsigned
s_http_version(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
        return sniff_error;
    }

    if (c == '\r') {
        return sniff_error;
    }

    struct parser *string_parser = p->string_parser;
    unsigned state = parser_feed(string_parser, c)->type;

    if (state == STRING_CMP_EQ) {
        set_string_parser(p, "AUTHORIZATION:");
        return sniff_http_authorization;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_http_version;
}

static unsigned
s_http_authorization(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
        return sniff_error;
    }

    struct parser *string_parser = p->string_parser;
    unsigned state = parser_feed(string_parser, c)->type;

    if (state == STRING_CMP_EQ) {
        set_string_parser(p, "BASIC");
        return sniff_http_authorization_basic;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_http_authorization;
}

static unsigned
s_http_authorization_basic(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
        return sniff_error;
    }

    if (c == '\r') {
        return sniff_error;
    }

    struct parser *string_parser = p->string_parser;
    unsigned state = parser_feed(string_parser, c)->type;

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        return sniff_http_authorization_value;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_http_authorization_basic;
}

static unsigned
s_http_authorization_value(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
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
s_pop3_origin_ack(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == CLIENT_OWNED) {
        return sniff_error;
    }

    struct parser *string_parser = p->string_parser;

    unsigned state = parser_feed(string_parser, c)->type;

    if (state == STRING_CMP_EQ) {
        set_string_parser(p, "USER");
        return sniff_pop3_user_username;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_pop3_origin_ack;
}

static unsigned
s_pop3_user_username(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
        return sniff_pop3_user_username;
    }

    struct parser *string_parser = p->string_parser;

    unsigned state = parser_feed(string_parser, c)->type;

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        return sniff_pop3_user_username_value;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_pop3_user_username;
}

static unsigned
s_pop3_user_username_value(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
        return sniff_pop3_user_username_value;
    }

    if (p->i >= MAX_USER_LENGTH) {
        return sniff_error;
    }

    if (IS_SPACE(c) || c == '\r') {
        return sniff_pop3_user_username_value;
    }

    if (c == '\n') {
        p->user[p->i] = 0;
        set_string_parser(p, "+OK");
        return sniff_pop3_origin_user_ack;
    }

    if (IS_VCHAR(c)) {
        p->user[p->i++] = c;
        return sniff_pop3_user_username_value;
    }

    return sniff_error;
}

static unsigned
s_pop3_origin_user_ack(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == CLIENT_OWNED) {
        return sniff_pop3_origin_user_ack;
    }

    struct parser *string_parser = p->string_parser;

    unsigned state = parser_feed(string_parser, c)->type;

    if (c == '-') {
        return sniff_error;
    }

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        set_string_parser(p, "PASS");
        return sniff_pop3_user_pass;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_pop3_origin_user_ack;
}

static unsigned
s_pop3_user_pass(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
        return sniff_pop3_user_pass;
    }

    struct parser *string_parser = p->string_parser;

    unsigned state = parser_feed(string_parser, c)->type;

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        return sniff_pop3_user_pass_value;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_pop3_user_pass;
}

static unsigned
s_pop3_user_pass_value(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == ORIGIN_OWNED) {
        return sniff_error;
    }

    if (p->i >= MAX_PASSWORD_LENGTH) {
        return sniff_error;
    }

    if (IS_SPACE(c) || c == '\r') {
        return sniff_pop3_user_pass_value;
    }

    if (c == '\n') {
        p->password[p->i] = 0;
        set_string_parser(p, "+OK");
        return sniff_pop3_origin_pass_ack;
    }

    if (IS_VCHAR(c)) {
        p->password[p->i++] = c;
        return sniff_pop3_user_pass_value;
    }

    return sniff_error;
}

static unsigned
s_pop3_origin_pass_ack(uint8_t c, sniffer_parser_t *p, int data_owner) {
    if (data_owner == CLIENT_OWNED) {
        return sniff_pop3_origin_pass_ack;
    }

    struct parser *string_parser = p->string_parser;

    unsigned state = parser_feed(string_parser, c)->type;

    if (c == '-') {
        return sniff_error;
    }

    if (state == STRING_CMP_EQ) {
        p->i = 0;
        p->sniffed_protocol = POP3;
        return sniff_done;
    }

    if (state == STRING_CMP_NEQ) {
        parser_reset(string_parser);
    }

    return sniff_pop3_origin_pass_ack;
}

static unsigned
decode_and_parse_auth(sniffer_parser_t *p) {
    size_t len;
    uint8_t *authorization = base64_decode(p->auth_value, (size_t) p->i, &len);
    if (authorization == NULL) {
        return sniff_error;
    }

    parse_user_and_password(authorization, len, p);
    free(authorization);
    p->sniffed_protocol = HTTP;

    return sniff_done;
}

static void
parse_user_and_password(uint8_t *authorization, size_t len, sniffer_parser_t *p) {
    size_t i = 0;
    while (i < len && authorization[i] != ':') {
        i++;
    }
    size_t size = i > MAX_USER_LENGTH ? MAX_USER_LENGTH : i;
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
set_string_parser(sniffer_parser_t *p, char *string) {
    struct parser_definition parser_def;
    parser_utils_strcmpi(string, &parser_def);

    if (p->parser_is_set) {
        parser_destroy(p->string_parser);
    }

    p->parser_is_set = true;
    p->string_parser = parser_init(parser_no_classes(), parser_def);
}