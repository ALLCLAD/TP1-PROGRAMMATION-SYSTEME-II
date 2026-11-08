/* trans.c : transposition de matrice du TP1 (CSC 374), partie B.
 *
 * Nom, prénom :
 * Matricule   :
 *
 * transposer_soumission(M, N, A, B) doit ranger dans B (M lignes, N colonnes) la transposée
 * de A (N lignes, M colonnes), en faisant le moins de défauts possible dans un cache à
 * correspondance directe de 32 ensembles de 32 octets (s = 5, E = 1, b = 5).
 * Tailles évaluées : 32 x 32, 64 x 64, 61 x 67 (M = 61 colonnes, N = 67 lignes).
 *
 * Règles (vérifiées à la lecture du code) :
 * - au plus 12 variables locales de type int dans chaque fonction (paramètres non compris) ;
 * - aucun tableau local, aucune variable globale ou statique, aucune allocation dynamique :
 *   les seules données en mémoire sont A et B ;
 * - pas de récursivité ; A ne doit jamais être modifié ;
 * - on peut écrire des fonctions auxiliaires et choisir une méthode selon M et N.
 */
#include "cachelab.h"

/*
 * ==============================================================================
 * CAS 1 : Matrice 32 x 32
 * ==============================================================================
 * Configuration du cache : 32 ensembles de 32 octets (8 entiers int par bloc).
 * Une ligne de 32 entiers occupe 4 blocs de cache (4 ensembles consécutifs).
 * Le cache contient 32 ensembles = 8 lignes complètes d'une matrice 32x32.
 * Deux lignes distantes de 8 lignes (ex: ligne 0 et ligne 8) entrent en conflit.
 *
 * Stratégie : Découpage par blocs de 8 x 8 (tiling).
 *
 * Problème de la diagonale : A[i][j] et B[j][i] entrent en collision quand i = j
 * car B commence 256 Kio après A (multiple exact de la taille du cache).
 * Solution : On lit une rangée complète de 8 entiers de A dans 8 variables locales
 * d'un coup (dans les registres processeur), puis on écrit ces 8 variables dans B.
 * Cela élimine totalement les conflits d'éviction diagonaux.
 *
 * Nombre de variables locales utilisées : 12 au total (i, j, k, v0..v7).
 */
static void transposer_32x32(int A[32][32], int B[32][32])
{
    int i, j, k;
    int v0, v1, v2, v3, v4, v5, v6, v7;

    for (i = 0; i < 32; i += 8) {
        for (j = 0; j < 32; j += 8) {
            for (k = 0; k < 8; k++) {
                /* Lecture d'une rangée complète de 8 entiers de A */
                v0 = A[i + k][j + 0];
                v1 = A[i + k][j + 1];
                v2 = A[i + k][j + 2];
                v3 = A[i + k][j + 3];
                v4 = A[i + k][j + 4];
                v5 = A[i + k][j + 5];
                v6 = A[i + k][j + 6];
                v7 = A[i + k][j + 7];

                /* Écriture transposée dans B */
                B[j + 0][i + k] = v0;
                B[j + 1][i + k] = v1;
                B[j + 2][i + k] = v2;
                B[j + 3][i + k] = v3;
                B[j + 4][i + k] = v4;
                B[j + 5][i + k] = v5;
                B[j + 6][i + k] = v6;
                B[j + 7][i + k] = v7;
            }
        }
    }
}

/*
 * ==============================================================================
 * CAS 2 : Matrice 64 x 64
 * ==============================================================================
 * Une ligne de 64 entiers occupe 8 blocs de cache (8 ensembles).
 * Le cache ne peut contenir que 4 lignes entières (32 / 8 = 4).
 * Ainsi, deux lignes distantes de 4 lignes entrent en conflit direct !
 * Un bloc 8x8 direct subit un désastre de conflits entre ses 4 premières lignes
 * et ses 4 suivantes.
 *
 * Stratégie : Macro-blocs de 8 x 8 découpés en sous-blocs de 4 x 4 :
 *   1. On charge 4 lignes de A (8 entiers chacune) :
 *      - Les 4 entiers de gauche vont dans leur position transposée dans B (haut-gauche).
 *      - Les 4 entiers de droite vont provisoirement dans le coin haut-droit de B !
 *        (Ce stockage temporaire évite d'aller toucher les 4 lignes suivantes de B).
 *   2. On lit colonne par colonne la partie haut-droite temporaire de B dans 4 variables,
 *      on lit les 4 nouveaux éléments du sous-bloc bas-gauche de A dans 4 variables,
 *      puis on range la vraie transposée à sa place définitive dans B.
 *   3. On transpose enfin le sous-bloc bas-droit 4x4.
 *
 * Nombre de variables locales utilisées : 12 au total (i, j, k, v0..v7, w0).
 */
