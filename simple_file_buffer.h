#ifndef __SIMPLE_FILE_BUFFER__
#include "huffman.h"
struct buffer_ops * create_file_buffer_ops(const char *file_name, const char *mode);
void desotry_file_buffer_ops(struct buffer_ops *ops);
#endif
