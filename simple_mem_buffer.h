#ifndef __SIMPLE_MEM_BUFFER__
#include "huffman.h"
struct mem_region {
    size_t cur;
    size_t end;
    void *data;
};
struct buffer_ops * create_mem_buffer_ops(void *buffer, size_t len);
void desotry_mem_buffer_ops(struct buffer_ops *ops);
#endif
