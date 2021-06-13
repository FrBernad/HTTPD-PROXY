#ifndef _PERCY_RESPONSE_PARSER_H_0e5ee1c0a0072ac39c7ab7c3397fb2feda42d0528f1f3b9fa83a08a9
#define _PERCY_RESPONSE_PARSER_H_0e5ee1c0a0072ac39c7ab7c3397fb2feda42d0528f1f3b9fa83a08a9

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