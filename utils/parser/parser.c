#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "parser.h"

void
parser_destroy(struct parser *p) {
    if(p != NULL) {
        free(p);
    }
}

struct parser *
parser_init(unsigned *classes,
            struct parser_definition def) {
    struct parser *ret = malloc(sizeof(*ret));
    if(ret != NULL) {
        memset(ret, 0, sizeof(*ret));
        ret->classes = classes;
        memcpy(&ret->def, &def, sizeof(def));
        ret->state   = def.start_state;
    }
    return ret;
}

void
parser_reset(struct parser *p) {
    p->state   = p->def.start_state;
}

struct parser_event *
parser_feed(struct parser *p, uint8_t c) {
    unsigned type = p->classes[c];

    p->e1.next = p->e2.next = 0;

    struct parser_state_transition *state = p->def.states[p->state];
    size_t n                              = p->def.states_n[p->state];
    bool matched   = false;

    for(unsigned i = 0; i < n ; i++) {
        int when = state[i].when;
        if (state[i].when <= 0xFF) {
            matched = (c == when);
        } else if(state[i].when == ANY) {
            matched = true;
        } else if(state[i].when > 0xFF) {
            matched = (type & when);
        } else {
            matched = false;
        }

        if(matched) {
            state[i].act1(&p->e1, c);
            if(state[i].act2 != NULL) {
                p->e1.next = &p->e2;
                state[i].act2(&p->e2, c);
            }
            p->state = state[i].dest;
            break;
        }
    }
    return &p->e1;
}


static unsigned classes[0xFF] = {0x00};

unsigned *
parser_no_classes(void) {
    return classes;
}

