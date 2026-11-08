#!/usr/bin/env python3
"""test-trans.py : évaluation de la partie B du TP1 (CSC 374).

Pour chaque taille (32 x 32, 64 x 64, 61 x 67) : compile tracegen avec trans.c (en -O0),
l'exécute sous valgrind --tool=lackey, garde les accès aux tableaux A et B faits entre les deux
écritures dans le marqueur (donc par transposer_soumission), les rejoue dans csim-ref avec
s = 5, E = 1, b = 5, et convertit le nombre de défauts en points.
Usage : python3 test-trans.py [-v]     (-v conserve les traces trans_NxM.trace)
"""
import os
import re
import subprocess
import sys

# (M, N, seuil bas, seuil haut, points) : la totalité des points sous le seuil bas,
# aucun point au-dessus du seuil haut, interpolation linéaire entre les deux.
TAILLES = [(32, 32, 300, 600, 3.0), (64, 64, 1300, 2000, 3.0), (61, 67, 2000, 3000, 3.0)]
ICI = os.path.dirname(os.path.abspath(__file__))


def compiler():
    cmd = ["gcc", "-std=c17", "-O0", "-g", "-Wall", "-Wextra", "-o", "tracegen",
           "tracegen.c", "trans.c", "cachelab.c"]
    r = subprocess.run(cmd, cwd=ICI, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit("échec de la compilation :\n" + r.stderr)
    if r.stderr.strip():
        print("avertissements de compilation :\n" + r.stderr)


def tracer(M, N, sortie):
    """Renvoie (correct, nombre d'accès retenus) et écrit la trace filtrée."""
    r = subprocess.run(["valgrind", "--tool=lackey", "--trace-mem=yes", "--log-fd=1",
                        "./tracegen", str(M), str(N)], cwd=ICI, capture_output=True, text=True)
    m = re.search(r"marqueur (0x[0-9a-f]+) zone (0x[0-9a-f]+) (\d+)", r.stderr)
    if m is None:
        sys.exit("tracegen n'a pas pu s'exécuter :\n" + r.stderr[-2000:])
    marq, base, taille = int(m.group(1), 16), int(m.group(2), 16), int(m.group(3))
    dedans, n = False, 0
    with open(os.path.join(ICI, sortie), "w") as f:
        for ligne in r.stdout.splitlines():
            if not (ligne.startswith(" ") and ligne[1:2] in ("L", "S", "M")):
                continue
            adr = int(ligne.split()[1].split(",")[0], 16)
            if adr == marq and ligne[1] == "S":
                if dedans:
                    break
                dedans = True
                continue
            if dedans and base <= adr < base + taille:
                f.write(ligne + "\n")
                n += 1
    return r.returncode == 0, n


def simuler(trace):
    r = subprocess.run(["./csim-ref", "-s", "5", "-E", "1", "-b", "5", "-t", trace],
                       cwd=ICI, capture_output=True, text=True)
    m = re.search(r"hits:(\d+) misses:(\d+) evictions:(\d+)", r.stdout)
    if m is None:
        sys.exit("csim-ref a échoué :\n" + r.stderr)
    return tuple(int(x) for x in m.groups())


def points(defauts, bas, haut, maxi):
    if defauts <= bas:
        return maxi
    if defauts >= haut:
        return 0.0
    return maxi * (haut - defauts) / (haut - bas)


def main():
    garder = "-v" in sys.argv[1:]
    compiler()
    total = 0.0
    print(f"{'Taille':>8} {'Correct':>8} {'Accès':>7} {'Succès':>7} {'Défauts':>8} {'Seuils':>11} {'Points':>7}")
    for M, N, bas, haut, maxi in TAILLES:
        nom = f"trans_{M}x{N}.trace"
        correct, n = tracer(M, N, nom)
        succes, defauts, _ = simuler(nom)
        p = points(defauts, bas, haut, maxi) if correct else 0.0
        total += p
        print(f"{M:>3} x {N:<3} {'oui' if correct else 'NON':>8} {n:>7} {succes:>7} {defauts:>8} "
              f"{bas:>5}-{haut:<5} {p:>5.2f}/{maxi:g}")
        if not garder:
            os.remove(os.path.join(ICI, nom))
    print(f"Total partie B : {total:.2f} / 9")


if __name__ == "__main__":
    main()
