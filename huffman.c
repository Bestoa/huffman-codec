#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "huffman.h"

#if defined(DEBUG)
#define INDENT (8)
void dump_huffman_tree(struct huffman_node *root, int space)
{
    int i = 0;
    if (root->right)
        dump_huffman_tree(root->right, space + INDENT);
    for (i = 0; i < space; i++)
        LOGI(" ");
    if (!root->left)
        LOGI("[%d:%ld]\n", root->value, root->weight);
    else
        LOGI("R:%ld\n", root->weight);
    if (root->left)
        dump_huffman_tree(root->left, space + INDENT);
}

void dump_huffman_code_list() {
    int i = 0, j = 0;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (huffman_code_list[i].length != 0) {
            LOGI("value = %d, length = %d code = ", i, huffman_code_list[i].length);
            for (j = 0; j < huffman_code_list[i].length; j++) {
                LOGI("%d", huffman_code_list[i].code[j] ? 1 : 0);
            }
            putchar(10);
        }
    }
}
#endif

struct huffman_code huffman_code_list[TABLE_SIZE];

int table_unit_compar(const void *a, const void *b) {
    uint64_t x = *(uint64_t *)a;
    uint64_t y = *(uint64_t *)b;
    if (x > y)
        return 1;
    else if (x < y)
        return -1;
    else
        return 0;
}

void clean_huffman_code_list() {
    ZAP(&huffman_code_list);
}

void generate_huffman_code_recusive(struct huffman_node *tree, char *code, int len) {
    if (!tree->left) {
        memcpy(huffman_code_list[tree->value].code, code, len);
        huffman_code_list[tree->value].length = len;
        return;
    }
    generate_huffman_code_recusive(tree->left, code, len + 1);
    code[len] = 1;
    generate_huffman_code_recusive(tree->right, code, len + 1);
    code[len] = 0;
}

void generate_huffman_code(struct huffman_node *tree) {
    char code[TABLE_SIZE] = { 0 };
    generate_huffman_code_recusive(tree, code, 0);
}

struct huffman_node * build_huffman_tree(uint64_t *table, int size) {

    int i = 0, j = 0, first_unused = 0;
    struct huffman_node *p_huffman_node_list[TABLE_SIZE];
    // Max node number is 2 * (table size) - 1
    static struct huffman_node huffman_node_list[TABLE_SIZE * 2];

    ZAP(huffman_node_list);

    for (i = 0; i < size; i++) {
        p_huffman_node_list[i] = &huffman_node_list[first_unused++];
        p_huffman_node_list[i]->weight = table[i] >> 8;
        p_huffman_node_list[i]->value = table[i] & 0xff;
    }

    for (i = 0; i < size - 1; i++) {
        struct huffman_node *p_node = &huffman_node_list[first_unused++];
        p_node->weight = p_huffman_node_list[i]->weight + p_huffman_node_list[i + 1]->weight;
        p_node->left = p_huffman_node_list[i];
        p_node->right = p_huffman_node_list[i + 1];
        p_huffman_node_list[i + 1] = p_node;
        p_huffman_node_list[i] = NULL;
        // Ensure the list is in order
        for (j = i + 1; j < size - 1; j++) {
            if (p_huffman_node_list[j]->weight > p_huffman_node_list[j + 1]->weight) {
                struct huffman_node *node = p_huffman_node_list[j + 1];
                p_huffman_node_list[j + 1] = p_huffman_node_list[j];
                p_huffman_node_list[j] = node;
            } else {
                break;
            }
        }
    }
    return p_huffman_node_list[size - 1];
}

int file_eof(void *handle) {
    return feof((FILE *)handle);
}

int file_read(void *handle, void *buffer, size_t len) {
    int ret = 0, c;
    if (len == 1) {
        c = fgetc((FILE *)handle);
        *(int *)buffer = c;
        if (c == EOF)
            ret = 0;
        else
            ret = 1;
    } else {
        ret = fread(buffer, 1, len, (FILE *)handle);
    }
    return ret;
}

int file_write(void *handle, void *data, size_t len) {
    int ret = 0, c;
    if (len == 1) {
        c = fputc(*(char *)data, (FILE *)handle);
        if (c == EOF)
            ret = 0;
        else
            ret = 1;
    } else {
        ret = fwrite(data, 1, len, (FILE *)handle);
    }
    return ret;
}

int file_rewind(void *handle) {
    rewind((FILE *)handle);
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
    ops->handle = fp;
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
    if (ops->handle && ops->handle != stderr && ops->handle != stdin)
        fclose(ops->handle);
    free(ops);
}


