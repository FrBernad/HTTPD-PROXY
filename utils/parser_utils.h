#ifndef _PARSER_H
#define _PARSER_H_

#define CONNECT "CONNECT"
#define OPTIONS "OPTIONS"
#define SCHEME "http://"
#define HTTP "HTTP/"
#define IS_DIGIT(x) ((x) >= '0' && (x)<= '9' )
#define IS_ALPHA(x) (((x) >= 'a' && (x) <= 'z' ) || ((x) >= 'A' && (x) <= 'Z' ))
#define IS_TOKEN(x) ((x) == '!' || (x) == '#' || (x) == '$' || (x) == '%' || (x) == '&' || (x) == '\'' || (x) == '*' || (x) == '+' \
|| (x) == '-' || (x) == '.' || (x) == '^' || (x) == '_' || (x) == '`' || (x) == '|' || (x) == '~' || IS_DIGIT(x) || IS_ALPHA(x))
#define END_OF_AUTHORITY(x) ((x) == '/' || (x) == '#' || (x) == '?')
#define IS_REASON_PHRASE(x) ((x) == ' ' || (x) == '\t' || IS_ALPHA(x) || IS_DIGIT(x))
#define IS_SPACE(x) ((x == ' ' || x == '\t'))
#define IS_VCHAR(x) ((x)>=0x21 && (x)<=0x7E)

#endif