#!/usr/bin/env bash
set -euo pipefail
# Use P=7 localmente. P=15/31, apenas dentro de alocacao adequada.
# Uso: bash testar_balanceamento.sh [N] [P]
N=${1:-10000}
P=${2:-7}
mkdir -p resultados
LOG="resultados/balanceamento_N${N}_P${P}.log"
: > "$LOG"
make
for v in mpi_arvore mpi_arvore_balanceada mpi_balanceado; do
  mpirun -np "$P" "./$v" "$N" --stats | tee -a "$LOG"
done
python3 analisar_balanceamento.py "$LOG" > "${LOG%.log}_por_nivel.csv"
echo "Resumo: ${LOG%.log}_por_nivel.csv"
