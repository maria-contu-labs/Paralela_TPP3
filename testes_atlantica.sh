#!/usr/bin/env bash
set -euo pipefail

# Executar DENTRO da alocacao de dois nos exclusivos autorizada pelo professor.
# Este script nao solicita recursos ao escalonador e NAO habilita oversubscription.
# Uso: CONFIRMO_ALOCACAO=1 bash testes_atlantica.sh [N] [repeticoes]
if [[ "${CONFIRMO_ALOCACAO:-0}" != "1" ]]; then
  echo "ERRO: primeiro reserve/aloque os 2 nos e defina CONFIRMO_ALOCACAO=1." >&2
  exit 2
fi
N=${1:-10000}
REPETICOES=${2:-3}
mkdir -p resultados
LOG="resultados/atlantica_N${N}_P1-3-7-15-31.log"
: > "$LOG"
make
for ((rep=1;rep<=REPETICOES;rep++)); do
  echo "---- Repeticao ${rep}/${REPETICOES}, N=${N} ----" >&2
  ./sequencial "$N" | tee -a "$LOG"
  for p in 1 3 7 15 31; do
    for versao in mpi_arvore mpi_arvore_balanceada mpi_balanceado; do
      mpirun -np "$p" "./$versao" "$N" | tee -a "$LOG"
    done
  done
done
python3 calcular_metricas.py "$LOG" > "${LOG%.log}_metricas.csv"
echo "Log: $LOG"
echo "Metricas: ${LOG%.log}_metricas.csv"
