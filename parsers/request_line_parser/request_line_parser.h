#ifndef _REQUEST_LINE_PARSER_H_
#define _REQUEST_LINE_PARSER_H_

#include <netinet/in.h>

/*   The HTTP request line is formed as follows:
 *
 *   A request-line begins with a method token, followed by a single space
 *   (SP), the request-target, another single space (SP), the protocol
 *   version, and ends with CRLF.
 *
 *   request-line   = method SP request-target SP HTTP-version CRLF
 *
 *   Where:
 *      
 *     METHODS ARE CASE SENSITIVE
 *
 *        o method  = token
 *               token = 1*tchar
 *               tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*"
 *               / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
 *               / DIGIT / ALPHA; any VCHAR, except delimiters 
 * 
 *        o request-target = absolute-form | authority-form # authority for CONNECT AND OPTIONS
 *                  …
 *                  absolute-form  = absolute-URI
 *                  absolute-URI  = scheme ":" hier-part [ "?" query ]
 *                  …
 *                  http-URI = "http:" "//" authority path-abempty [ "?" query ]
 *                                  [ "#" fragment ]
 *
 *                  authority   = [ userinfo "@" ] host [ ":" port ]
 *                  path-abempty  = *( "/" segment )
 * 
 *         o HTTP-version = HTTP-name "/" DIGIT "." DIGIT
 *       
 */

enum {
    /**  max length for HTTP method */
    MAX_METHOD_LENGTH = 24,

    /** max length for DNS request FQDN */
    MAX_FQDN_LENGTH = 0xFF,

    /** scheme length */
    SCHEME_LENGTH = 7,

    /** max port length (65535)*/
    MAX_PORT_LENGTH = 5,

    /** max port length (65535)*/
    VERSION_LENGTH = 8,

    MAX_ORIGIN_FORM = 1 << 10,
};

struct request_line {

    /**
     * HTTP major version.
     */
    uint8_t version_major;

    /**
     * HTTP minor version.
     */
    uint8_t version_minor;

    /**
     * HTTP method (NUL terminated)
     */
    uint8_t method[MAX_METHOD_LENGTH + 1];

    struct request_target {
        // request_target type.
        enum {
            absolute_form,
            authority_form,
        } type;

        /** host type */
        enum host_type {
            domain,
            ipv4,
            ipv6,
        } host_type;

        /** host no connect */
        union {
            /** NULL-terminated FQDN */
            char domain[MAX_FQDN_LENGTH + 1];
            /** IPV4 */
            struct sockaddr_in ipv4;
            /** IPV6 */
            struct sockaddr_in6 ipv6;
        } host;

        /** port in network byte order */
        in_port_t port;

        // origin-form    = absolute-path [ "?" query ]
        uint8_t origin_form[MAX_ORIGIN_FORM];
    } request_target;
    
};

typedef enum request_state {
    request_method,
    request_target_scheme,
    request_target_host,
    request_target_port,
    request_target_ogform,
    request_version,
    request_version_major,
    request_version_minor,
    request_end,

    //Done
    request_done,

    //Error
    request_error,
    request_error_unsupported_version,
    request_error_unsupported_method,
} request_state;

struct request_parser {
    struct request_line *request;
    request_state state;

    int i;
    int n;
};


/** init parser */
void 
request_parser_init(struct request_parser *p);

/** returns true if done */
enum request_state 
request_parser_feed(struct request_parser *p, const uint8_t c);

#endif
