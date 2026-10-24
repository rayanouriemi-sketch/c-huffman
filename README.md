# Compresseur Huffman en C

Compression et décompression de fichiers avec le **codage de Huffman**, l'algorithme utilisé (entre autres) dans ZIP, JPEG et MP3.

## Principe

1. On compte la fréquence de chaque octet du fichier.
2. On construit un arbre binaire en fusionnant à chaque fois les deux noeuds les moins fréquents.
3. Chaque octet reçoit un code binaire : les octets fréquents ont des codes courts.
4. On écrit l'en-tête (table des fréquences) puis le flux de bits.

Le décompresseur reconstruit exactement le même arbre à partir des fréquences.

## Utilisation

```bash
make
./huffman c livre.txt livre.huf    # compresser
./huffman d livre.huf livre2.txt   # décompresser
```

## Format du fichier .huf

| Octets | Contenu |
|--------|---------|
| 4      | HUF1 (nombre magique) |
| 256 x 8 | fréquence de chaque octet (uint64 little-endian) |
| reste  | flux de bits, bit de poids fort en premier |

## Tests

```bash
make test
```

Les tests vérifient l'aller-retour sur un fichier vide, un seul symbole, du texte, des données aléatoires, ainsi que le refus des fichiers tronqués.

## Limites

L'en-tête fait 2 Ko : la compression n'est rentable que sur des fichiers de quelques Ko ou plus.
