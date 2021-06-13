#include "headers_parser.h"

#include "utils/parser/parser_utils.h"

static enum headers_state
h_field_name(uint8_t c, headers_parser_t *p);

static enum headers_state
h_field_value(uint8_t c, headers_parser_t *p);

static enum headers_state
h_end(uint8_t c, headers_parser_t *p);

static enum headers_state
h_may_be_end(uint8_t c, headers_parser_t *p);

static enum headers_state
h_field_value_end(uint8_t c, headers_parser_t *p);

void
headers_parser_init(headers_parser_t *p) {
    p->state = headers_field_name;
    p->headers_count = 0;
    p->i = 0;
    p->n = MAX_HEADER_FIELD_NAME_LENGTH;
}

enum headers_state
headers_parser_feed(headers_parser_t *p, uint8_t c) {
    enum headers_state next;

    switch (p->state) {
        case headers_field_name:
            next = h_field_name(c, p);
            break;
        case headers_field_value:
            next = h_field_value(c, p);
            break;
        case headers_field_value_end:
            next = h_field_value_end(c, p);
            break;
        case headers_may_be_end:
            next = h_may_be_end(c, p);
            break;
        case headers_end:
            next = h_end(c, p);
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

static enum headers_state
h_field_name(uint8_t c, headers_parser_t *p) {
    if (p->i >= p->n) {
        return headers_error;
    }

    if (c == ':') {
        p->headers_count++;
        p->i = 0;
        p->n = MAX_HEADER_FIELD_VALUE_LENGTH;
        return headers_field_value;
    }

    if (!IS_TOKEN(c)) {
        return headers_error;
    }

    return headers_field_name;
}

static enum headers_state
h_field_value(uint8_t c, headers_parser_t *p) {
    if (p->i >= p->n)
        return headers_error;

    if (c == '\r') {
        return headers_field_value_end;
    }

    if (IS_VCHAR(c) || IS_SPACE(c)) {
        return headers_field_value;
    }

    return headers_error;
}

static enum headers_state
h_field_value_end(uint8_t c, headers_parser_t *p) {
    if (c != '\n') {
        return headers_error;
    }

    return headers_may_be_end;
}

static enum headers_state
h_may_be_end(uint8_t c, headers_parser_t *p) {
    if (c == '\r') {
        return headers_end;
    }

    if (!IS_TOKEN(c)) {
        return headers_error;
    }
    p->i = 0;
    p->n = MAX_HEADER_FIELD_NAME_LENGTH;

    return headers_field_name;
}

static enum headers_state
h_end(uint8_t c, headers_parser_t *p) {
    if (c != '\n') {
        return headers_error;
    }

    return headers_done;
}