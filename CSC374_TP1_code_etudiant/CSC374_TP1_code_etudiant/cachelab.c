/* cachelab.c : fonctions communes du TP1 (CSC 374). Ne pas modifier. */
#include <stdio.h>
#include "cachelab.h"

void afficher_bilan(unsigned long succes, unsigned long defauts, unsigned long evictions)
{
    printf("hits:%lu misses:%lu evictions:%lu\n", succes, defauts, evictions);
    FILE *f = fopen(".csim_resultats", "w");
    if (f != NULL) {
        fprintf(f, "%lu %lu %lu\n", succes, defauts, evictions);
        fclose(f);
    }
}

int est_transposee(int M, int N, int A[N][M], int B[M][N])
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            if (B[j][i] != A[i][j])
                return 0;
    return 1;
}
