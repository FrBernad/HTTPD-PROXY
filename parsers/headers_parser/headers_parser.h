#ifndef _HEADERS_PARSER_H_
#define _HEADERS_PARSER_H_

#include <netinet/in.h>

/*  Each header field consists of a case-insensitive field name followed
    by a colon (":"), optional leading whitespace, the field value, and
    optional trailing whitespace.
        
        HTTP-message = 
            start-line
            *( header-field CRLF )
            CRLF
            [ message-body ]
            
        header-field   = field-name ":" OWS field-value OWS

        field-name     = token
        field-value    = *( field-content / obs-fold )
        field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
        field-vchar    = VCHAR / obs-text

        obs-fold       = CRLF 1*( SP / HTAB )
                        ; obsolete line folding
                        ; see Section 3.2.4

        As a convention, ABNF rule names prefixed with "obs-" denote
        "obsolete" grammar rules that appear for historical reasons.
        OWS: zero or more linear whitespace octets might appear.
    	
        token          = 1*tchar

        tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                        / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                        / DIGIT / ALPHA
                        ; any VCHAR, except delimiters
                    
    Field Extensibility

        Header fields are fully extensible: there is no limit on the
        introduction of new field names, each presumably defining new
        semantics, nor on the number of header fields used in a given
        message.

        A proxy MUST forward unrecognized header fields unless the field-name
        is listed in the Connection header field (Section 6.1) or the proxy
        is specifically configured to block, or otherwise transform, such
        fields.  

    Field Parsing

        No whitespace is allowed between the header field-name and colon. A
        server MUST reject any received request message that contains
        whitespace between a header field-name and colon with a response code
        of 400 (Bad Request).  A proxy MUST remove any such whitespace from a
        response message before forwarding the message downstream.

        A field value might be preceded and/or followed by optional whitespace(OWS); 
        a single SP preceding the field-value is preferred for consistent readability by humans. 
        The field value does not  include any leading or trailing whitespace:
        OWS occurring before the first non-whitespace octet of the field value or after the last
        non-whitespace octet of the field value ought to be excluded by
        parsers when extracting the field value from a header field. 

    Field Limits

        HTTP does not place a predefined limit on the length of each header
        field or on the length of the header section as a whole, as described
        in Section 2.5.  Various ad hoc limitations on individual header
        field length are found in practice, often depending on the specific
        field semantics.

        A server that receives a request header field, or set of fields,
        larger than it wishes to process MUST respond with an appropriate 4xx
        (Client Error) status code.  Ignoring such header fields would
        increase the server's vulnerability to request smuggling attacks
        (Section 9.5).


    Field Order

        The order in which header fields with differing field names are
        received is not significant. However, it is good practice to send
        header fields that contain control data first, such as Host on
        requests and Date on responses, so that implementations can decide
        when not to handle a message as early as possible. A server MUST NOT
        apply a request to the target resource until the entire request

        header section is received, since later header fields might include
        conditionals, authentication credentials, or deliberately misleading
        duplicate header fields that would impact request processing.
        A sender MUST NOT generate multiple header fields with the same field
        name in a message unless either the entire field value for that
        header field is defined as a comma-separated list [i.e., #(values)]
        or the header field is a well-known exception (as noted below).

        A recipient MAY combine multiple header fields with the same field
        name into one "field-name: field-value" pair, without changing the
        semantics of the message, by appending each subsequent field value to
        the combined field value in order, separated by a comma. The order
        in which header fields with the same field name are received is
        therefore significant to the interpretation of the combined field
        value; a proxy MUST NOT change the order of these field values when
        forwarding a message.

        Note: In practice, the "Set-Cookie" header field ([RFC6265]) often
        appears multiple times in a response message and does not use the
        list syntax, violating the above requirements on multiple header
        fields with the same name. Since it cannot be combined into a
        single field-value, recipients ought to handle "Set-Cookie" as a
        special case while processing header fields. (See Appendix A.2.3
        of [Kri2001] for details.)

 *       
 */

enum {
    /**  max length for HTTP field */
    MAX_HEADER_FIELD_NAME_LENGTH = 128,

    /**  max length for HTTP field */
    MAX_HEADER_FIELD_VALUE_LENGTH = 1 << 10,

};

typedef enum headers_state {

    headers_field_name,
    headers_field_value,
    headers_field_value_end,
    headers_may_be_end,
    headers_end,

    //Done
    headers_done,

    //Error
    headers_error,
} headers_state;

struct headers_parser {
    headers_state state;
    unsigned headersCount;

    int i;
    int n;
};

/** init parser */
void 
headers_parser_init(struct headers_parser *p);

/** returns true if done */
enum headers_state
headers_parser_feed(struct headers_parser *p, const uint8_t c);

#endif
