#include <stdio.h>
#include <string.h>
#include "headers_parser.h"

int main(int argc, char const *argv[])
{
    char * headers = "Location: http://www.google.com/\r\nContent-Type: text/html; charset=UTF-8\r\nDate: Sun, 06 Jun 2021 01:27:22 GMT\r\nExpires: Tue, 06 Jul 2021 01:27:22 GMT\r\nCache-Control: public, max-age=2592000\r\nServer: gws\r\nContent-Length: 219\r\nX-XSS-Protection: 0\r\nX-Frame-Options: SAMEORIGIN\r\n\r\n";

    struct headers_parser parser;

    headers_parser_init(&parser);

    int length = strlen(headers);
    enum headers_state state;

    for (int i = 0; i < length; i++) {
        state = headers_parser_feed(&parser,headers[i]);
        if(state == headers_done){
            printf("DONE!!! Headers count: %d\n",parser.headersCount);
        }
        if (state == headers_error) {
            printf("ERROR!\n");
        }
    }

    return 0;
}
