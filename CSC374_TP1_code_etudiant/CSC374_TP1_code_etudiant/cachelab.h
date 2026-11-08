/* cachelab.h : fonctions communes du TP1 (CSC 374). Ne pas modifier. */
#ifndef CACHELAB_H
#define CACHELAB_H

/* Partie A : à appeler une fois, à la fin de csim, avec les trois compteurs.
 * Écrit le bilan sur la sortie standard et dans le fichier .csim_resultats,
 * que lit l'outil de test. */
void afficher_bilan(unsigned long succes, unsigned long defauts, unsigned long evictions);

/* Partie B : vrai si B est la transposée de A (B[j][i] == A[i][j] pour tout i, j). */
int est_transposee(int M, int N, int A[N][M], int B[M][N]);

#endif
