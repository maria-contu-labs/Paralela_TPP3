#!/usr/bin/env bash
set -euo pipefail

# TPP3 / Atlantica (LAD): usa srun conforme material do professor.
# Uso: bash testes_atlantica.sh 10000 3
# Cada execucao solicita recursos exclusivos; nao usar mpirun no login node.

N=${1:-10000}
REPETICOES=${2:-3}

if ! [[ "$N" =~ ^[0-9]+$ ]] || (( N < 1 )); then
    echo "Erro: N deve ser inteiro positivo." >&2
    exit 2
fi
if ! [[ "$REPETICOES" =~ ^[0-9]+$ ]] || (( REPETICOES < 1 )); then
    echo "Erro: repeticoes deve ser inteiro positivo." >&2
    exit 2
fi

command -v srun >/dev/null || { echo "Erro: srun nao encontrado (execute na Atlantica)." >&2; exit 2; }
for programa in sequencial mpi_arvore mpi_arvore_balanceada mpi_balanceado; do
    [[ -x "./$programa" ]] || { echo "Erro: falta executar make clean && make ($programa)." >&2; exit 2; }
done

mkdir -p resultados
LOG="resultados/atlantica_N${N}_P1-3-7-15-31.log"
: > "$LOG"

for ((rep=1; rep<=REPETICOES; rep++)); do
    echo "---- Repeticao ${rep}/${REPETICOES}, N=${N} ----" >&2
    # O sequencial executa em apenas um no.
    srun --exclusive -N 1 -n 1 ./sequencial "$N" | tee -a "$LOG"

    for p in 1 3 7 15 31; do
        # Para p=1, uma tarefa ocupa um no; demais configuracoes usam dois nos.
        if (( p == 1 )); then nos=1; else nos=2; fi
        for versao in mpi_arvore mpi_arvore_balanceada mpi_balanceado; do
            srun --exclusive -N "$nos" -n "$p" "./$versao" "$N" | tee -a "$LOG"
        done
    done
done

python3 calcular_metricas.py "$LOG" > "${LOG%.log}_metricas.csv"
echo "Log: $LOG"
echo "Metricas: ${LOG%.log}_metricas.csv"
