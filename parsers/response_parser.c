#include "response_parser.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>
#include "../utils/parser_utils.h"

static enum response_state
r_version(const uint8_t c, struct response_parser *p);

static enum response_state
r_version_major(const uint8_t c, struct response_parser *p);

static enum response_state
r_version_minor(const uint8_t c, struct response_parser *p);

static enum response_state
r_status_code(const uint8_t c, struct response_parser *p);

static enum response_state
r_reason_phrase(const uint8_t c, struct response_parser *p);

static enum response_state
r_end(const uint8_t c, struct response_parser *p);

void response_parser_init(struct response_parser *p) {
    p->state = response_version;
    p->i = 0;
    p->n = VERSION_LENGTH;
    memset(p->response, 0, sizeof(*(p->response)));
}

enum response_state response_parser_feed(struct response_parser *p, const uint8_t c) {
    enum response_state next;

    switch (p->state) {
        case response_version:
            next = r_version(c, p);
            break;
        case response_version_major:
            next = r_version_major(c, p);
            break;
        case response_version_minor:
            next = r_version_minor(c, p);
            break;
        case response_status_code:
            next = r_status_code(c, p);
            break;
        case response_reason_phrase:
            next = r_reason_phrase(c, p);
            break;
        case response_end:
            next = r_end(c, p);
            break;
        case response_done:
        case response_error:
        case response_error_unsupported_version:
            next = p->state;
            break;
        default:
            next = response_error;
            break;
    }

    return p->state = next;
}

static enum response_state
r_version(const uint8_t c, struct response_parser *p) {
    char *version = HTTP;

    if (p->i >= p->n || version[p->i] != c)
        return response_error;

    p->i++;

    if (c == '/') {
        p->i = 0;
        p->n = 2;  //FIXME: CREAR ENUM
        return response_version_major;
    }

    return response_version;
    ;
}

static enum response_state
r_version_major(const uint8_t c, struct response_parser *p) {
    if (p->i >= p->n)
        return response_error;

    if (c == '.') {
        p->i = 0;
        p->n = 2;  //FIXME: CREAR ENUM
        return response_version_minor;
    }

    if (!IS_DIGIT(c))
        return response_error;

    //FIXME:REVISAR ESTO
    uint8_t num = c - '0';

    uint16_t major = p->response->version_major;

    if (p->i == 0)
        major = num;
    else
        major = major * 10 + num;

    p->i++;
    p->response->version_major = major;

    return response_version_major;
}

static enum response_state
r_version_minor(const uint8_t c, struct response_parser *p) {
    if (p->i >= p->n)
        return response_error;

    if (c == ' ') {
        p->i = 0;
        p->n = STATUS_CODE_LENGTH;
        return response_status_code;
    }

    if (!IS_DIGIT(c))
        return response_error;

    uint8_t num = c - '0';

    uint16_t minor = p->response->version_minor;

    if (p->i == 0)
        minor = num;
    else
        minor = minor * 10 + num;

    p->i++;
    p->response->version_minor = minor;

    return response_version_minor;
}
/*HTTP/1.1 200 OK */
static enum response_state
r_status_code(const uint8_t c, struct response_parser *p) {
    if (p->i > p->n){
        return response_error;
    }

    if (p->i == STATUS_CODE_LENGTH && c == ' ') {
        p->i = 0;
        p->n = MAX_REASON_PHRASE;
        return response_reason_phrase;
    }

    if (!IS_DIGIT(c))
        return response_error;

    uint8_t num = c - '0';

    uint16_t statusCode = p->response->status_code;

    if (p->i == 0)
        statusCode = num;
    else
        statusCode = statusCode * 10 + num;

    p->i++;
    p->response->version_minor = statusCode;

    return response_status_code;
}

static enum response_state
r_reason_phrase(const uint8_t c, struct response_parser *p) {

    if (p->i >= p->n){
        return response_error;
    }

    if(c == '\r'){
        p->i = 0;
        return response_end;
    }

    if(!IS_REASON_PHRASE(c)){
        return response_error;
    }


    p->response->reason_phrase[p->i++] = c;

    return response_reason_phrase;
   
}
static enum response_state
r_end(const uint8_t c, struct response_parser *p) {
    if (c != '\n')
        return response_error;

    return response_done;
}
