#!/usr/bin/env python3
"""Resume dados STATS produzidos com --stats, por nivel da arvore e versao."""
import csv
import sys
import statistics
from collections import defaultdict

if len(sys.argv) != 2:
    raise SystemExit("Uso: python3 analisar_balanceamento.py stats.log")

medidas = defaultdict(list)
with open(sys.argv[1], encoding="utf-8") as arq:
    for line in arq:
        parts = line.strip().split(",")
        if len(parts) != 9 or parts[0] != "STATS":
            continue
        _, versao, n, p, rank, elementos, bubble, merge, comunicacao = parts
        rank = int(rank)
        # Nivel do rank em uma arvore binaria completa: floor(log2(rank+1)).
        # Na arvore adaptada, o mapeamento de ajudantes tem topologia diferente;
        # este nivel serve como identificador aproximado, NAO como profundidade real.
        nivel = (rank + 1).bit_length() - 1
        medidas[(versao, int(n), int(p), nivel)].append(
            (int(elementos), float(bubble), float(merge), float(comunicacao)))

wr = csv.writer(sys.stdout)
wr.writerow(["versao", "N", "P", "nivel_rank", "ranks", "elementos_total_bubble",
             "min_elementos_por_rank", "max_elementos_por_rank",
             "tempo_bubble_max_s", "tempo_intercalacao_max_s", "tempo_comunicacao_espera_max_s"])
for (versao, n, p, nivel), dados in sorted(medidas.items()):
    elementos = [d[0] for d in dados]
    wr.writerow([versao,n,p,nivel,len(dados),sum(elementos),min(elementos),max(elementos),
                 f"{max(d[1] for d in dados):.9f}",
                 f"{max(d[2] for d in dados):.9f}",
                 f"{max(d[3] for d in dados):.9f}"])
