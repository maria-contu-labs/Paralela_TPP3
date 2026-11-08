#!/usr/bin/env bash
set -euo pipefail
make
for n in 7 13 40 1000; do
  ./sequencial "$n"
  for p in 1 3 7; do
    for v in mpi_arvore mpi_arvore_balanceada mpi_balanceado; do
      if [[ "$v" == "mpi_arvore_balanceada" && "$p" -gt 1 && "$n" -lt "$((p+1))" ]]; then
        continue
      fi
      mpirun -np "$p" "./$v" "$n"
   done
 done
done
# Testes de muitos ranks so quando a alocacao comportar 15 e 31 ranks.
if [[ "${TESTE_GRANDE:-0}" == "1" ]]; then
  for v in mpi_arvore mpi_arvore_balanceada mpi_balanceado; do
    mpirun -np 15 "./$v" 101
    mpirun -np 31 "./$v" 103
  done
fi
