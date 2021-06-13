#ifndef _LOGGER_UTILS_H_3ee92ecd7369be131d48d6e6a85428aa80dde39d08a047611e637a2f
#define _LOGGER_UTILS_H_3ee92ecd7369be131d48d6e6a85428aa80dde39d08a047611e637a2f

#include "utils/selector/selector.h"
#include "logger/logger.h"

void
log_new_connection(struct selector_key *key);

void
log_user_and_password(struct selector_key *key);

void
log_connection_closed(struct selector_key *key);

void
log_connection_failed(struct selector_key *key, char *reason_phrase);

void
log_level_msg(char *msg, log_level level);

#endif
