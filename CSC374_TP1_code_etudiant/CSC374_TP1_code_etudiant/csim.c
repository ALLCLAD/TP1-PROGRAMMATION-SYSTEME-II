/* csim.c : simulateur de cache du TP1 (CSC 374), partie A.
 *
 * Nom, prénom :
 * Matricule   :
 *
 * Usage : ./csim [-hv] -s <s> -E <E> -b <b> -t <fichier_trace>
 *
 * REPRÉSENTATION DU CACHE :
 * -------------------------
 * Un cache est caractérisé par :
 *   - S = 2^s ensembles (sets)
 *   - E lignes par ensemble (associativité)
 *   - B = 2^b octets par bloc
 *
 * Chaque ligne (struct LigneCache) contient :
 *   1. valid : drapeau booléen (1 si la ligne contient un bloc valide, 0 sinon).
 *   2. tag   : l'étiquette extraite de l'adresse mémoire (bits de poids fort).
 *   3. lru   : horodatage logique du dernier accès pour la politique LRU.
 *
 * Un ensemble (struct EnsembleCache) contient un tableau dynamique de E lignes.
 * Le cache complet (struct Cache) contient un tableau dynamique de S ensembles.
 *
 * DÉCOMPOSITION DE L'ADRESSE (64 bits) :
 * --------------------------------------
 * | 64 - (s + b) bits de tag | s bits d'indice d'ensemble | b bits de décalage |
 *  - Indice d'ensemble (set index) = (adresse >> b) & ((1ULL << s) - 1)
 *  - Tag = adresse >> (s + b)
 *
 * POLITIQUE DE REMPLACEMENT LRU (Least Recently Used) :
 * -----------------------------------------------------
 * Un compteur d'accès global (compteur_lru) est incrémenté à chaque opération mémoire.
 * Lorsqu'une ligne est accédée (en succès ou lors d'un chargement), son champ 'lru'
 * reçoit la valeur actuelle de ce compteur.
 * Lors d'une éviction (toutes les E lignes sont occupées), la ligne victime choisie
 * est obligatoirement celle qui possède la plus petite valeur 'lru' (la moins récemment utilisée).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include "cachelab.h"

/* Structure représentant une ligne dans un ensemble de cache */
typedef struct {
    int valide;            /* 1 si la ligne est valide, 0 sinon */
    uint64_t tag;          /* Étiquette d'identification du bloc */
    uint64_t lru;          /* Horodatage du dernier accès (LRU) */
} LigneCache;

/* Structure représentant un ensemble de lignes */
typedef struct {
    LigneCache *lignes;    /* Tableau de E lignes */
} EnsembleCache;

/* Structure représentant le cache complet */
typedef struct {
    int s;                 /* Bits d'index */
    int E;                 /* Lignes par ensemble */
    int b;                 /* Bits de décalage */
    uint64_t nb_ensembles; /* S = 2^s */
    EnsembleCache *ensembles;
} Cache;

/* Compteur global pour horodater les accès LRU */
static uint64_t compteur_lru = 0;

/* Statistiques globales */
static int nb_succes = 0;
static int nb_defauts = 0;
static int nb_evictions = 0;

/*
 * afficher_aide : Affiche la syntaxe d'utilisation du programme
 */
static void afficher_aide(const char *nom_prog)
{
    printf("Usage : %s [-hv] -s <s> -E <E> -b <b> -t <fichier_trace>\n", nom_prog);
    printf("Options :\n");
    printf("  -h          affiche cette aide\n");
    printf("  -v          mode bavard : affiche le résultat de chaque accès\n");
    printf("  -s <s>      nombre de bits d'indice d'ensemble (S = 2^s ensembles)\n");
    printf("  -E <E>      associativité (nombre de lignes par ensemble)\n");
    printf("  -b <b>      nombre de bits de décalage (B = 2^b octets par bloc)\n");
    printf("  -t <trace>  fichier de trace au format de Lackey\n");
}

/*
 * allouer_cache : Alloue dynamiquement la mémoire pour le cache selon s et E.
 * Retourne le pointeur vers la structure allouée, ou NULL en cas d'erreur.
 */
static Cache *allouer_cache(int s, int E, int b)
{
    Cache *cache = malloc(sizeof(Cache));
    if (!cache) {
        perror("Erreur allocation cache");
        return NULL;
    }

    cache->s = s;
    cache->E = E;
    cache->b = b;
    cache->nb_ensembles = 1ULL << s;

    cache->ensembles = malloc(cache->nb_ensembles * sizeof(EnsembleCache));
    if (!cache->ensembles) {
        perror("Erreur allocation ensembles");
        free(cache);
        return NULL;
    }

    for (uint64_t i = 0; i < cache->nb_ensembles; i++) {
        cache->ensembles[i].lignes = calloc(E, sizeof(LigneCache));
        if (!cache->ensembles[i].lignes) {
            perror("Erreur allocation lignes");
            /* Libération des ensembles déjà alloués en cas d'échec */
            for (uint64_t j = 0; j < i; j++) {
                free(cache->ensembles[j].lignes);
            }
            free(cache->ensembles);
            free(cache);
            return NULL;
        }
    }

    return cache;
}

/*
 * liberer_cache : Libère proprement toute la mémoire dynamique du cache.
 */
static void liberer_cache(Cache *cache)
{
    if (!cache) return;
    if (cache->ensembles) {
        for (uint64_t i = 0; i < cache->nb_ensembles; i++) {
            free(cache->ensembles[i].lignes);
        }
        free(cache->ensembles);
    }
    free(cache);
}

