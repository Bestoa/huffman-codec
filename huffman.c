#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

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
}huffman_code_list[TABLE_SIZE];

#if defined(DEBUG)
#define INDENT (8)
void dump_huffman_tree(struct huffman_node *root, int space)
{
    int i = 0;
    if (root->right)
        dump_huffman_tree(root->right, space + INDENT);
    for (i = 0; i < space; i++)
        printf(" ");
    if (!root->left)
        printf("[%d:%ld]\n", root->value, root->weight);
    else
        printf("R:%ld\n", root->weight);
    if (root->left)
        dump_huffman_tree(root->left, space + INDENT);
}

void dump_huffman_code_list() {
    int i = 0, j = 0;
    for (i = 0; i < TABLE_SIZE; i++) {
        if (huffman_code_list[i].length != 0) {
            printf("value = %d, length = %d code = ", i, huffman_code_list[i].length);
            for (j = 0; j < huffman_code_list[i].length; j++) {
                printf("%d", huffman_code_list[i].code[j] ? 1 : 0);
            }
            putchar(10);
        }
    }
}
#endif

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

void free_huffman_tree(struct huffman_node *root) {
    if (root->left)
        free_huffman_tree(root->left);
    if(root->right)
        free_huffman_tree(root->right);
    free(root);
}

struct huffman_node * build_huffman_tree(uint64_t *table, int size) {
    struct huffman_node *huffman_node_list[size];
    int i = 0, j = 0;
    for (i = 0; i < size; i++) {
        huffman_node_list[i] = malloc(sizeof(struct huffman_node));
        if (!huffman_node_list[i]) {
            printf("OOM\n");
            while(--i > 0) {
                free(huffman_node_list[i]);
            }
            return NULL;
        }
        ZAP(huffman_node_list[i]);
        huffman_node_list[i]->weight = table[i] >> 8;
        huffman_node_list[i]->value = table[i] & 0xff;
    }
    for (i = 0; i < size - 1; i++) {
        struct huffman_node *hn = malloc(sizeof(struct huffman_node));
        if (!hn) {
            printf("OOM\n");
            while (i < size) {
                free_huffman_tree(huffman_node_list[i]);
                i++;
            }
            return NULL;
        }
        hn->weight = huffman_node_list[i]->weight + huffman_node_list[i + 1]->weight;
        hn->left = huffman_node_list[i];
        hn->right = huffman_node_list[i + 1];
        huffman_node_list[i + 1] = hn;
        huffman_node_list[i] = NULL;
        for (j = i + 1; j < size -1; j++) {
            if (huffman_node_list[j]->weight > huffman_node_list[j + 1]->weight) {
                struct huffman_node *node = huffman_node_list[j + 1];
                huffman_node_list[j + 1] = huffman_node_list[j];
                huffman_node_list[j] = node;
            } else {
                break;
            }
        }
    }
    return huffman_node_list[size - 1];
}

#define GEN_TABLE_UNIT(d, f) ((d) | ((f) << 8))

struct huffman_file_header {
    char magic[8];
    uint64_t file_size;
    uint32_t table_size;
};

