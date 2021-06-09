#ifndef PERCY_REQUEST_PARSER_H
#define PERCY_REQUEST_PARSER_H

#include <stdint.h>

/*
    +----------+----------+----------+----------+----------+----------+----------+
    |    VER   |      PASSPHRASE     |   TYPE   |  METHOD  |   RESV   |   VALUE  |
    +----------+---------------------+----------+----------+----------+----------+
    |     1    |          6          |    1     |     1    |    1     |    2     |
    +----------+----------+----------+----------+----------+----------+----------+

*/

enum {
    PASS_PHRASE_LEN = 6,
    VALUE_LEN = 2,
    PERCY_REQUEST_SIZE = 12
};

struct request_percy {
    uint8_t ver;
    uint8_t type;
    uint8_t passphrase[PASS_PHRASE_LEN];
    uint8_t method;
    uint8_t resv;
    uint16_t value;
};

typedef enum percy_request_state {

    percy_request_ver,
    percy_request_type,
    percy_request_passphrase,
    percy_request_method,
    percy_request_resv,
    percy_request_value,

    percy_request_done,

    //Error
    percy_request_error,
} percy_request_state;

struct percy_request_parser {
    struct request_percy *request;
    percy_request_state state;
    int i;
    int n;
};

/** init parser */
void 
percy_request_parser_init(struct percy_request_parser *p);

/** returns true if done */
enum percy_request_state 
percy_request_parser_feed(struct percy_request_parser *p, const uint8_t c);


#endif