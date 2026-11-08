#!/usr/bin/env python3
"""test-csim.py : évaluation de la partie A du TP1 (CSC 374).

Compile csim (make csim), puis compare, sur huit couples (trace, paramètres), le bilan de
./csim à celui du simulateur de référence ./csim-ref. Chaque couple vaut 1 point, accordé
seulement si les trois nombres (succès, défauts, évictions) sont identiques.
Usage : python3 test-csim.py
"""
import os
import subprocess
import sys

ICI = os.path.dirname(os.path.abspath(__file__))
TESTS = [
    ("traces/exercice2.trace", 1, 2, 2),
    ("traces/entr4.trace", 2, 1, 3),
    ("traces/cyclique_13.trace", 6, 12, 6),
    ("traces/prodscal_0.trace", 6, 1, 6),
    ("traces/ordre_colonnes_64.trace", 4, 4, 6),
    ("traces/trans_32x32.trace", 0, 32, 5),
    ("traces/histo.trace", 4, 2, 4),
    ("traces/histo.trace", 8, 4, 6),
]


def executer(prog, s, E, b, trace):
    chemin = os.path.join(ICI, ".csim_resultats")
    if os.path.exists(chemin):
        os.remove(chemin)
    try:
        r = subprocess.run([prog, "-s", str(s), "-E", str(E), "-b", str(b), "-t", trace],
                           cwd=ICI, capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        return None, "délai dépassé (60 s)"
    if not os.path.exists(chemin):
        return None, f"pas de bilan (code de retour {r.returncode}) {r.stderr.strip()[:200]}"
    with open(chemin) as f:
        return tuple(int(x) for x in f.read().split()), ""


def main():
    r = subprocess.run(["make", "csim"], cwd=ICI, capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit("échec de la compilation :\n" + r.stdout + r.stderr)
    print(f"{'Trace':<30} {'s':>2} {'E':>3} {'b':>2}  {'csim (succès défauts évictions)':<32} {'référence':<22} pts")
    total = 0
    for trace, s, E, b in TESTS:
        ref, _ = executer("./csim-ref", s, E, b, trace)
        res, err = executer("./csim", s, E, b, trace)
        ok = res == ref
        total += ok
        txt = " ".join(map(str, res)) if res else err
        print(f"{trace:<30} {s:>2} {E:>3} {b:>2}  {txt:<32} {' '.join(map(str, ref)):<22} {int(ok)}")
    print(f"Total partie A : {total} / {len(TESTS)}")


if __name__ == "__main__":
    main()
