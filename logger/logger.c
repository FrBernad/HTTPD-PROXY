#include "logger.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "utils/buffer/buffer.h"

#define BUFFER_SIZE 1024

static fd_selector selector;

static fd_handler logger_handler;

static buffer stdout_buffer;
static buffer stderr_buffer;

static void
logger_on_write_ready(struct selector_key *key);

int init_logger(fd_selector s) {
    selector = s;

    uint8_t *stdout = malloc(BUFFER_SIZE * sizeof(uint8_t));
    if (stdout == NULL) {
        return -1;
    }

    buffer_init(&stdout_buffer, BUFFER_SIZE, stdout);

    uint8_t *stderr = malloc(BUFFER_SIZE * sizeof(uint8_t));
    if (stderr == NULL) {
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

void logger_log(char *log, uint64_t totalBytes) {
    int fd;
    buffer *buff;

    fd = STDOUT_FILENO;
    buff = &stdout_buffer;

    ssize_t bytesWritten = write(fd, log, totalBytes);
    if (bytesWritten > 0) {
        totalBytes -= bytesWritten;
        log += bytesWritten;
    }

    if (bytesWritten == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        size_t maxBytes;
        uint8_t *data = buffer_write_ptr(buff, &maxBytes);

        if (maxBytes < totalBytes) {
            return;
        }

        memcpy(data, log, totalBytes);

        buffer_write_adv(buff, totalBytes);

        selector_set_interest(selector, fd, OP_WRITE);
    }
}

static void
logger_on_write_ready(struct selector_key *key) {
    buffer *buff = (buffer *)key->data;

    size_t maxBytes;
    ssize_t totalBytes;
    uint8_t *data = buffer_read_ptr(buff, &maxBytes);

    totalBytes = write(key->fd, data, maxBytes);

    if (totalBytes < 0) {
        selector_set_interest_key(key, OP_NOOP);
        return;
    }

    buffer_read_adv(buff, totalBytes);

    if (!buffer_can_read(buff)) {
        selector_set_interest_key(key, OP_NOOP);
    }
}
