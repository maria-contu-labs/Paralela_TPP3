#!/usr/bin/env bash
set -euo pipefail

# Testes exploratorios, NAO substituem a medicao oficial na Atlantica.
# Uso: bash testes_locais.sh [repeticoes] [N]
REPETICOES=${1:-3}
N=${2:-100000}
mkdir -p resultados
LOG="resultados/local_N${N}_P1-3-7.log"
: > "$LOG"
make
for ((rep=1;rep<=REPETICOES;rep++)); do
  echo "---- Repeticao ${rep}/${REPETICOES}, N=${N} ----" >&2
  ./sequencial "$N" | tee -a "$LOG"
  for p in 1 3 7; do
    for versao in mpi_arvore mpi_arvore_balanceada mpi_balanceado; do
      mpirun -np "$p" "./$versao" "$N" | tee -a "$LOG"
    done
  done
done
python3 calcular_metricas.py "$LOG" > "${LOG%.log}_metricas.csv"
echo "Log: $LOG"
echo "Metricas: ${LOG%.log}_metricas.csv"