#define MAGIC ("HUFFMAN")
int encode(const char *name) {
    int c, ret = 0;
    FILE *ifp, *ofp;
    uint8_t cached_c = 0, used_bits = 0;
    uint64_t table[TABLE_SIZE] = { 0 };
    struct huffman_node *tree = NULL;
    struct huffman_file_header fh;

    ifp = fopen(name, "r");
    if (!ifp) {
        printf("Open input file %s failed\n", name);
        return 1;
    }

    while(!feof(ifp)) {
        c= fgetc(ifp);
        table[c]++;
    }
    fseek(ifp, 0L, SEEK_SET);

    for (c = 0; c < TABLE_SIZE; c++) {
        table[c] = GEN_TABLE_UNIT(c, table[c]);
    }
    qsort(table, TABLE_SIZE, sizeof(uint64_t), table_unit_compar);
    for (c = 0; c < TABLE_SIZE; c++)
        if (table[c] >> 8)
            break;
    if (c > TABLE_SIZE - 2) {
        c = TABLE_SIZE - 2;
    }

    tree = build_huffman_tree(table + c, TABLE_SIZE - c);
    if (!tree) {
        printf("Create huffman tree failed\n");
        ret = 1;
        goto ERR_CLOSE_INPUT_FILE;
    }
#ifdef DEBUG
    dump_huffman_tree(tree, 0);
#endif
    clean_huffman_code_list();
    generate_huffman_code(tree);
#ifdef DEBUG
    dump_huffman_code_list();
#endif

    ofp = fopen("out.he", "wb");
    if (!ofp) {
        printf("Create file failed.\n");
        ret = 1;
        goto ERR_CLEAN_HUFFMAN_CODE_LIST;
    }

    ZAP(&fh);
    memcpy(fh.magic, MAGIC, sizeof(MAGIC));
    fh.file_size = tree->weight;
    printf("File size = %lu\n", tree->weight);
    fh.table_size = TABLE_SIZE - c;
    printf("Table size = %u\n", TABLE_SIZE - c);
    fwrite(&fh, 1, sizeof(struct huffman_file_header), ofp);
    fwrite(table + c, TABLE_SIZE - c, sizeof(uint64_t), ofp);

    while (!feof(ifp)) {
        int i = 0;
        c = fgetc(ifp);
        for (i = 0; i < huffman_code_list[c].length; i++) {
            if (huffman_code_list[c].code[i]) {
                SETB(cached_c, 7 - used_bits);
            }
            used_bits++;
            if (used_bits == 8) {
                fputc(cached_c, ofp);
                used_bits = 0;
                cached_c = 0;
            }
        }
    }
    if (used_bits)
        fputc(cached_c, ofp);
    fclose(ofp);
ERR_CLEAN_HUFFMAN_CODE_LIST:
    clean_huffman_code_list();
    free_huffman_tree(tree);
ERR_CLOSE_INPUT_FILE:
    fclose(ifp);
    return ret;
}

int decode(const char *name) {
    int ret = 0;
    int cached_c = 0, used_bits = 0;
    FILE *ifp, *ofp;
    uint64_t table[TABLE_SIZE] = { 0 };
    uint64_t size = 0;
    struct huffman_file_header fh;
    struct huffman_node *tree = NULL,*walk;

    ifp = fopen(name, "r");
    if (!ifp) {
        printf("Open input file %s failed.\n", name);
        return 1;
    }

    if (fread(&fh, sizeof(struct huffman_file_header), 1, ifp) != 1) {
        printf("Read file header failed.\n");
        ret = 1;
        goto ERR_CLOSE_INPUT_FILE;
    }
    if (strcmp(fh.magic, MAGIC)) {
        printf("Miss magic number, abort.\n");
        ret = 1;
        goto ERR_CLOSE_INPUT_FILE;
    }
    if (fread(table, sizeof(uint64_t), fh.table_size, ifp) != fh.table_size) {
        printf("Read table failed.\n");
        ret = 1;
        goto ERR_CLOSE_INPUT_FILE;
    }

    tree = build_huffman_tree(table, fh.table_size);
    if (!tree) {
        printf("Build huffman tree failed.\n");
        ret = 1;
        goto ERR_CLOSE_INPUT_FILE;
    }
#ifdef DEBUG
    dump_huffman_tree(tree, 0);
#endif
    ofp = fopen("out.hd", "wb");
    if (!ofp) {
        printf("Create file filed\n");
        ret = 1;
        goto ERR_FREE_HUFFMAN_TREE;
    }
    walk = tree;
    while (size < fh.file_size) {
        if (!used_bits) {
            if (feof(ifp)) {
                printf("Unexpect file end, size = %lu\n", size);
                ret = 1;
                goto ERR_FREE_HUFFMAN_TREE;
            }
            cached_c = fgetc(ifp);
        }

        if (GETB(cached_c, 7 - used_bits))
            walk = walk->right;
        else
            walk = walk->left;

        used_bits++;
        if (used_bits == 8)
            used_bits = 0;

        if (!walk->left) {
            fputc(walk->value, ofp);
            walk = tree;
            size++;
        }
    }
    fclose(ofp);
ERR_FREE_HUFFMAN_TREE:
    free_huffman_tree(tree);
ERR_CLOSE_INPUT_FILE:
    fclose(ifp);
    return ret;
}
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Miss argv.\n");
        return 1;
    }
    if (argv[1][0] == 'e') {
        encode(argv[2]);
    } else if (argv[1][0] == 'd') {
        decode(argv[2]);
    } else {
        printf("Unknown action.\n");
    }
    return 0;
}
