#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>

#include "utils/selector/selector.h"

int
init_logger(fd_selector s);

void 
logger_log(char *log, uint64_t totalBytes);

#endif