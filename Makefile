CXX ?= g++
MPICXX = mpic++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra -Wpedantic

.PHONY: all clean
all: sequencial mpi_arvore mpi_arvore_balanceada mpi_balanceado

sequencial: sequencial.cpp ordenacao.h
	$(CXX) $(CXXFLAGS) sequencial.cpp -o sequencial

mpi_arvore: mpi_arvore.cpp ordenacao.h
	$(MPICXX) $(CXXFLAGS) mpi_arvore.cpp -o mpi_arvore

mpi_arvore_balanceada: mpi_arvore_balanceada.cpp ordenacao.h
	$(MPICXX) $(CXXFLAGS) mpi_arvore_balanceada.cpp -o mpi_arvore_balanceada

mpi_balanceado: mpi_balanceado.cpp ordenacao.h
	$(MPICXX) $(CXXFLAGS) mpi_balanceado.cpp -o mpi_balanceado

clean:
	rm -f sequencial mpi_arvore mpi_arvore_balanceada mpi_balanceado
