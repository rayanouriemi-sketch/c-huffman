#include "huffman.h"

#include <stdint.h>
#include <string.h>

#define SYMBOLS 256
#define MAX_NODES (2 * SYMBOLS - 1)
#define MAGIC "HUF1"
#define MAGIC_LEN 4
#define CHUNK 65536

typedef struct {
    uint64_t freq;
    int symbol; /* -1 pour un noeud interne */
    int left, right;
} Node;

typedef struct {
    unsigned char bits[SYMBOLS][SYMBOLS];
    int len[SYMBOLS];
} CodeTable;

/* Construit l'arbre dans pool et retourne l'indice de la racine (-1 si aucun symbole).
 * Le choix des deux minimums est déterministe : le décompresseur reconstruit le même arbre. */
static int build_tree(const uint64_t freq[SYMBOLS], Node pool[MAX_NODES]) {
    int active[SYMBOLS], n_active = 0, n_nodes = 0;
    for (int s = 0; s < SYMBOLS; s++) {
        if (freq[s] == 0) continue;
        pool[n_nodes] = (Node){freq[s], s, -1, -1};
        active[n_active++] = n_nodes++;
    }
    if (n_active == 0) return -1;

    while (n_active > 1) {
        int a = 0, b = 1; /* positions dans active des deux plus petits */
        if (pool[active[b]].freq < pool[active[a]].freq) { a = 1; b = 0; }
        for (int i = 2; i < n_active; i++) {
            uint64_t f = pool[active[i]].freq;
            if (f < pool[active[a]].freq) { b = a; a = i; }
            else if (f < pool[active[b]].freq) b = i;
        }
        pool[n_nodes] = (Node){pool[active[a]].freq + pool[active[b]].freq, -1, active[a], active[b]};
        int hi = a > b ? a : b, lo = a < b ? a : b;
        active[lo] = n_nodes++;
        active[hi] = active[--n_active];
    }
    return active[0];
}

static void assign_codes(const Node *pool, int idx, unsigned char *path, int depth, CodeTable *t) {
    const Node *n = &pool[idx];
    if (n->symbol >= 0) {
        int len = depth > 0 ? depth : 1; /* un seul symbole : code 0 sur 1 bit */
        memcpy(t->bits[n->symbol], path, (size_t)len);
        t->len[n->symbol] = len;
        return;
    }
    path[depth] = 0;
    assign_codes(pool, n->left, path, depth + 1, t);
    path[depth] = 1;
    assign_codes(pool, n->right, path, depth + 1, t);
}

static int write_u64(FILE *out, uint64_t v) {
    unsigned char b[8];
    for (int i = 0; i < 8; i++) b[i] = (unsigned char)(v >> (8 * i));
    return fwrite(b, 1, 8, out) == 8 ? 0 : -1;
}

static int read_u64(FILE *in, uint64_t *v) {
    unsigned char b[8];
    if (fread(b, 1, 8, in) != 8) return -1;
    *v = 0;
    for (int i = 0; i < 8; i++) *v |= (uint64_t)b[i] << (8 * i);
    return 0;
}

int huff_compress(FILE *in, FILE *out) {
    static unsigned char buf[CHUNK];
    static Node pool[MAX_NODES];
    static CodeTable table;
    unsigned char path[SYMBOLS] = {0};
    uint64_t freq[SYMBOLS] = {0};
    size_t n;

    while ((n = fread(buf, 1, CHUNK, in)) > 0)
        for (size_t i = 0; i < n; i++) freq[buf[i]]++;
    if (ferror(in) || fseek(in, 0, SEEK_SET) != 0) return -1;

    if (fwrite(MAGIC, 1, MAGIC_LEN, out) != MAGIC_LEN) return -1;
    for (int s = 0; s < SYMBOLS; s++)
        if (write_u64(out, freq[s]) != 0) return -1;

    int root = build_tree(freq, pool);
    if (root < 0) return 0; /* fichier vide */
    memset(&table, 0, sizeof table);
    assign_codes(pool, root, path, 0, &table);

    unsigned char byte = 0;
    int filled = 0;
    while ((n = fread(buf, 1, CHUNK, in)) > 0) {
        for (size_t i = 0; i < n; i++) {
            for (int k = 0; k < table.len[buf[i]]; k++) {
                byte = (unsigned char)((byte << 1) | table.bits[buf[i]][k]);
                if (++filled == 8) {
                    if (fputc(byte, out) == EOF) return -1;
                    byte = 0;
                    filled = 0;
                }
            }
        }
    }
    if (ferror(in)) return -1;
    if (filled > 0 && fputc(byte << (8 - filled), out) == EOF) return -1;
    return 0;
}

int huff_decompress(FILE *in, FILE *out) {
    static Node pool[MAX_NODES];
    char magic[MAGIC_LEN];
    uint64_t freq[SYMBOLS], total = 0;

    if (fread(magic, 1, MAGIC_LEN, in) != MAGIC_LEN || memcmp(magic, MAGIC, MAGIC_LEN) != 0) return -1;
    for (int s = 0; s < SYMBOLS; s++) {
        if (read_u64(in, &freq[s]) != 0) return -1;
        total += freq[s];
    }

    int root = build_tree(freq, pool);
    if (root < 0) return 0;
    if (pool[root].symbol >= 0) { /* un seul symbole répété */
        for (uint64_t i = 0; i < total; i++)
            if (fputc(pool[root].symbol, out) == EOF) return -1;
        return 0;
    }

    uint64_t written = 0;
    int node = root, c;
    while (written < total && (c = fgetc(in)) != EOF) {
        for (int bit = 7; bit >= 0 && written < total; bit--) {
            node = (c >> bit) & 1 ? pool[node].right : pool[node].left;
            if (pool[node].symbol >= 0) {
                if (fputc(pool[node].symbol, out) == EOF) return -1;
                written++;
                node = root;
            }
        }
    }
    return written == total ? 0 : -1; /* sinon : fichier tronqué */
}
