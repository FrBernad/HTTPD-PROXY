#include "percy_response_parser.h"
#include <stdint.h>
#include <string.h>

static enum percy_response_state p_response_ver(struct percy_response_parser *p, const uint8_t c);
static enum percy_response_state p_response_status(struct percy_response_parser *p, const uint8_t c);
static enum percy_response_state p_response_resv(struct percy_response_parser *p, const uint8_t c);
static enum percy_response_state p_response_value(struct percy_response_parser *p, const uint8_t c);

/** init parser */
void percy_response_parser_init(struct percy_response_parser *p) {
    p->state = percy_response_ver;
    memset(p->response, 0, sizeof(*(p->response)));
}

enum percy_response_state
percy_response_parser_feed(struct percy_response_parser *p, const uint8_t c) {
    enum percy_response_state next;

    switch (p->state) {
        case percy_response_ver:
            next = p_response_ver(p, c);
            break;
        case percy_response_status:
            next = p_response_status(p, c);
            break;
        case percy_response_resv:
            next = p_response_resv(p, c);
            break;
        case percy_response_value:
            next = p_response_value(p, c);
            break;
        case percy_response_done:
        case percy_response_error:
            next = p->state;
            break;
        default:
            next = percy_response_error;
            break;
    }
    return p->state = next;
}

static enum percy_response_state p_response_ver(struct percy_response_parser *p, const uint8_t c) {
    p->response->ver = c;
    return percy_response_status;
}

static enum percy_response_state p_response_status(struct percy_response_parser *p, const uint8_t c) {
    p->response->status = c;
    return percy_response_resv;
}
static enum percy_response_state p_response_resv(struct percy_response_parser *p, const uint8_t c) {
    p->response->resv = c;
    p->i = 0;
    p->n = VALUE_LEN;
    return percy_response_value;
}
static enum percy_response_state p_response_value(struct percy_response_parser *p, const uint8_t c) {
    enum percy_response_state next_state = percy_response_value;

    if (p->i == p->n)
        return percy_response_done;

    switch (p->i) {
        case 0:
            p->response->value = ((uint64_t)c) << 56 ;
            break;
        case 1:
            p->response->value += ((uint64_t)c) << 48;
            break;
        case 2:
            p->response->value += ((uint64_t)c) << 40;
            break;
        case 3:
            p->response->value += ((uint64_t)c) << 32;
            break;
        case 4:
            p->response->value += ((uint64_t)c) << 24;
            break;
        case 5:
            p->response->value += ((uint64_t)c) << 16;
            break;
        case 6:
            p->response->value += ((uint64_t)c) << 8;
            break;
        case 7:
            p->response->value += ((uint64_t)c);
            break;
        default:
            break;
            //never reaches
    }
    p->i++;
    return next_state;
}