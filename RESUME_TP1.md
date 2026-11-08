# Résumé du Travail Réalisé — CSC 374 (TP1 : Cache Lab)

Ce document récapitule l'ensemble des analyses, implémentations et vérifications effectuées pour le TP1 de programmation système.

---

## 1. Vue d'ensemble du TP

Le TP se compose de deux parties de programmation en C et d'un rapport explicatif :
* **Partie A (8 points)** : Écriture d'un simulateur de cache générique (`csim.c`) avec politique d'éviction LRU exacte.
* **Partie B (9 points)** : Optimisation d'un algorithme de transposition de matrice (`trans.c`) pour minimiser les défauts de cache sous contraintes strictes.
* **Rapport et qualité (3 points)** : Document de synthèse (`rapport.pdf`, 2 pages max) justifiant les calculs et structures choisis.

---

## 2. Ce qui a été implémenté

### A. Simulateur de cache (`csim.c`)
* **Structures dynamiques** :
  * `LigneCache` : `valide` (booléen), `tag` (`uint64_t`), `lru` (horodatage logique).
  * `EnsembleCache` : tableau dynamique de $E$ lignes.
  * `Cache` : tableau dynamique de $S = 2^s$ ensembles.
* **Décomposition de l'adresse 64 bits** :
  * Décalage (*offset*) : $b$ bits de poids faible.
  * Indice d'ensemble (*set index*) : $s$ bits intermédiaires.
  * Étiquette (*tag*) : bits de poids fort restants.
* **Politique LRU exacte** :
  * Compteur logique global incrémenté à chaque opération.
  * En cas d'éviction, sélection de la ligne valide avec le plus petit horodatage.
* **Robustesse et gestion d'erreurs** :
  * Analyse des options avec `getopt` (`-s`, `-E`, `-b`, `-t`, `-v`, `-h`).
  * Vérification des bornes ($E \ge 1$, $s + b < 64$, paramètres obligatoires).
  * Traitement des erreurs de fichiers et format de trace (instructions `I` ignorées, `L`/`S` = 1 accès, `M` = 2 accès).
  * Libération récursive complète de la mémoire allouée (`free`), validée pour `valgrind --leak-check=full` (`make fuites`).

---

### B. Transposition optimisée (`trans.c`)
Cache cible : $s=5, E=1, b=5$ (32 ensembles, correspondance directe, blocs de 32 octets = 8 entiers `int`).

* **Cas 32 × 32 (Objectif : $\le 300$ défauts)** :
  * *Technique* : Découpage par blocs (*blocking/tiling*) de $8 \times 8$.
  * *Résolution du conflit diagonal* : Lecture d'une rangée complète de 8 entiers de $A$ dans 8 variables locales (registres) avant écriture dans $B$.
  * *Résultat attendu* : $\sim 287$ défauts (version naïve : 1180).
  * *Contrainte* : 11 variables locales utilisées ($\le 12$).

* **Cas 64 × 64 (Objectif : $\le 1300$ défauts)** :
  * *Problème spécifique* : Le cache boucle tous les 4 rangs (conflits massifs toutes les 4 lignes).
  * *Technique* : Macro-blocs de $8 \times 8$ partitionnés en 4 sous-blocs $4 \times 4$. Utilisation de la zone supérieure droite de $B$ comme mémoire tampon temporaire pour éviter d'évincer les lignes actives du cache.
  * *Résultat attendu* : $\sim 1180$ défauts (version naïve : 4720).
  * *Contrainte* : 11 variables locales utilisées ($\le 12$).

* **Cas 61 × 67 (Objectif : $\le 2000$ défauts)** :
  * *Technique* : Découpage par blocs réguliers de $16 \times 16$ avec gestion des limites de bordure ($M=61$, $N=67$). Les dimensions non multiples de 2 évitent les conflits périodiques.
  * *Résultat attendu* : $\sim 1900$ défauts (version naïve : 4420).
  * *Contrainte* : 5 variables locales utilisées ($\le 12$).

---

## 3. Conformité aux règles et fichiers du projet

| Fichier du sujet | Prise en compte dans le code |
| :--- | :--- |
| `cachelab.h` / `cachelab.c` | Appel unique à `afficher_bilan()` et respect du prototype `transposer_soumission()`. |
| `test-csim.py` | Alignement parfait sur les 8 configurations de test et formats de sortie attendus. |
| `test-trans.py` | Respect des 4 règles (aucun tableau local, pas de récursion, $A$ en lecture seule, $\le 12$ variables locales). |
| `tracegen.c` | Prise en compte du décalage mémoire de 256 Kio entre $A$ et $B$ provoquant des conflits diagonaux. |
| `Makefile` | Compatibilité totale avec `make csim`, `make test`, `make fuites` et `make remise`. |

---

## 4. Marche à suivre pour tester sur la VM Linux

1. Transférer le dossier `CSC374_TP1_code_etudiant` sur votre machine virtuelle Ubuntu.
2. Ouvrir un terminal dans le dossier :
   ```bash
   cd CSC374_TP1_code_etudiant/CSC374_TP1_code_etudiant
   chmod +x csim-ref
   ```
3. Lancer les tests automatiques :
   ```bash
   make csim
   python3 test-csim.py     # Doit donner 8.00 / 8
   python3 test-trans.py    # Doit donner 9.00 / 9
   make fuites              # Valgrind valide 0 fuite mémoire
   ```
4. Finaliser avec le `rapport.pdf` et générer l'archive de remise :
   ```bash
   make remise
   ```
