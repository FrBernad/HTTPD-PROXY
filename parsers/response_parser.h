#ifndef _RESPONSE_PARSER_H_
#define _RESPONSE_PARSER_H_

#include <netinet/in.h>

/*   The HTTP status line is formed as follows:
 *
 *   The first line of a response message is the status-line, consisting
 *   of the protocol version, a space (SP), the status code, another
 *   space, a possibly empty textual phrase describing the status code,
 *   and ending with CRLF.
 *
 *     o status-line = HTTP-version SP status-code SP reason-phrase CRLF
 *
 *     o status-code    = 3DIGIT
 *
 *   The reason-phrase element exists for the sole purpose of providing a
 *   textual description associated with the numeric status code, mostly
 *   out of deference to earlier Internet application protocols that were
 *   more frequently used with interactive text clients.  A client SHOULD
 *   ignore the reason-phrase content.
 * 
 *     o reason-phrase  = *( HTAB / SP / VCHAR / obs-text )
 *    
 *     o HTTP-version = HTTP-name "/" DIGIT "." DIGIT
 *       


 */

enum {

    STATUS_CODE_LENGTH = 3,
    MAX_REASON_PHRASE = 50,
    /** max port length (65535)*/
    VERSION_LENGTH = 8,
};

struct response_line {
    /**
     * HTTP major version.
     */
    uint8_t version_major;

    /**
     * HTTP minor version.
     */
    uint8_t version_minor;

    int status_code;

    uint8_t reason_phrase[MAX_REASON_PHRASE];

};

typedef enum response_state {
    response_version,
    response_version_major,
    response_version_minor,
    response_status_code,
    response_reason_phrase,

    response_end,

    //Done
    response_done,

    //Error
    response_error,
    response_error_unsupported_version,
} response_state;

struct response_parser {
    struct response_line *response;
    response_state state;

    int i;
    int n;
};

/** init parser */
void response_parser_init(struct response_parser *p);

/** returns true if done */
enum response_state
response_parser_feed(struct response_parser *p, const uint8_t c);

#endif
