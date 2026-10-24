#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>

/* Retournent 0 en cas de succès, -1 en cas d'erreur.
 * huff_compress lit l'entrée deux fois : `in` doit être un fichier (pas un pipe). */
int huff_compress(FILE *in, FILE *out);
int huff_decompress(FILE *in, FILE *out);

#endif
