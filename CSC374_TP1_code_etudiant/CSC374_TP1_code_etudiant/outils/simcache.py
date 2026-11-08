"""simcache.py : simulateur de cache pédagogique (séance 2).

Lit une trace au format de valgrind --tool=lackey --trace-mem=yes :
    I 0400d7d4,8       chargement d'instruction (ignoré, comme dans le Cache Lab)
     L 7ff0005c8,8     lecture de 8 octets
     S 7ff0005d0,4     écriture
     M 0421c7f0,4     modification (lecture puis écriture de la même donnée)
et simule un cache de 2^s ensembles, E lignes par ensemble, blocs de 2^b octets.

Usage : python3 simcache.py -s 6 -E 12 -b 6 [-p lru|fifo|alea] [--3c] trace
L'option --3c classe les défauts : obligatoires (premier accès au bloc), de capacité
(défauts que ferait aussi un cache entièrement associatif LRU de même taille), de conflit (le reste).
Ce simulateur sert aux expériences du cours ; le TP1 en demande une version en C.
"""
import argparse
import random
from collections import OrderedDict


class Cache:
    def __init__(self, s, E, b, politique="lru", graine=374):
        self.s, self.E, self.b = s, E, b
        self.politique = politique
        self.ensembles = [OrderedDict() for _ in range(1 << s)]   # étiquette -> None
        self.rng = random.Random(graine)
        self.succes = self.defauts = self.evictions = 0

    def decomposer(self, adresse):
        bloc = adresse >> self.b
        return bloc & ((1 << self.s) - 1), bloc >> self.s           # (ensemble, étiquette)

    def acceder(self, adresse):
        """Renvoie True en cas de succès. Met à jour les compteurs."""
        num, etiquette = self.decomposer(adresse)
        ens = self.ensembles[num]
        if etiquette in ens:
            self.succes += 1
            if self.politique == "lru":
                ens.move_to_end(etiquette)          # devient la plus récemment utilisée
            return True
        self.defauts += 1
        if len(ens) == self.E:
            self.evictions += 1
            if self.politique == "alea":
                victime = self.rng.choice(list(ens))
                del ens[victime]
            else:                                    # lru et fifo : la plus ancienne de la file
                ens.popitem(last=False)
        ens[etiquette] = None
        return False


def lire_trace(chemin):
    """Produit les adresses accédées ; une opération M produit deux accès."""
    with open(chemin) as f:
        for ligne in f:
            if not ligne.startswith(" "):
                continue                              # lignes I et commentaires
            op, reste = ligne.split()
            adresse = int(reste.split(",")[0], 16)
            yield adresse
            if op == "M":
                yield adresse


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-s", type=int, required=True)
    ap.add_argument("-E", type=int, required=True)
    ap.add_argument("-b", type=int, required=True)
    ap.add_argument("-p", "--politique", default="lru", choices=["lru", "fifo", "alea"])
    ap.add_argument("--3c", dest="trois_c", action="store_true")
    ap.add_argument("trace")
    a = ap.parse_args()
    c = Cache(a.s, a.E, a.b, a.politique)
    if a.trois_c:
        pleine = Cache(0, (1 << a.s) * a.E, a.b, "lru")     # entièrement associatif, même taille
        vus = set()
        oblig = capa = conflit = 0
    for adr in lire_trace(a.trace):
        ok = c.acceder(adr)
        if a.trois_c:
            ok_plein = pleine.acceder(adr)
            bloc = adr >> a.b
            if not ok:
                if bloc not in vus:
                    oblig += 1
                elif not ok_plein:
                    capa += 1
                else:
                    conflit += 1
            vus.add(bloc)
    total = c.succes + c.defauts
    print(f"hits:{c.succes} misses:{c.defauts} evictions:{c.evictions}"
          f"  (taux de défauts {100 * c.defauts / total:.2f} %)")
    if a.trois_c:
        print(f"obligatoires:{oblig} capacité:{capa} conflit:{conflit}")


if __name__ == "__main__":
    main()
