#include "headers_parser.h"

#include "../utils/parser_utils.h"

static enum headers_state h_field_name(const uint8_t c, struct headers_parser *p);
static enum headers_state h_field_value(const uint8_t c, struct headers_parser *p);
static enum headers_state h_end(const uint8_t c, struct headers_parser *p);
static enum headers_state h_may_be_end(const uint8_t c, struct headers_parser *p);
static enum headers_state h_field_value_start_ows(const uint8_t c, struct headers_parser *p);
static enum headers_state h_field_value_end_ows(const uint8_t c, struct headers_parser *p);
static enum headers_state h_field_value_end(const uint8_t c, struct headers_parser *p);

void headers_parser_init(struct headers_parser *p) {
    p->state = headers_field_name;
    p->headersCount = 0;
    p->i = 0;
    p->n = MAX_HEADER_FIELD_NAME_LENGTH;
}

enum headers_state
headers_parser_feed(struct headers_parser *p, const uint8_t c) {
    enum headers_state next;

    switch (p->state) {
        case headers_field_name:
            next = h_field_name(c,p);
            break;
        case headers_field_value_start_ows:
            next = h_field_value_start_ows(c,p);
            break;
        case headers_field_value:
            next = h_field_value(c,p);
            break;
        case headers_field_value_end_ows:
            next = h_field_value_end_ows(c,p);
            break;
        case headers_field_value_end:
            next = h_field_value_end(c,p);
            break;
        case headers_may_be_end:
            next = h_may_be_end(c,p);
            break;
        case headers_end:
            next = h_end(c,p);
            break;

        //Done
        case headers_done:
        case headers_error:
            next = p->state;
            break;
        default:
            next = headers_error;
            break;
    }

    return p->state = next;
}

static enum headers_state h_field_name(const uint8_t c, struct headers_parser *p) {
    if (p->i >= p->n){
        return headers_error;
    }

    if (c == ':') {
        p->headersCount++;
        p->i = 0;
        p->n = MAX_HEADER_FIELD_VALUE_LENGTH;
        return headers_field_value_start_ows;
    }

    if (!IS_TOKEN(c)) {
        return headers_error;
    }

    p->i++;

    return headers_field_name;
}

static enum headers_state h_field_value_start_ows(const uint8_t c, struct headers_parser *p) {
    if (p->i >= p->n) {
        return headers_error;
    }

    if (IS_SPACE(c)) {
        p->i++;
        return headers_field_value_start_ows;
    }

    if (c == '\r') {
        return headers_field_value_end;
    }

    if (IS_TOKEN(c)) {
        p->i++;
        return headers_field_value;
    }

    return headers_error;
}

static enum headers_state h_field_value(const uint8_t c, struct headers_parser *p) {
    if (p->i >= p->n)
        return headers_error;

    if (c == '\r') {
        return headers_field_value_end;
    }

    if (IS_SPACE(c)) {
        p->i++;
        return headers_field_value_end_ows;
    }

    if (IS_TOKEN(c)) {
        p->i++;
        return headers_field_value;
    }

    return headers_error;
}

static enum headers_state h_field_value_end_ows(const uint8_t c, struct headers_parser *p) {
    if (p->i >= p->n) {
        return headers_error;
    }

    if (IS_SPACE(c)) {
        p->i++;
        return headers_field_value_end_ows;
    }

    if (c == '\r') {
        return headers_field_value_end;
    }

    return headers_error;
}

static enum headers_state h_field_value_end(const uint8_t c, struct headers_parser *p) {
    if (c != '\n') {
        return headers_error;
    }
    p->headersCount++;
    return headers_may_be_end;
}

static enum headers_state h_may_be_end(const uint8_t c, struct headers_parser *p) {
    if (c == '\r') {
        return headers_end;
    }

    if (!IS_TOKEN(c)) {
        return headers_error;
    }

    p->i = 0;
    p->n = MAX_HEADER_FIELD_NAME_LENGTH;
    //FIXME: si vamos a guardar algun header habria que agregar esta letra
    return headers_field_name;
}

static enum headers_state h_end(const uint8_t c, struct headers_parser *p) {
    if (c != '\n') {
        return headers_error;
    }

    return headers_done;
}
