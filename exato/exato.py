# resolvedor exato do m1 e do fleet com pulp + highs
# uso: python exato.py m1|fleet atual|5a|5b 10|15|30 [lambda] [limite_s]
import sys
import time

import highspy
import pulp

RAIOS = {10: 0.0056, 15: 0.0084, 30: 0.0168}
FROTAS = {"atual": (1, 3), "5a": (1, 4), "5b": (2, 3)}


def carregar(arquivo):
    quadras = []
    with open(arquivo) as f:
        f.readline()
        for linha in f:
            linha = linha.strip()
            if not linha:
                continue
            c = linha.split(",")
            quadras.append((float(c[1]), float(c[2]), float(c[3])))
    return quadras


def vizinhos(quadras, raio):
    # mesmo teste do comum.hpp
    raio2 = raio * raio
    n = len(quadras)
    viz = [[] for _ in range(n)]
    for j in range(n):
        xj, yj, _ = quadras[j]
        for i in range(n):
            dx = quadras[i][0] - xj
            dy = quadras[i][1] - yj
            if dx * dx + dy * dy <= raio2:
                viz[j].append(i)
    return viz


def main():
    modelo = sys.argv[1]
    frota = sys.argv[2]
    raio = int(sys.argv[3])
    lam_txt = sys.argv[4] if len(sys.argv) > 4 else "0.2"
    lam = float(lam_txt)
    limite = float(sys.argv[5]) if len(sys.argv) > 5 else 5 * 3600
    if modelo == "fleet":
        lam = 0
        lam_txt = "0"

    usa, usb = FROTAS[frota]
    quadras = carregar("../quadras.csv")
    n = len(quadras)
    d = [q[2] for q in quadras]
    total = sum(d)
    # distancia simetrica: viz[i] tambem e quem cobre i
    viz = vizinhos(quadras, RAIOS[raio])

    inicio = time.time()

    lp = pulp.LpProblem("cobertura", pulp.LpMaximize)
    xa = [pulp.LpVariable(f"xa{j}", cat="Binary") for j in range(n)]
    xb = [pulp.LpVariable(f"xb{j}", cat="Binary") for j in range(n)]
    # y e ya continuas: com x inteiro existe otimo com y em 0 ou 1
    y = [pulp.LpVariable(f"y{i}", 0, 1) for i in range(n)]
    ya = [pulp.LpVariable(f"ya{i}", 0, 1) for i in range(n)]

    if modelo == "m1":
        # cobertura por qualquer viatura + lambda x cobertura pela usa
        lp += pulp.lpSum(d[i] * y[i] + lam * d[i] * ya[i] for i in range(n))
        for i in range(n):
            lp += y[i] <= pulp.lpSum(xa[j] + xb[j] for j in viz[i])
            lp += ya[i] <= pulp.lpSum(xa[j] for j in viz[i])
    else:
        # fleet: so conta se uma usa e uma usb cobrem a quadra ao mesmo tempo
        lp += pulp.lpSum(d[i] * y[i] for i in range(n))
        for i in range(n):
            lp += y[i] <= pulp.lpSum(xa[j] for j in viz[i])
            lp += y[i] <= pulp.lpSum(xb[j] for j in viz[i])

    lp += pulp.lpSum(xa) == usa
    lp += pulp.lpSum(xb) == usb
    # uma viatura por quadra
    for j in range(n):
        lp += xa[j] + xb[j] <= 1

    montagem = time.time() - inicio

    # gapRel=0: so termina ao provar o otimo
    solver = pulp.HiGHS(msg=False, timeLimit=limite, gapRel=0)
    inicio = time.time()
    lp.solve(solver)
    tempo = time.time() - inicio

    local_a = [j for j in range(n) if (xa[j].value() or 0) > 0.5]
    local_b = [j for j in range(n) if (xb[j].value() or 0) > 0.5]

    # recalcula o valor a partir das bases escolhidas
    cob_a = set(i for j in local_a for i in viz[j])
    cob_b = set(i for j in local_b for i in viz[j])
    if modelo == "m1":
        valor = sum(d[i] for i in cob_a | cob_b) + lam * sum(d[i] for i in cob_a)
    else:
        valor = sum(d[i] for i in cob_a & cob_b)
    simples = sum(d[i] for i in cob_a | cob_b)

    limitante = "nd"
    gap = "nd"
    otimo = "nao"
    h = getattr(lp, "solverModel", None)
    if h is not None:
        info = h.getInfo()
        limitante = info.mip_dual_bound
        gap = info.mip_gap
        if h.getModelStatus() == highspy.HighsModelStatus.kOptimal:
            otimo = "sim"

    solucao = " ".join([f"USA:{j}" for j in local_a] + [f"USB:{j}" for j in local_b])
    print(f"{modelo},{frota},{raio},{lam_txt},{valor},{100 * valor / total},"
          f"{100 * simples / total},{limitante},{gap},{otimo},{tempo},"
          f"{montagem},{solucao}")


main()
