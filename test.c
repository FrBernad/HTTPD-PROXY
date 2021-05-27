#include <parser.h>
#include <parser_utils.h>
#include <stdio.h>
#include <string.h>

#define MAX_HOST_LENGTH 20

int main(int argc, char const *argv[]) {
    const struct parser_definition d = parser_utils_strcmpi("host:");
    struct parser *parser = parser_init(parser_no_classes(), &d);

    char *test = "> GET / HTTP/1.1 \r\n Ho";
    int size = strlen(test);
    char host[MAX_HOST_LENGTH + 1] = {0};
    int hostLength = 0;

    const struct parser_event *event;
    int i;
    for (i = 0; i < size; i++) {
        event = parser_feed(parser, test[i]);
        if (event->type == STRING_CMP_EQ) {
            i++;
            break;
        } else if (event->type == STRING_CMP_NEQ) {
            parser_reset(parser);
        }
    }

    char *test2 = "st: 127.0.0.1 \r\n User - Agent: curl / 7.74.0 \r\n Accept: */*";

    const struct parser_event *event;
    int i;
    for (i = 0; i < size; i++) {
        event = parser_feed(parser, test[i]);
        if (event->type == STRING_CMP_EQ) {
            i++;
            break;
        } else if (event->type == STRING_CMP_NEQ) {
            parser_reset(parser);
        }
    }

    parser_destroy(parser);
    parser_utils_strcmpi_destroy(&d);

    int error = 0;

    while (i < size) {
        if (hostLength == MAX_HOST_LENGTH - 1 && test[i] != '\r') {
            error = 1;
            break;
        }
 
        if (test[i] == '\r' && test[i + 1] == '\n') {
            break;
        }

        if (test[i] != '\t' && test[i] != ' ') {
            host[hostLength] = test[i];
            hostLength++;
        }
        i++;
    }

    if (!error) {
        host[hostLength] = 0;
        printf("success, host:%s\n", host);
    } else
        printf("error parsing host header");

    return 0;
}
