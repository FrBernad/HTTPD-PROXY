#include "headers_parser.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "utils/base64/base64.h"
#include "utils/parser/parser_utils.h"

static enum headers_state h_field_name(const uint8_t c, struct headers_parser *p);

static enum headers_state h_field_value(const uint8_t c, struct headers_parser *p);

static enum headers_state h_end(const uint8_t c, struct headers_parser *p);

static enum headers_state h_may_be_end(const uint8_t c, struct headers_parser *p);

static enum headers_state h_field_value_end(const uint8_t c, struct headers_parser *p);

static void parseAuthorizationValue(struct headers_parser *p);
void parseUserAndPassword(char *authorization, int len, struct headers_parser *p);
void parseFieldValueAndType(char *fieldValue, char *a_value, char *a_type);

void headers_parser_init(struct headers_parser *p) {
    p->state = headers_field_name;
    p->authorization.a_type = NULL;
    p->authorization.user = NULL;
    p->authorization.password = NULL;
    p->headersCount = 0;
    p->i = 0;
    p->n = MAX_HEADER_FIELD_NAME_LENGTH;
}

enum headers_state
headers_parser_feed(struct headers_parser *p, const uint8_t c) {
    enum headers_state next;

    switch (p->state) {
        case headers_field_name:
            next = h_field_name(c, p);
            break;
        case headers_field_value:
            next = h_field_value(c, p);
            break;
        case headers_field_value_end:
            next = h_field_value_end(c, p);
            break;
        case headers_may_be_end:
            next = h_may_be_end(c, p);
            break;
        case headers_end:
            next = h_end(c, p);
            break;

            //Done
        case headers_done:
        case headers_error:
            next = p->state;
            break;
        default:
            next = headers_error;
            break;
    }

    return p->state = next;
}

static enum headers_state h_field_name(const uint8_t c, struct headers_parser *p) {
    if (p->i >= p->n) {
        return headers_error;
    }

    if (c == ':') {
        p->headersCount++;
        p->current_header.c_field_name[p->i] = 0;
        p->i = 0;
        p->n = MAX_HEADER_FIELD_VALUE_LENGTH;
        return headers_field_value;
    }

    if (!IS_TOKEN(c)) {
        return headers_error;
    }

    p->current_header.c_field_name[p->i++] = c;

    return headers_field_name;
}

static enum headers_state h_field_value(const uint8_t c, struct headers_parser *p) {
    if (p->i >= p->n)
        return headers_error;

    if (c == '\r') {
        p->current_header.c_field_value[p->i] = 0;
        return headers_field_value_end;
    }

    if (IS_VCHAR(c) || IS_SPACE(c)) {
        p->current_header.c_field_value[p->i++] = c;
        return headers_field_value;
    }

    return headers_error;
}

static enum headers_state h_field_value_end(const uint8_t c, struct headers_parser *p) {
    if (c != '\n') {
        return headers_error;
    }

    if (strcasecmp(p->current_header.c_field_name, AUTHORIZATION_HEADER) == 0) {
        parseAuthorizationValue(p);
    }
    return headers_may_be_end;
}

static enum headers_state h_may_be_end(const uint8_t c, struct headers_parser *p) {
    if (c == '\r') {
        return headers_end;
    }

    if (!IS_TOKEN(c)) {
        return headers_error;
    }
    p->i = 0;
    p->n = MAX_HEADER_FIELD_NAME_LENGTH;

    p->current_header.c_field_name[p->i++] = c;
    return headers_field_name;
}

static enum headers_state h_end(const uint8_t c, struct headers_parser *p) {
    if (c != '\n') {
        return headers_error;
    }

    return headers_done;
}

static void parseAuthorizationValue(struct headers_parser *p) {
    char a_value[MAX_HEADER_FIELD_VALUE_LENGTH];
    char a_type[MAX_TYPE_AUTHORIZATION_LENGTH];
    char *fieldValue = p->current_header.c_field_value;

    /*Salteo espacios del principio si es que hay*/

    int i = 0;
    while (fieldValue[i] != 0 && fieldValue[i] == ' ') {
        i++;
    }

    int j = 0;
    while (fieldValue[i] != 0 && fieldValue[i] != ' ') {
        a_type[j++] = fieldValue[i++];
    }
    a_type[j] = 0;

    p->authorization.a_type = malloc(j + 1);
    if(p->authorization.a_type == NULL){
        return;
    }
    strcpy(p->authorization.a_type, a_type);

    while (fieldValue[i] != 0 && fieldValue[i] == ' ') {
        i++;
    }

    int k = 0;
    while (fieldValue[i] != 0) {
        a_value[k++] = fieldValue[i++];
    }
    a_value[k] = 0;

    int len;
    char *authorization = base64_decode(a_value, k, (size_t *)&len);
    if(authorization == NULL){
        free(p->authorization.a_type);
        p->authorization.a_type = NULL;
        return;
    }
    
    if(parseUserAndPassword(authorization, len, p) < 0){
        free(p->authorization.a_type);
        p->authorization.a_type = NULL;
    }

    free(authorization);
}

int parseUserAndPassword(char *authorization, int len, struct headers_parser *p) {
    int i = 0;
    while (i < len && authorization[i] != ':') {
        i++;
    }

    if (i < len) {
        p->authorization.user = malloc(i + 1);
        if(p->authorization.user == NULL){
            return -1;
        }
        memcpy(p->authorization.user, authorization, i);
        p->authorization.user[i] = 0;
    }
    i++;

    p->authorization.password = malloc(len - i + 1);
    if(p->authorization.password == NULL){
        free(p->authorization.user);
        p->authorization.user = NULL;
        return -1;
    }
    memcpy(p->authorization.password, authorization + i, len - i);
    p->authorization.password[len - i] = 0;
}
