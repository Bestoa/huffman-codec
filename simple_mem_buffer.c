#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "huffman.h"
#include "simple_mem_buffer.h"

int mem_eof(struct buffer_ops *handle) {
    struct mem_region *pmr = handle->data;
    if (pmr->end == pmr->cur)
        return 1;
    else
        return 0;
}

int mem_read(struct buffer_ops *handle, void *buffer, size_t len) {
    struct mem_region *pmr = handle->data;
    if (len + pmr->cur >= pmr->end)
        len = pmr->end - pmr->cur;
    memcpy(buffer, pmr->data + pmr->cur, len);
    pmr->cur += len;
    return len;
}

int mem_write(struct buffer_ops *handle, void *data , size_t len) {
    struct mem_region *pmr = handle->data;
    if (len + pmr->cur >= pmr->end)
        len = pmr->end - pmr->cur;
    memcpy(pmr->data + pmr->cur, data, len);
    pmr->cur += len;
    return len;
}

int mem_rewind(struct buffer_ops *handle) {
    struct mem_region *pmr = handle->data;
    pmr->cur = 0;
    return 0;
}

struct buffer_ops * create_mem_buffer_ops(void *buffer, size_t len) {
    struct buffer_ops *ops = malloc(sizeof(struct buffer_ops));
    struct mem_region *pmr = malloc(sizeof(struct mem_region));
    pmr->data = buffer;
    pmr->cur = 0;
    pmr->end = len;
    ops->data = pmr;
    ops->eof = mem_eof;
    ops->read = mem_read;
    ops->write = mem_write;
    ops->rewind = mem_rewind;
    return ops;
}

void desotry_mem_buffer_ops(struct buffer_ops *ops) {
    free(ops);
}