int encode(struct buffer_ops *in, struct buffer_ops *out) {

    int c;
    uint8_t cached_c = 0, used_bits = 0;
    uint64_t table[TABLE_SIZE] = { 0 };
    struct huffman_node *tree = NULL;
    struct huffman_file_header fh;
    void * hin, *hout;

    hin = in->handle;
    hout = out->handle;

    while(!in->eof(hin)) {
        in->read(hin, &c, 1);
        table[c]++;
    }
    in->rewind(hin);

    for (c = 0; c < TABLE_SIZE; c++) {
        table[c] = GEN_TABLE_UNIT(c, table[c]);
    }
    qsort(table, TABLE_SIZE, sizeof(uint64_t), table_unit_compar);
    // Find the first non-zero value.
    for (c = 0; c < TABLE_SIZE; c++)
        if (table[c] >> 8)
            break;
    // Need 2 units at least
    if (c > TABLE_SIZE - 2) {
        c = TABLE_SIZE - 2;
    }

    tree = build_huffman_tree(table + c, TABLE_SIZE - c);
#ifdef DEBUG
    dump_huffman_tree(tree, 0);
#endif
    clean_huffman_code_list();
    generate_huffman_code(tree);
#ifdef DEBUG
    dump_huffman_code_list();
#endif

    ZAP(&fh);
    memcpy(fh.magic, MAGIC, sizeof(MAGIC));
    fh.file_size = tree->weight;
    LOGI("File size = %lu\n", tree->weight);
    fh.table_size = TABLE_SIZE - c;
    LOGI("Table size = %u\n", TABLE_SIZE - c);
    out->write(hout, &fh, sizeof(struct huffman_file_header));
    out->write(hout, table + c, (TABLE_SIZE - c) * sizeof(uint64_t));

    while (!in->eof(hin)) {
        int i = 0;
        in->read(hin, &c, 1);
        for (i = 0; i < huffman_code_list[c].length; i++) {
            if (huffman_code_list[c].code[i]) {
                SETB(cached_c, 7 - used_bits);
            }
            used_bits++;
            if (used_bits == 8) {
                out->write(hout, &cached_c, 1);
                used_bits = 0;
                cached_c = 0;
            }
        }
    }
    if (used_bits)
        out->write(hout, &cached_c, 1);

    return 0;
}

int file_encode(const char *input_file, const char *output_file) {

    struct buffer_ops *in, *out;

    in = create_file_buffer_ops(input_file, "rb");
    if (!in) {
        LOGE("Create input file ops failed\n");
        return 1;
    }
    out = create_file_buffer_ops(output_file, "wb");
    if (!out) {
        LOGE("Create input file ops failed\n");
        goto OUT;
    }
    encode(in, out);
    desotry_file_buffer_ops(out);
OUT:
    desotry_file_buffer_ops(in);
    return 0;
}

int decode(struct buffer_ops *in, struct buffer_ops *out) {
    int cached_c = 0, used_bits = 0;
    void * hin, *hout;
    uint64_t table[TABLE_SIZE] = { 0 };
    uint64_t size = 0;
    struct huffman_file_header fh;
    struct huffman_node *tree = NULL,*walk;

    hin = in->handle;
    hout = out->handle;

    if (in->read(hin, &fh, sizeof(fh)) != sizeof(fh)) {
        LOGE("Read file header failed.\n");
        return 1;
    }
    if (strcmp(fh.magic, MAGIC)) {
        LOGE("Miss magic number, abort.\n");
        return 1;
    }
    if (in->read(hin, table, sizeof(uint64_t) * fh.table_size) != fh.table_size * sizeof(uint64_t)) {
        LOGE("Read table failed.\n");
        return 1;
    }

    if (fh.table_size > TABLE_SIZE) {
        LOGE("Table size is invalid.\n");
        return 1;
    }

    tree = build_huffman_tree(table, fh.table_size);
#ifdef DEBUG
    dump_huffman_tree(tree, 0);
#endif

    walk = tree;
    while (size < fh.file_size) {
        if (!used_bits) {
            if (in->eof(hin)) {
                LOGE("Unexpect file end, size = %lu\n", size);
                return 1;
            }
            in->read(hin, &cached_c, 1);
        }

        if (GETB(cached_c, 7 - used_bits))
            walk = walk->right;
        else
            walk = walk->left;

        used_bits++;
        if (used_bits == 8)
            used_bits = 0;

        if (!walk->left) {
            out->write(hout, &walk->value, 1);
            walk = tree;
            size++;
        }
    }
    return 0;
}

int file_decode(const char *input_file, const char *output_file) {

    struct buffer_ops *in, *out;

    in = create_file_buffer_ops(input_file, "rb");
    if (!in) {
        LOGE("Create input file ops failed\n");
        return 1;
    }
    out = create_file_buffer_ops(output_file, "wb");
    if (!out) {
        LOGE("Create input file ops failed\n");
        goto OUT;
    }
    decode(in, out);
    desotry_file_buffer_ops(out);
OUT:
    desotry_file_buffer_ops(in);
    return 0;
}

int main(int argc, char *argv[]) {
    char *input, *output;
    if (argc < 3) {
        LOGE("Miss args.\n");
        LOGE("Usage: -d/-e input [output]\n");
        return 1;
    }
    input = argv[2];
    output = NULL;
    if (argc > 3) {
        output = argv[3];
    }
    if (!strcmp(argv[1], "-e")) {
        file_encode(input, output);
    } else if (!strcmp(argv[1], "-d")) {
        file_decode(input, output);
    } else {
        LOGE("Unknown action.\n");
    }
    return 0;
}
