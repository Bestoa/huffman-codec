#include <stdio.h>
#include <stdlib.h>
#include "huffman.h"

int file_eof(struct buffer_ops *handle) {
    FILE *fp = handle->data;
    return feof(fp);
}

int file_read(struct buffer_ops *handle, void *buffer, size_t len) {
    FILE *fp = handle->data;
    int ret = 0, c;
    if (len == 1) {
        c = fgetc(fp);
        *(int *)buffer = c;
        if (c == EOF)
            ret = 0;
        else
            ret = 1;
    } else {
        ret = fread(buffer, 1, len, fp);
    }
    return ret;
}

int file_write(struct buffer_ops *handle, void *data, size_t len) {
    FILE *fp = handle->data;
    int ret = 0, c;
    if (len == 1) {
        c = fputc(*(char *)data, fp);
        if (c == EOF)
            ret = 0;
        else
            ret = 1;
    } else {
        ret = fwrite(data, 1, len, fp);
    }
    return ret;
}

int file_rewind(struct buffer_ops *handle) {
    FILE *fp = handle->data;
    rewind(fp);
    return 0;
}

struct buffer_ops * create_file_buffer_ops(const char *file_name, const char *mode) {
    struct buffer_ops *ops = malloc(sizeof(struct buffer_ops));
    FILE *fp;
    if (!ops)
        return ops;

    if (!file_name)
        fp = stdout;
    else
        fp = fopen(file_name, mode);
    if (!fp)
        goto FREE_OPS;
    ops->data = fp;
    ops->eof = file_eof;
    ops->read = file_read;
    ops->write = file_write;
    ops->rewind = file_rewind;
    return ops;
FREE_OPS:
    free(ops);
    return NULL;
}

void desotry_file_buffer_ops(struct buffer_ops *ops) {
    if (ops->data && ops->data != stderr && ops->data != stdin && ops->data != stdout)
        fclose(ops->data);
    free(ops);
}

