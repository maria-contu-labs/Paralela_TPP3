#!/usr/bin/env bash
set -euo pipefail
# Executar apenas em alocação que comporte os processos, de acordo com as regras da Atlantica.
# Este script NAO reserva nos nem altera opcoes de oversubscription.
# Reduza/substitua a lista de processos conforme a alocacao aprovada e o ambiente MPI.
make
for n in 10000 100000 1000000; do
  ./sequencial "$n"
  for p in 1 3 7 15 31; do
    mpirun -np "$p" ./mpi_arvore "$n"
    mpirun -np "$p" ./mpi_balanceado "$n"
  done
done
# Para 63,127,255,511,1023, executar caso a caso com as opções apropriadas
# da plataforma, depois de verificar disponibilidade e limites do cluster.
