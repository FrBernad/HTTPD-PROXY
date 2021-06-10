#ifndef PARSER_UTILS_H_c2f29bb6482d34fc6f94a09046bbd65a5f668acf
#define PARSER_UTILS_H_c2f29bb6482d34fc6f94a09046bbd65a5f668acf

/*
 * parser_utils.c -- factory de ciertos parsers típicos
 *
 * Provee parsers reusables, como por ejemplo para verificar que
 * un string es igual a otro de forma case insensitive.
 */
#include "parser.h"

#define IS_DIGIT(x) ((x) >= '0' && (x) <= '9')
#define IS_ALPHA(x) (((x) >= 'a' && (x) <= 'z') || ((x) >= 'A' && (x) <= 'Z'))
#define IS_TOKEN(x) ((x) == '!' || (x) == '#' || (x) == '$' || (x) == '%' || (x) == '&' || (x) == '\'' || (x) == '*' || (x) == '+' || (x) == '-' || (x) == '.' || (x) == '^' || (x) == '_' || (x) == '`' || (x) == '|' || (x) == '~' || IS_DIGIT(x) || IS_ALPHA(x))
#define END_OF_AUTHORITY(x) ((x) == '/' || (x) == '#' || (x) == '?')
#define IS_REASON_PHRASE(x) ((x) == ' ' || (x) == '\t' || IS_ALPHA(x) || IS_DIGIT(x))
#define IS_SPACE(x) ((x == ' ' || x == '\t'))
#define IS_VCHAR(x) ((x) >= 0x21 && (x) <= 0x7E)
#define IS_BASE_64(x) (IS_DIGIT(x) || IS_ALPHA(x) || (x) == '+' || (x) == '/' || (x) == '=')

enum string_cmp_event_types {
    STRING_CMP_MAYEQ,
    /** hay posibilidades de que el string sea igual */
    STRING_CMP_EQ,
    /** NO hay posibilidades de que el string sea igual */
    STRING_CMP_NEQ,
};

char *
parser_utils_strcmpi_event(enum string_cmp_event_types type);


/*
 * Crea un parser que verifica que los caracteres recibidos forment el texto
 * descripto por `s'.
 *
 * Si se recibe el evento `STRING_CMP_NEQ' el texto entrado no matchea.
 */
void
parser_utils_strcmpi(char *s, struct parser_definition * parser_definition);

#endif
