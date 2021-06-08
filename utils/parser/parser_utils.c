#include "parser_utils.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *
parser_utils_strcmpi_event(enum string_cmp_event_types type) {
    char *ret = NULL;

    switch (type) {
        case STRING_CMP_MAYEQ:
            ret = "wait(c)";
            break;
        case STRING_CMP_EQ:
            ret = "eq(c)";
            break;
        case STRING_CMP_NEQ:
            ret = "neq(c)";
            break;
    }
    return ret;
}

static void
may_eq(struct parser_event *ret, uint8_t c) {
    ret->type = STRING_CMP_MAYEQ;
    ret->n = 1;
    ret->data[0] = c;
}

static void
eq(struct parser_event *ret, uint8_t c) {
    ret->type = STRING_CMP_EQ;
    ret->n = 1;
    ret->data[0] = c;
}

static void
neq(struct parser_event *ret, uint8_t c) {
    ret->type = STRING_CMP_NEQ;
    ret->n = 1;
    ret->data[0] = c;
}

/*
 * para comparar "foo" (length 3) necesitamos 3 + 2 estados.
 * Los útimos dos, son el sumidero de comparación fallida, y
 * el estado donde se llegó a la comparación completa.
 *
 * static struct parser_state_transition ST_0 [] =  {
 *   {.when = 'F',        .dest = 1,         .action1 = may_eq, },
 *   {.when = 'f',        .dest = 1,         .action1 = may_eq, },
 *   {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 * };
 * static struct parser_state_transition ST_1 [] =  {
 *   {.when = 'O',        .dest = 2,         .action1 = may_eq, },
 *   {.when = 'o',        .dest = 2,         .action1 = may_eq, },
 *   {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 * };
 * static struct parser_state_transition ST_2 [] =  {
 *   {.when = 'O',        .dest = EQ,        .action1 = eq, },
 *   {.when = 'o',        .dest = EQ,        .action1 = eq, },
 *   {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 * };
 * static struct parser_state_transition ST_EQ  (3) [] =  {
 *   {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 * };
 * static struct parser_state_transition ST_NEQ (4) [] =  {
 *   {.when = ANY,        .dest = NEQ,       .action1 = neq,},
 * };
 *
 */
void parser_utils_strcmpi(char *s, struct parser_definition *def) {
    size_t n = strlen(s);

    struct parser_state_transition states[MAX_STATES][STATE_TRANSITIONS];
    size_t nstates[MAX_STATES];

    memset(states, 0, sizeof(states));
    memset(nstates, 0, sizeof(nstates));

    // estados fijos
    size_t st_eq = n;
    size_t st_neq = n + 1;

    for (size_t i = 0; i < n; i++) {
        size_t dest = (i + 1 == n) ? st_eq : i + 1;

        states[i][0].when = tolower(s[i]);
        states[i][0].dest = dest;
        states[i][0].act1 = i + 1 == n ? eq : may_eq;
        states[i][1].when = toupper(s[i]);
        states[i][1].dest = dest;
        states[i][1].act1 = i + 1 == n ? eq : may_eq;
        states[i][2].when = ANY;
        states[i][2].dest = st_neq;
        states[i][2].act1 = neq;
        nstates[i] = 3;
    }

    // EQ
    states[n][0].when = ANY;
    states[n][0].dest = st_neq;
    states[n][0].act1 = neq;
    nstates[n] = 1;
    // NEQ
    states[n + 1][0].when = ANY;
    states[n + 1][0].dest = st_neq;
    states[n + 1][0].act1 = neq;
    nstates[n + 1] = 1;

    def->start_state = 0;
    def->states_count = n+2;

    memcpy(def->states, states, sizeof(states));
    memcpy(def->states_n, nstates, sizeof(nstates));

}