static void transposer_64x64(int A[64][64], int B[64][64])
{
    int i, j, k;
    int v0, v1, v2, v3, v4, v5, v6, v7;

    for (i = 0; i < 64; i += 8) {
        for (j = 0; j < 64; j += 8) {
            /* Étape 1 : Traitement de la moitié supérieure (4 lignes de A) */
            for (k = 0; k < 4; k++) {
                v0 = A[i + k][j + 0];
                v1 = A[i + k][j + 1];
                v2 = A[i + k][j + 2];
                v3 = A[i + k][j + 3];
                v4 = A[i + k][j + 4];
                v5 = A[i + k][j + 5];
                v6 = A[i + k][j + 6];
                v7 = A[i + k][j + 7];

                /* Sous-bloc haut-gauche de B (définitif) */
                B[j + 0][i + k] = v0;
                B[j + 1][i + k] = v1;
                B[j + 2][i + k] = v2;
                B[j + 3][i + k] = v3;

                /* Sous-bloc haut-droit de B (stockage provisoire sans conflit) */
                B[j + 0][i + k + 4] = v4;
                B[j + 1][i + k + 4] = v5;
                B[j + 2][i + k + 4] = v6;
                B[j + 3][i + k + 4] = v7;
            }

            /* Étape 2 : Réorganisation et traitement de la moitié inférieure */
            for (k = 0; k < 4; k++) {
                /* Lecture d'une colonne du tampon temporaire de B */
                v0 = B[j + k][i + 4];
                v1 = B[j + k][i + 5];
                v2 = B[j + k][i + 6];
                v3 = B[j + k][i + 7];

                /* Lecture de la rangée correspondante dans le bas-gauche de A */
                v4 = A[i + 4][j + k];
                v5 = A[i + 5][j + k];
                v6 = A[i + 6][j + k];
                v7 = A[i + 7][j + k];

                /* Écriture définitive dans le haut-droit de B */
                B[j + k][i + 4] = v4;
                B[j + k][i + 5] = v5;
                B[j + k][i + 6] = v6;
                B[j + k][i + 7] = v7;

                /* Écriture définitive dans le bas-gauche de B */
                B[j + k + 4][i + 0] = v0;
                B[j + k + 4][i + 1] = v1;
                B[j + k + 4][i + 2] = v2;
                B[j + k + 4][i + 3] = v3;
            }

            /* Étape 3 : Transposition du dernier sous-bloc bas-droit (4x4) */
            for (k = 0; k < 4; k++) {
                v0 = A[i + k + 4][j + 4];
                v1 = A[i + k + 4][j + 5];
                v2 = A[i + k + 4][j + 6];
                v3 = A[i + k + 4][j + 7];

                B[j + 4][i + k + 4] = v0;
                B[j + 5][i + k + 4] = v1;
                B[j + 6][i + k + 4] = v2;
                B[j + 7][i + k + 4] = v3;
            }
        }
    }
}

/*
 * ==============================================================================
 * CAS 3 : Matrice 61 x 67 (M = 61 colonnes, N = 67 lignes)
 * ==============================================================================
 * Les dimensions ne sont pas des puissances de 2. Les lignes successives ne
 * tombent pas avec une périodicité stricte sur les mêmes ensembles du cache.
 * Les conflits destructeurs de la diagonale et des puissances de 2 disparaissent.
 * Un simple découpage par blocs (blocking) de taille 16 x 16 ou 17 x 17
 * suffit largement pour descendre sous le seuil maximal de 2 000 défauts.
 *
 * Nombre de variables locales utilisées : 5 au total (i, j, r, c, val).
 */
static void transposer_61x67(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, r, c;
    int val;
    const int TAILLE_BLOC = 16;

    for (i = 0; i < N; i += TAILLE_BLOC) {
        for (j = 0; j < M; j += TAILLE_BLOC) {
            for (r = i; r < i + TAILLE_BLOC && r < N; r++) {
                for (c = j; c < j + TAILLE_BLOC && c < M; c++) {
                    val = A[r][c];
                    B[c][r] = val;
                }
            }
        }
    }
}

/*
 * transposer_soumission : Point d'entrée évalué par test-trans.py.
 * Sélectionne la stratégie optimale selon les dimensions de la matrice.
 */
void transposer_soumission(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        transposer_32x32(A, B);
    } else if (M == 64 && N == 64) {
        transposer_64x64(A, B);
    } else {
        transposer_61x67(M, N, A, B);
    }
}
