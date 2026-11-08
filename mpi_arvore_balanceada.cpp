#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include "ordenacao.h"

// Adaptacao da estrategia de delegacao de trabalho de mpi_mergesort.c,
// do repositorio gary-funck/parallel-merge-sort, mantendo Bubble Sort nas folhas.
// Diferenca em relacao a mpi_arvore: o proprio processo continua ordenando
// a metade esquerda enquanto o ajudante ordena a metade direita.
// A implementacao e original e nao copia codigo do repositorio externo.

constexpr int TAG_TRABALHO = 31;
constexpr int TAG_RESULTADO = 32;

struct Estatisticas {
    double tempo_bubble = 0.0;
    double tempo_intercalacao = 0.0;
    double tempo_comunicacao = 0.0;
    int elementos_bubble = 0;
};

// Rank 0 comeca no nivel 0. Os demais comecam em ceil(log2(rank+1)).
// Segue o mapeamento de ajudantes da referencia: rank + 2^nivel.
int nivel_inicial(int rank) {
    int nivel = 0;
    long long potencia = 1;
    while (potencia <= rank) {
        potencia *= 2;
        ++nivel;
    }
    return nivel;
}

void ordenar_arvore_balanceada(std::vector<int>& vetor, int rank, int processos,
                              int nivel, int delta, Estatisticas& est) {
    const long long ajudante = static_cast<long long>(rank) + (1LL << nivel);
    if (static_cast<int>(vetor.size()) <= delta || ajudante >= processos) {
        double inicio = MPI_Wtime();
        bubble_sort(vetor);
        est.tempo_bubble += MPI_Wtime() - inicio;
        est.elementos_bubble += static_cast<int>(vetor.size());
        return;
    }

    const std::size_t meio = vetor.size() / 2;
    std::vector<int> direita(vetor.begin() + meio, vetor.end());
    vetor.resize(meio);

    // Envia a segunda metade, mas segue trabalhando localmente na primeira.
    double inicio = MPI_Wtime();
    MPI_Send(direita.data(), static_cast<int>(direita.size()), MPI_INT,
             static_cast<int>(ajudante), TAG_TRABALHO, MPI_COMM_WORLD);
    est.tempo_comunicacao += MPI_Wtime() - inicio;

    ordenar_arvore_balanceada(vetor, rank, processos, nivel + 1, delta, est);

    inicio = MPI_Wtime();
    MPI_Recv(direita.data(), static_cast<int>(direita.size()), MPI_INT,
             static_cast<int>(ajudante), TAG_RESULTADO, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
    est.tempo_comunicacao += MPI_Wtime() - inicio;

    const std::size_t esquerda = vetor.size();
    vetor.insert(vetor.end(), direita.begin(), direita.end());
    inicio = MPI_Wtime();
    intercalar(vetor, esquerda);
    est.tempo_intercalacao += MPI_Wtime() - inicio;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank = 0, processos = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);

    if (argc < 2 || argc > 3) {
        if (rank == 0) std::fprintf(stderr,
            "Uso: mpirun -np P ./mpi_arvore_balanceada N [--stats]\n");
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
    if ((argc == 3 && !stats) ||
        (processos & (processos + 1)) != 0 ||
        (processos > 1 && n < processos + 1)) {
        if (rank == 0) std::fprintf(stderr,
            "Opcao invalida. Use P=1,3,7,... e N >= P+1 para P>1.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    const int delta = 1; // Consome os ajudantes disponiveis; folha sem ajudante executa Bubble Sort.
    std::vector<int> vetor;
    if (rank == 0) {
        vetor.resize(n);
        preencher_reverso(vetor);
    }
    Estatisticas est;
    MPI_Barrier(MPI_COMM_WORLD);
    const double inicio_total = MPI_Wtime();

    if (rank != 0) {
        MPI_Status status;
        double inicio = MPI_Wtime();
        MPI_Probe(MPI_ANY_SOURCE, TAG_TRABALHO, MPI_COMM_WORLD, &status);
        int tamanho = 0;
        MPI_Get_count(&status, MPI_INT, &tamanho);
        const int pai = status.MPI_SOURCE;
        vetor.resize(tamanho);
        MPI_Recv(vetor.data(), tamanho, MPI_INT, pai, TAG_TRABALHO,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        est.tempo_comunicacao += MPI_Wtime() - inicio;

        ordenar_arvore_balanceada(vetor, rank, processos, nivel_inicial(rank),
                                 delta, est);

        inicio = MPI_Wtime();
        MPI_Send(vetor.data(), static_cast<int>(vetor.size()), MPI_INT,
                 pai, TAG_RESULTADO, MPI_COMM_WORLD);
        est.tempo_comunicacao += MPI_Wtime() - inicio;
    } else {
        ordenar_arvore_balanceada(vetor, rank, processos, 0, delta, est);
    }

    const double segundos = MPI_Wtime() - inicio_total;
    if (rank == 0) {
        const bool ok = resultado_correto(vetor);
        std::printf("RESULTADO,mpi_arvore_balanceada,%d,%d,%d,%.9f,%s\n",
                    n, processos, delta, segundos, ok ? "OK" : "ERRO");
        if (!ok) MPI_Abort(MPI_COMM_WORLD, 2);
    }

    if (stats) {
        double medidas[3] = {est.tempo_bubble, est.tempo_intercalacao,
                             est.tempo_comunicacao};
        std::vector<double> todos;
        std::vector<int> elementos;
        if (rank == 0) {
            todos.resize(3 * processos);
            elementos.resize(processos);
        }
        MPI_Gather(medidas, 3, MPI_DOUBLE,
                   rank == 0 ? todos.data() : nullptr,
                   3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Gather(&est.elementos_bubble, 1, MPI_INT,
                   rank == 0 ? elementos.data() : nullptr,
                   1, MPI_INT, 0, MPI_COMM_WORLD);
        if (rank == 0) {
            for (int r = 0; r < processos; ++r) {
                std::printf("STATS,mpi_arvore_balanceada,%d,%d,%d,%d,%.9f,%.9f,%.9f\n",
                            n, processos, r, elementos[r],
                            todos[3*r], todos[3*r+1], todos[3*r+2]);
            }
        }
    }
    MPI_Finalize();
    return 0;
}
