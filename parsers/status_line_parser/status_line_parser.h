#ifndef _STATUS_LINE_PARSER_H_ccd96ae151f1acb44a4700613805f5cf9640117e628c19b4a5c1183e
#define _STATUS_LINE_PARSER_H_ccd96ae151f1acb44a4700613805f5cf9640117e628c19b4a5c1183e

#include <netinet/in.h>

/*   The HTTP status line is formed as follows:
 *
 *   The first line of a status_line message is the status-line, consisting
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

enum data_size{

    STATUS_CODE_LENGTH = 3,
    MAX_REASON_PHRASE = 50,
    /** max port length (65535)*/
    STATUS_VERSION_LENGTH = 8,
};

struct status_line {
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

typedef enum status_line_state {
    status_line_version,
    status_line_version_major,
    status_line_version_minor,
    status_line_status_code,
    status_line_reason_phrase,

    status_line_end,

    //Done
    status_line_done,

    //Error
    status_line_error,
    status_line_error_unsupported_version,
} status_line_state;

typedef struct status_line_parser {
    struct status_line *status_line;
    status_line_state state;

    int i;
    int n;
}status_line_parser_t;

/** init parser */
void status_line_parser_init(status_line_parser_t *p);

/** returns true if done */
enum status_line_state
status_line_parser_feed(status_line_parser_t *p, uint8_t c);

#endif
