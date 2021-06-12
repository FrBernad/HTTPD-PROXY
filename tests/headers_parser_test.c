#include "headers_parser.h"

#include <stdio.h>
#include <string.h>
#include "logger/logger_utils.h"

int main(int argc, char const *argv[]) {
    char *headers = "Location: http://www.google.com/\r\nAuthorization: Basic YWxhZGRpbjpvcGVuc2VzYW1l\r\nContent-Type: text/html; charset=UTF-8\r\nDate: Sun, 06 Jun 2021 01:27:22 GMT\r\nExpires: Tue, 06 Jul 2021 01:27:22 GMT\r\nCache-Control: public, max-age=2592000\r\nServer: gws\r\nContent-Length: 219\r\nX-XSS-Protection: 0\r\nX-Frame-Options: SAMEORIGIN\r\n\r\n";

    struct headers_parser parser;

    headers_parser_init(&parser);

    int length = strlen(headers);
    enum headers_state state;

    for (int i = 0; i < length; i++) {
        state = headers_parser_feed(&parser, headers[i]);
        if (state == headers_done) {

            log_level_msg("DONE!!! Headers count: %d\n", parser.headersCount,);
        }
        if (state == headers_error) {
            log_level
        }
    }

    printf("%s\n",parser.authorization.a_type);
    printf("%s\n",parser.authorization.a_value);


    return 0;
}
