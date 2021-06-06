#include <stdio.h>
#include <string.h>

#include "response_parser.h"

int main(int argc, char const *argv[])
{
    char *response = "HTTP/1.1 301 Moved Permanently\r\n";

    struct response_parser parser;
    struct response_line response;

    response_parser_init(&parser, &response);

    int length = strlen(response);
    enum response_state state;

    for (int i = 0; i < length; i++) {
        state = response_parser_feed(&parser, response[i]);
        if (state == repsonse_done) {
            printf("DONE!!! Version HTTP/%d.%d\n", response.);
        }
    }

    return 0;
}
