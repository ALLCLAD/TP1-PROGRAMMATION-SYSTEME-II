/* tracegen.c : banc d'essai de la partie B du TP1 (CSC 374). Ne pas modifier.
 * Usage : ./tracegen M N    (A a N lignes et M colonnes, B a M lignes et N colonnes)
 * Remplit A, appelle transposer_soumission entre deux écritures dans marqueur,
 * vérifie le résultat, puis écrit sur la sortie d'erreur les adresses dont test-trans.py
 * a besoin pour ne garder que les accès de la fonction aux tableaux A et B.
 * Code de retour : 0 si B est la transposée de A, 1 sinon, 2 si les arguments sont invalides. */
#include <stdio.h>
#include <stdlib.h>
#include "cachelab.h"

void transposer_soumission(int M, int N, int A[N][M], int B[M][N]);

volatile int marqueur;
static struct {
    int A[256][256];
    int B[256][256];          /* B commence 256 Kio après A : A[i][j] et B[i][j] ont le même indice */
} m __attribute__((aligned(4096)));

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage : %s M N\n", argv[0]); return 2; }
    int M = atoi(argv[1]), N = atoi(argv[2]);
    if (M < 1 || M > 256 || N < 1 || N > 256) { fprintf(stderr, "dimensions entre 1 et 256\n"); return 2; }
    int (*A)[M] = (int (*)[M]) m.A;
    int (*B)[N] = (int (*)[N]) m.B;
    srand(374);
    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            A[i][j] = rand();
    marqueur = 1;
    transposer_soumission(M, N, A, B);
    marqueur = 2;
    int ok = est_transposee(M, N, A, B);
    fprintf(stderr, "marqueur %p zone %p %zu\n", (void *) &marqueur, (void *) &m, sizeof m);
    printf("%dx%d : %s\n", N, M, ok ? "transposition correcte" : "transposition INCORRECTE");
    return ok ? 0 : 1;
}
