#ifndef _LOGGER_H_baa492942894a26aa8ecec51eaf3d6c47b01f19d2de6d59f31988589
#define _LOGGER_H_baa492942894a26aa8ecec51eaf3d6c47b01f19d2de6d59f31988589

#include <stdint.h>

#include "utils/selector/selector.h"

typedef enum {
    LEVEL_TRACE=0,
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR,
    LEVEL_FATAL
} log_level;

int
init_logger(fd_selector s);

void
destroy_logger();

void 
logger_log(char *log, uint64_t total_bytes, log_level level);


#endif