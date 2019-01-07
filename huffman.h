#ifndef __HUNFFMAN_H__
#define __HUNFFMAN_H__

#define ZAP(x) (memset((x), 0, sizeof(*(x))))
#define SETB(x, b) ((x) |= ((1) << (b)))
#define RSETB(x, b) ((x) &= (~((1) << (b))))
#define GETB(x, b) ((x) & (1 << (b)))

#define TABLE_SIZE (256)

struct huffman_node {
    uint64_t weight;
    uint8_t value;
    struct huffman_node *left;
    struct huffman_node *right;
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

// Low 8 bits store the key, high 56 bits store the weight
#define GEN_TABLE_UNIT(d, f) ((d) | ((f) << 8))

#define LOGE(msg, ...) fprintf(stderr, msg, ## __VA_ARGS__)
#define LOGI(msg, ...) fprintf(stderr, msg, ## __VA_ARGS__)

#define MAGIC ("HUFFMAN")
#if defined(DEBUG)
#define INDENT (8)
#endif

#endif
