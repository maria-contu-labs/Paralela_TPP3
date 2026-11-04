#!/usr/bin/env bash
set -euo pipefail
make
for n in 11 40 1000; do
  ./sequencial "$n"
  for p in 1 3 7; do
    mpirun -np "$p" ./mpi_arvore "$n" --stats
    mpirun -np "$p" ./mpi_balanceado "$n" --stats
  done
done
