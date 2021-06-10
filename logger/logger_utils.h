#ifndef LOGGER_UTILS_H
#define LOGGER_UTILS_H

#include "utils/selector/selector.h"

void log_new_connection(struct selector_key *key);
void log_user_and_password(struct selector_key *key);
void log_connection_closed(struct selector_key *key);
void log_connection_failed(struct selector_key *key);
void log_error(char* msg);
void log_fatal_error(char* msg);

#endif
