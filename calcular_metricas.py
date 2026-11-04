#!/usr/bin/env python3
"""Lê linhas RESULTADO do log; calcula mediana, speed-up e eficiência por versão, N e P."""
import csv
import statistics
import sys
from collections import defaultdict

if len(sys.argv) != 2:
    raise SystemExit("Uso: python3 calcular_metricas.py resultados.log")

medidas = defaultdict(list)
with open(sys.argv[1], encoding="utf-8") as arquivo:
    for linha in arquivo:
        partes = linha.strip().split(",")
        if len(partes) != 7 or partes[0] != "RESULTADO" or partes[6] != "OK":
            continue
        _, versao, n, p, delta, tempo, _ = partes
        medidas[(versao, int(n), int(p), int(delta))].append(float(tempo))

referencia = {}
for (versao, n, p, delta), tempos in medidas.items():
    if versao == "sequencial":
        referencia[n] = statistics.median(tempos)

writer = csv.writer(sys.stdout)
writer.writerow(["versao", "N", "processos", "delta", "repeticoes", "tempo_mediano_s", "speedup", "eficiencia"])
for (versao, n, p, delta), tempos in sorted(medidas.items(), key=lambda x: (x[0][1], x[0][0], x[0][2])):
    tempo = statistics.median(tempos)
    seq = referencia.get(n)
    speedup = seq / tempo if seq is not None and tempo > 0 else None
    eficiencia = speedup / p if speedup is not None else None
    writer.writerow([versao, n, p, delta, len(tempos), f"{tempo:.9f}",
                     f"{speedup:.6f}" if speedup is not None else "",
                     f"{eficiencia:.6f}" if eficiencia is not None else ""])
