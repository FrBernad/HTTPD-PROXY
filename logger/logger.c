#include "logger.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "utils/buffer/buffer.h"

#define BUFFER_SIZE 1024

#define IS_STDOUT_LEVEL(l) ((l) >= LEVEL_TRACE && (l) <= LEVEL_WARN)
#define IS_STDERR_LEVEL(l) ((l) <= LEVEL_FATAL && (l) >= LEVEL_ERROR)

static fd_selector selector;

static fd_handler logger_handler;

static buffer stdout_buffer;
static buffer stderr_buffer;

static void
logger_on_write_ready(struct selector_key *key);

int 
init_logger(fd_selector s) {
    selector = s;

    uint8_t *stdout = malloc(BUFFER_SIZE * sizeof(uint8_t));
    if (stdout == NULL) {
        return -1;
    }

    buffer_init(&stdout_buffer, BUFFER_SIZE, stdout);

    uint8_t *stderr = malloc(BUFFER_SIZE * sizeof(uint8_t));
    if (stderr == NULL) {
        free(stdout);
        return -1;
    }

    buffer_init(&stderr_buffer, BUFFER_SIZE, stderr);

    selector_fd_set_nio(STDOUT_FILENO);
    selector_fd_set_nio(STDERR_FILENO);

    logger_handler.handle_read = NULL;
    logger_handler.handle_write = logger_on_write_ready;
    logger_handler.handle_close = NULL;
    logger_handler.handle_block = NULL;

    selector_register(selector, STDOUT_FILENO, &logger_handler, OP_NOOP, &stdout_buffer);
    selector_register(selector, STDERR_FILENO, &logger_handler, OP_NOOP, &stderr_buffer);

    return 1;
}

void 
destroy_logger() {
    if (stdout_buffer.data != NULL) {
        free(stdout_buffer.data);
    }

    if (stderr_buffer.data != NULL) {
        free(stderr_buffer.data);
    }
}

void 
logger_log(char *log, uint64_t total_bytes, log_level level) {   

    int fd;
    buffer *buff;

    if(IS_STDOUT_LEVEL(level)){
        fd = STDOUT_FILENO;
        buff = &stdout_buffer;
        
    }else if (IS_STDERR_LEVEL(level)){
        fd = STDERR_FILENO;
        buff = &stderr_buffer;   
    }
    else{
        return;
    }

    ssize_t bytes_written = write(fd, log, total_bytes);
    if (bytes_written > 0) {
        total_bytes -= bytes_written;
        log += bytes_written;
    }

    if (bytes_written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        size_t max_bytes;
        uint8_t *data = buffer_write_ptr(buff, &max_bytes);

        if (max_bytes < total_bytes) {
            return;
        }

        memcpy(data, log, total_bytes);

        buffer_write_adv(buff, total_bytes);

        selector_set_interest(selector, fd, OP_WRITE);
    }
}

static void
logger_on_write_ready(struct selector_key *key) {
    buffer *buff = (buffer *)key->data;

    size_t max_bytes;
    ssize_t total_bytes;
    uint8_t *data = buffer_read_ptr(buff, &max_bytes);

    total_bytes = write(key->fd, data, max_bytes);

    if (total_bytes < 0) {
        selector_set_interest_key(key, OP_NOOP);
        return;
    }

    buffer_read_adv(buff, total_bytes);

    if (!buffer_can_read(buff)) {
        selector_set_interest_key(key, OP_NOOP);
    }
}
