#include "status_line_parser.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>

#include "utils/parser/parser_utils.h"

static enum status_line_state
sl_version(const uint8_t c, struct status_line_parser *p);

static enum status_line_state
sl_version_major(const uint8_t c, struct status_line_parser *p);

static enum status_line_state
sl_version_minor(const uint8_t c, struct status_line_parser *p);

static enum status_line_state
sl_status_code(const uint8_t c, struct status_line_parser *p);

static enum status_line_state
sl_reason_phrase(const uint8_t c, struct status_line_parser *p);

static enum status_line_state
sl_end(const uint8_t c, struct status_line_parser *p);

void status_line_parser_init(struct status_line_parser *p) {
    p->state = status_line_version;
    p->i = 0;
    p->n = STATUS_VERSION_LENGTH;
    memset(p->status_line, 0, sizeof(*(p->status_line)));
}

enum status_line_state status_line_parser_feed(struct status_line_parser *p, const uint8_t c) {
    enum status_line_state next;

    switch (p->state) {
        case status_line_version:
            next = sl_version(c, p);
            break;
        case status_line_version_major:
            next = sl_version_major(c, p);
            break;
        case status_line_version_minor:
            next = sl_version_minor(c, p);
            break;
        case status_line_status_code:
            next = sl_status_code(c, p);
            break;
        case status_line_reason_phrase:
            next = sl_reason_phrase(c, p);
            break;
        case status_line_end:
            next = sl_end(c, p);
            break;
        case status_line_done:
        case status_line_error:
        case status_line_error_unsupported_version:
            next = p->state;
            break;
        default:
            next = status_line_error;
            break;
    }

    return p->state = next;
}

static enum status_line_state
sl_version(const uint8_t c, struct status_line_parser *p) {
    char *version = HTTP;

    if (p->i >= p->n || version[p->i] != c)
        return status_line_error;

    p->i++;

    if (c == '/') {
        p->i = 0;
        p->n = 2;  //FIXME: CREAR ENUM
        return status_line_version_major;
    }

    return status_line_version;
}

static enum status_line_state
sl_version_major(const uint8_t c, struct status_line_parser *p) {
    if (p->i >= p->n)
        return status_line_error;

    if (c == '.') {
        p->i = 0;
        p->n = 2;  //FIXME: CREAR ENUM
        return status_line_version_minor;
    }

    if (!IS_DIGIT(c))
        return status_line_error;

    //FIXME:REVISAR ESTO
    uint8_t num = c - '0';

    uint16_t major = p->status_line->version_major;

    if (p->i == 0)
        major = num;
    else
        major = major * 10 + num;

    p->i++;
    p->status_line->version_major = major;

    return status_line_version_major;
}

static enum status_line_state
sl_version_minor(const uint8_t c, struct status_line_parser *p) {
    if (p->i >= p->n)
        return status_line_error;

    if (c == ' ') {
        p->i = 0;
        p->n = STATUS_CODE_LENGTH;
        return status_line_status_code;
    }

    if (!IS_DIGIT(c))
        return status_line_error;

    uint8_t num = c - '0';

    uint16_t minor = p->status_line->version_minor;

    if (p->i == 0)
        minor = num;
    else
        minor = minor * 10 + num;

    p->i++;
    p->status_line->version_minor = minor;

    return status_line_version_minor;
}
/*HTTP/1.1 200 OK */
static enum status_line_state
sl_status_code(const uint8_t c, struct status_line_parser *p) {
    if (p->i > p->n) {
        return status_line_error;
    }

    if (p->i == STATUS_CODE_LENGTH && c == ' ') {
        p->i = 0;
        p->n = MAX_REASON_PHRASE;
        return status_line_reason_phrase;
    }

    if (!IS_DIGIT(c))
        return status_line_error;

    uint8_t num = c - '0';

    uint16_t statusCode = p->status_line->status_code;

    if (p->i == 0)
        statusCode = num;
    else
        statusCode = statusCode * 10 + num;

    p->i++;
    p->status_line->status_code = statusCode;

    return status_line_status_code;
}

static enum status_line_state
sl_reason_phrase(const uint8_t c, struct status_line_parser *p) {
    if (p->i >= p->n) {
        return status_line_error;
    }

    if (c == '\r') {
        p->i = 0;
        return status_line_end;
    }

    if (!IS_REASON_PHRASE(c)) {
        return status_line_error;
    }

    p->status_line->reason_phrase[p->i++] = c;

    return status_line_reason_phrase;
}
static enum status_line_state
sl_end(const uint8_t c, struct status_line_parser *p) {
    if (c != '\n')
        return status_line_error;

    return status_line_done;
}
