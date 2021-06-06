#include <stdio.h>
#include <string.h>

#include "response_parser.h"

int main(int argc, char const *argv[])
{
    char *response = "HTTP/1.1 301 Moved Permanently\r\n";

    struct response_parser parser;
    struct response_line responseLine;
    
    parser.response = &responseLine;

    response_parser_init(&parser);

    int length = strlen(response);
    enum response_state state;

    for (int i = 0; i < length; i++) {
        state = response_parser_feed(&parser, response[i]);
        if (state == response_done) {
            printf("DONE!!! Version HTTP/%d.%d CODE:%d\n", responseLine.version_major, responseLine.version_minor, responseLine.status_code);
        }
        if (state == response_error) {
            printf("ERROR!\n");
        }
    }

    return 0;
}
