#ifndef LOGGER_UTILS_H
#define LOGGER_UTILS_H

#include "utils/selector/selector.h"
#include "logger/logger.h"

void log_new_connection(struct selector_key *key);
void log_user_and_password(struct selector_key *key);
void log_connection_closed(struct selector_key *key);
void log_connection_failed(struct selector_key *key,char * reasonPhrase);
void log_level_msg(char *msg,log_level level);
#endif
