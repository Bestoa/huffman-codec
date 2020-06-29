#ifndef __HUNFFMAN_H__
#define __HUNFFMAN_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define ZAP(x, len) (memset((x), 0, (len)))
#define SETB(x, b) ((x) |= ((1) << (b)))
#define RSETB(x, b) ((x) &= (~((1) << (b))))
#define GETB(x, b) ((x) & (1 << (b)))

#define TABLE_SIZE (256)

struct huffman_node {
    uint64_t weight;
    uint8_t value;
    struct huffman_node *left;
    struct huffman_node *right;
    /* record the old root node here */
    struct huffman_node *__free_handle;
};

struct huffman_code {
    /* Code len should be less than table len */
    uint8_t code[TABLE_SIZE];
    uint8_t length;
};

struct huffman_file_header {
    char magic[8];
    uint64_t file_size;
    uint32_t table_size;
};

struct buffer_ops {
    void *data;
    int (*eof)(struct buffer_ops *handle);
    int (*read)(struct buffer_ops *handle, void *buffer, size_t);
    int (*write)(struct buffer_ops *handle, void *data, size_t);
    int (*rewind)(struct buffer_ops *handle);
    void *priv_data;
};

int encode(struct buffer_ops *in, struct buffer_ops *out);
int decode(struct buffer_ops *in, struct buffer_ops *out);

// Low 8 bits store the key, high 56 bits store the weight
#define GEN_TABLE_UNIT(d, f) ((d) | ((f) << 8))

#define LOGE(msg, ...) fprintf(stderr, msg, ## __VA_ARGS__)
#define LOGI(msg, ...) fprintf(stderr, msg, ## __VA_ARGS__)

#define MAGIC ("HUFFMAN")
#if defined(DEBUG)
#define INDENT (8)
#endif

#endif
