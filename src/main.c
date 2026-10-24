#include <stdio.h>
#include <string.h>

#include "huffman.h"

static long file_size(FILE *f) {
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, pos, SEEK_SET);
    return size;
}

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[1], "c") != 0 && strcmp(argv[1], "d") != 0)) {
        fprintf(stderr, "Usage : %s c|d <entree> <sortie>\n  c = compresser, d = decompresser\n", argv[0]);
        return 2;
    }
    FILE *in = fopen(argv[2], "rb");
    if (!in) { perror(argv[2]); return 1; }
    FILE *out = fopen(argv[3], "wb");
    if (!out) { perror(argv[3]); fclose(in); return 1; }

    int compress = argv[1][0] == 'c';
    int rc = compress ? huff_compress(in, out) : huff_decompress(in, out);
    long in_size = file_size(in);
    fclose(in);
    long out_size = file_size(out);
    if (fclose(out) != 0) rc = -1;

    if (rc != 0) {
        fprintf(stderr, "Erreur : %s\n", compress ? "compression impossible" : "fichier compresse invalide ou tronque");
        remove(argv[3]);
        return 1;
    }
    if (compress && in_size > 0)
        printf("%ld -> %ld octets (%.1f%%)\n", in_size, out_size, 100.0 * out_size / in_size);
    return 0;
}
