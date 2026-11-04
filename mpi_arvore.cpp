#include <mpi.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include "ordenacao.h"

// Arvore binaria: o processo 0 e a raiz; filhos de r: 2*r+1 e 2*r+2.
// Apenas as folhas executam Bubble Sort; os nos internos so distribuem e intercalam.
constexpr int TAG_TRABALHO = 11;
constexpr int TAG_RESULTADO = 12;

struct Estatisticas {
    double tempo_bubble = 0.0;
    double tempo_intercalacao = 0.0;
    double tempo_comunicacao = 0.0; // inclui esperas nas chamadas MPI
    int elementos_bubble = 0;
};

void ordenar_arvore(std::vector<int>& vetor, int rank, int processos, int delta,
                    Estatisticas& estatisticas) {
    int filho_esquerdo = 2 * rank + 1;
    int filho_direito = 2 * rank + 2;
    if (static_cast<int>(vetor.size()) <= delta || filho_direito >= processos) {
        double inicio = MPI_Wtime();
        bubble_sort(vetor);
        estatisticas.tempo_bubble += MPI_Wtime() - inicio;
        estatisticas.elementos_bubble = static_cast<int>(vetor.size());
        return;
    }

    int esquerda = static_cast<int>(vetor.size()) / 2;
    int direita = static_cast<int>(vetor.size()) - esquerda;
    double inicio = MPI_Wtime();
    MPI_Send(vetor.data(), esquerda, MPI_INT, filho_esquerdo, TAG_TRABALHO, MPI_COMM_WORLD);
    MPI_Send(vetor.data() + esquerda, direita, MPI_INT, filho_direito,
             TAG_TRABALHO, MPI_COMM_WORLD);

    MPI_Recv(vetor.data(), esquerda, MPI_INT, filho_esquerdo, TAG_RESULTADO,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(vetor.data() + esquerda, direita, MPI_INT, filho_direito,
             TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    estatisticas.tempo_comunicacao += MPI_Wtime() - inicio;

    inicio = MPI_Wtime();
    intercalar(vetor, esquerda);
    estatisticas.tempo_intercalacao += MPI_Wtime() - inicio;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank = 0, processos = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);

    if (argc < 2 || argc > 3) {
        if (rank == 0) std::fprintf(stderr, "Uso: mpirun -np P ./mpi_arvore N [--stats]\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    char* fim = nullptr;
    long long lido = std::strtoll(argv[1], &fim, 10);
    if (*fim != '\0' || lido < 1 || lido > std::numeric_limits<int>::max()) {
        if (rank == 0) std::fprintf(stderr, "Tamanho do vetor invalido.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const int n = static_cast<int>(lido);
    const bool stats = (argc == 3 && std::string(argv[2]) == "--stats");
    if (argc == 3 && !stats) {
        if (rank == 0) std::fprintf(stderr, "Opcao desconhecida.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    // Para esta versao, usamos sempre uma arvore binaria completa.
    if ((processos & (processos + 1)) != 0 || n < (processos + 1) / 2) {
        if (rank == 0) std::fprintf(stderr,
            "Use P=1,3,7,15,31,... e N >= numero de folhas.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    const int folhas = (processos + 1) / 2;
    const int delta = (n + folhas - 1) / folhas;
    std::vector<int> vetor;
    if (rank == 0) {
        vetor.resize(n);
        preencher_reverso(vetor);
    }
    Estatisticas estatisticas;
    MPI_Barrier(MPI_COMM_WORLD);
    double inicio_total = MPI_Wtime();
    if (rank != 0) {
        int pai = (rank - 1) / 2;
        MPI_Status status;
        double t = MPI_Wtime();
        MPI_Probe(pai, TAG_TRABALHO, MPI_COMM_WORLD, &status);
        int tamanho = 0;
        MPI_Get_count(&status, MPI_INT, &tamanho);
        vetor.resize(tamanho);
        MPI_Recv(vetor.data(), tamanho, MPI_INT, pai, TAG_TRABALHO,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        estatisticas.tempo_comunicacao += MPI_Wtime() - t;
    }

    ordenar_arvore(vetor, rank, processos, delta, estatisticas);
    if (rank != 0) {
        int pai = (rank - 1) / 2;
        double t = MPI_Wtime();
        MPI_Send(vetor.data(), static_cast<int>(vetor.size()), MPI_INT,
                 pai, TAG_RESULTADO, MPI_COMM_WORLD);
        estatisticas.tempo_comunicacao += MPI_Wtime() - t;
    }
    double segundos = MPI_Wtime() - inicio_total;
    if (rank == 0) {
        bool ok = resultado_correto(vetor);
        std::printf("RESULTADO,mpi_arvore,%d,%d,%d,%.9f,%s\n",
                    n, processos, delta, segundos, ok ? "OK" : "ERRO");
        if (!ok) MPI_Abort(MPI_COMM_WORLD, 2);
    }

    if (stats) {
        double medidas[3] = {estatisticas.tempo_bubble,
                             estatisticas.tempo_intercalacao,
                             estatisticas.tempo_comunicacao};
        std::vector<double> todos;
        std::vector<int> todos_elementos;
        if (rank == 0) {
            todos.resize(3 * processos);
            todos_elementos.resize(processos);
        }
        MPI_Gather(medidas, 3, MPI_DOUBLE, rank == 0 ? todos.data() : nullptr,
                   3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&estatisticas.elementos_bubble, 1, MPI_INT,
                   rank == 0 ? todos_elementos.data() : nullptr,
                   1, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank == 0) {
            for (int r = 0; r < processos; ++r) {
                std::printf("STATS,mpi_arvore,%d,%d,%d,%d,%.9f,%.9f,%.9f\n",
                            n, processos, r, todos_elementos[r],
                            todos[3*r], todos[3*r+1], todos[3*r+2]);
            }
        }
    }
    MPI_Finalize();
    return 0;
}
