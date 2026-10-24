#!/bin/sh
# Compresse puis décompresse plusieurs fichiers et vérifie qu'on retrouve l'original octet par octet.
set -e
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

: > "$tmp/vide"
printf 'aaaaaaaaaa' > "$tmp/un_symbole"
printf 'abracadabra' > "$tmp/abracadabra"
cat src/*.c > "$tmp/code_source"
head -c 200000 /dev/urandom > "$tmp/aleatoire"
seq 1 50000 > "$tmp/nombres"

for f in "$tmp"/*; do
    ./huffman c "$f" "$f.huf" > /dev/null
    ./huffman d "$f.huf" "$f.out"
    cmp "$f" "$f.out"
    echo "ok: $(basename "$f") ($(wc -c < "$f") -> $(wc -c < "$f.huf") octets)"
done

# Un fichier tronqué ou invalide doit être refusé
head -c 2060 "$tmp/nombres.huf" > "$tmp/tronque.huf"
if ./huffman d "$tmp/tronque.huf" "$tmp/x" 2>/dev/null; then echo "ÉCHEC: fichier tronqué accepté"; exit 1; fi
if ./huffman d "$tmp/abracadabra" "$tmp/x" 2>/dev/null; then echo "ÉCHEC: fichier invalide accepté"; exit 1; fi
echo "ok: fichiers invalides refusés"
echo "Tous les tests passent."
