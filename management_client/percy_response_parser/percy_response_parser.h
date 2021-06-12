#ifndef PERCY_RESPONSE_PARSER_H
#define PERCY_RESPONSE_PARSER_H

#include <stdint.h>
/*        +----------+----------+----------+----------+
        |    VER   |  STATUS  |   RESV   |   VALUE  |
        +----------+----------+----------+----------+
        |     1    |     1    |    1     |     8    |
        +----------+----------+----------+----------+
*/

enum{
    VALUE_LEN = 8,
};

struct percy_response {
    uint8_t ver;
    uint8_t status;
    uint8_t resv;
    uint64_t value;
};

typedef enum percy_response_state {

    percy_response_ver,
    percy_response_status,
    percy_response_resv,
    percy_response_value,

    percy_response_done,
    //Error
    percy_response_error,
} percy_response_state;

typedef struct percy_response_parser {
    struct percy_response *response;
    percy_response_state state;
    int i;
    int n;
}percy_response_parser_t;

/** init parser */
void 
percy_response_parser_init(percy_response_parser_t *p);

/** returns true if done */
enum percy_response_state 
percy_response_parser_feed(percy_response_parser_t *p, uint8_t c);



#endif