#ifndef _PERCY_REQUEST_PARSER_H_e51430920c310d631533698c148d43d92300e347f79b2dbb82d1ea88
#define _PERCY_REQUEST_PARSER_H_e51430920c310d631533698c148d43d92300e347f79b2dbb82d1ea88

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

typedef struct request_percy {
    uint8_t ver;
    uint8_t type;
    uint8_t passphrase[PASS_PHRASE_LEN];
    uint8_t method;
    uint8_t resv;
    uint16_t value;
}request_percy_t;



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

typedef struct percy_request_parser {
    request_percy_t *request;
    percy_request_state state;
    int i;
    int n;
}percy_request_parser_t;

/** init parser */
void 
percy_request_parser_init(percy_request_parser_t *p);

/** returns true if done */
enum percy_request_state 
percy_request_parser_feed(percy_request_parser_t *p, uint8_t c);

#endif