/*
 * acceder_cache : Simule un accès mémoire à l'adresse spécifiée.
 * Met à jour les compteurs de succès, défauts, évictions et le LRU.
 * Si bavard != 0, affiche "hit", "miss", "eviction" selon le résultat.
 */
static void acceder_cache(Cache *cache, uint64_t adresse, int bavard)
{
    compteur_lru++;

    /* Découpage de l'adresse en indice d'ensemble et tag */
    uint64_t masque_ensemble = cache->nb_ensembles - 1ULL;
    uint64_t index_ensemble = (adresse >> cache->b) & masque_ensemble;
    uint64_t tag = adresse >> (cache->s + cache->b);

    EnsembleCache *ens = &cache->ensembles[index_ensemble];

    /* 1. Recherche d'un SUCCÈS (Hit) */
    for (int i = 0; i < cache->E; i++) {
        if (ens->lignes[i].valide && ens->lignes[i].tag == tag) {
            nb_succes++;
            ens->lignes[i].lru = compteur_lru;
            if (bavard) printf(" hit");
            return;
        }
    }

    /* 2. DÉFAUT (Miss) : le bloc n'est pas dans le cache */
    nb_defauts++;
    if (bavard) printf(" miss");

    /* Recherche d'une ligne libre (non valide) dans l'ensemble */
    int indice_libre = -1;
    for (int i = 0; i < cache->E; i++) {
        if (!ens->lignes[i].valide) {
            indice_libre = i;
            break;
        }
    }

    /* Si une ligne est libre, on y charge le bloc sans éviction */
    if (indice_libre != -1) {
        ens->lignes[indice_libre].valide = 1;
        ens->lignes[indice_libre].tag = tag;
        ens->lignes[indice_libre].lru = compteur_lru;
        return;
    }

    /* 3. ÉVICTION : toutes les lignes sont pleines, recherche de la victime LRU */
    nb_evictions++;
    if (bavard) printf(" eviction");

    int indice_victime = 0;
    uint64_t min_lru = ens->lignes[0].lru;

    for (int i = 1; i < cache->E; i++) {
        if (ens->lignes[i].lru < min_lru) {
            min_lru = ens->lignes[i].lru;
            indice_victime = i;
        }
    }

    /* Remplacement de la ligne victime par le nouveau bloc */
    ens->lignes[indice_victime].tag = tag;
    ens->lignes[indice_victime].lru = compteur_lru;
}

int main(int argc, char **argv)
{
    int opt;
    int opt_h = 0;
    int opt_v = 0;
    int s = -1;
    int E = -1;
    int b = -1;
    char *fichier_trace = NULL;

    /* Analyse des arguments de la ligne de commande */
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                opt_h = 1;
                break;
            case 'v':
                opt_v = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                fichier_trace = optarg;
                break;
            default:
                afficher_aide(argv[0]);
                return 1;
        }
    }

    /* Si -h est demandé */
    if (opt_h) {
        afficher_aide(argv[0]);
        return 0;
    }

    /* Validation des paramètres obligatoires et de leurs domaines */
    if (s < 0 || E <= 0 || b < 0 || fichier_trace == NULL) {
        fprintf(stderr, "%s : Paramètres obligatoires manquants ou invalides.\n", argv[0]);
        afficher_aide(argv[0]);
        return 1;
    }

    if (s + b >= 64) {
        fprintf(stderr, "%s : La somme s + b (%d) doit être strictement inférieure à 64.\n", argv[0], s + b);
        return 1;
    }

    /* Allocation dynamique du cache */
    Cache *cache = allouer_cache(s, E, b);
    if (!cache) {
        return 1;
    }

    /* Ouverture du fichier de trace */
    FILE *fichier = fopen(fichier_trace, "r");
    if (!fichier) {
        fprintf(stderr, "%s : Impossible d'ouvrir le fichier de trace '%s' : %s\n",
                argv[0], fichier_trace, strerror(errno));
        liberer_cache(cache);
        return 1;
    }

    char tampon_ligne[256];
    char op;
    uint64_t adresse;
    int taille;
    int num_ligne = 0;

    /* Lecture ligne par ligne de la trace */
    while (fgets(tampon_ligne, sizeof(tampon_ligne), fichier)) {
        num_ligne++;

        /* 1. Les lignes qui ne commencent pas par une espace (ex: instructions 'I') sont ignorées */
        if (tampon_ligne[0] != ' ') {
            continue;
        }

        /* 2. Analyse de l'opération, de l'adresse (hexadécimale) et de la taille */
        if (sscanf(tampon_ligne, " %c %lx,%d", &op, &adresse, &taille) == 3) {
            if (op == 'L' || op == 'S') {
                /* Lecture ou Écriture : 1 seul accès mémoire */
                if (opt_v) printf("%c %lx,%d", op, adresse, taille);
                acceder_cache(cache, adresse, opt_v);
                if (opt_v) printf("\n");
            } else if (op == 'M') {
                /* Modification : lecture suivie immédiatement d'une écriture */
                if (opt_v) printf("%c %lx,%d", op, adresse, taille);
                acceder_cache(cache, adresse, opt_v); /* Lecture */
                acceder_cache(cache, adresse, opt_v); /* Écriture */
                if (opt_v) printf("\n");
            }
        }
    }

    fclose(fichier);

    /* Libération de la mémoire allouée */
    liberer_cache(cache);

    /* Bilan final obligatoire */
    afficher_bilan(nb_succes, nb_defauts, nb_evictions);

    return 0;
}
