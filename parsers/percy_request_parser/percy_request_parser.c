#include "percy_request_parser.h"

#include <string.h>

#define IS_PERCY_TYPE(x) (x == 0x00 || x == 0x01 || x == 0x02)
#define IS_PERCY_METHOD(x) (x == 0x00 || x == 0x01 || x == 0x03 || x == 0x04 || x == 0x05 || x == 0x06 || x == 0x07)

/** init parser */
void percy_request_parser_init(struct percy_request_parser *p) {
    p->state = percy_request_ver;
    memset(p->request, 0, sizeof(*(p->request)));
}
static enum percy_request_state p_request_ver(struct percy_request_parser *p, const uint8_t c);
static enum percy_request_state p_request_type(struct percy_request_parser *p, const uint8_t c);
static enum percy_request_state p_request_passphrase(struct percy_request_parser *p, const uint8_t c);
static enum percy_request_state p_request_method(struct percy_request_parser *p, const uint8_t c);
static enum percy_request_state p_request_resv(struct percy_request_parser *p, const uint8_t c);
static enum percy_request_state p_request_value(struct percy_request_parser *p, const uint8_t c);

enum percy_request_state
percy_request_parser_feed(struct percy_request_parser *p, const uint8_t c) {
    enum percy_request_state next;

    switch (p->state) {
        case percy_request_ver:
            next = p_request_ver(p, c);
            break;

        case percy_request_type:
            next = p_request_type(p, c);
            break;

        case percy_request_passphrase:
            next = p_request_passphrase(p, c);
            break;

        case percy_request_method:
            next = p_request_method(p, c);
            break;

        case percy_request_resv:
            next = p_request_resv(p, c);
            break;

        case percy_request_value:
            next = p_request_value(p, c);
            break;

        case percy_request_done:
        case percy_request_error:
            next = p->state;
            break;
        default:
            next = percy_request_error;
            break;
    }
    return p->state = next;
}

/*
    +----------+----------+----------+----------+----------+----------+----------+
    |    VER   |      PASSPHRASE     |   TYPE   |  METHOD  |   RESV   |   VALUE  |
    +----------+---------------------+----------+----------+----------+----------+
    |     1    |          6          |    1     |     1    |    1     |    2     |
    +----------+----------+----------+----------+----------+----------+----------+

*/
static enum percy_request_state p_request_ver(struct percy_request_parser *p, const uint8_t c) {
    if (c != 0x01) {
        return percy_request_error;
    }
    p->request->ver = c;
    p->i = 0;
    p->n = PASS_PHRASE_LEN;
    return percy_request_passphrase;
}

static enum percy_request_state p_request_passphrase(struct percy_request_parser *p, const uint8_t c) {
    if (p->i == p->n)
        return percy_request_type;

    p->request->passphrase[p->i++] = c;

    return percy_request_passphrase;
}

static enum percy_request_state p_request_type(struct percy_request_parser *p, const uint8_t c) {
    if (!IS_PERCY_TYPE(c)) {
        return percy_request_error;
    }
    p->request->type = c;
    return percy_request_method;
}

static enum percy_request_state p_request_method(struct percy_request_parser *p, const uint8_t c) {
    if (!IS_PERCY_METHOD(c)) {
        return percy_request_error;
    }
    p->request->method = c;

    return percy_request_resv;
}

static enum percy_request_state p_request_resv(struct percy_request_parser *p, const uint8_t c) {
    p->request->resv = c;
    p->i = 0;
    p->n = VALUE_LEN;
    return percy_request_value;
}

static enum percy_request_state p_request_value(struct percy_request_parser *p, const uint8_t c) {
    percy_request_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->request->value = ((uint16_t)c) << 8;
            next = percy_request_value;
            break;
        case 1:
            p->request->value += (uint16_t)c;
            next = percy_request_done;
            break;
        default:
            next = percy_request_error;
            break;
    }

    return next;
}
