#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include "ordenacao.h"

// Versao de comparacao CONTROLADA, com Bubble Sort em TODOS os processos.
// Divisao equilibrada via Scatterv e conquista por intercalação binaria via MPI.
// Nao e o codigo do GitHub (que usa outro algoritmo de ordenacao).

constexpr int TAG_TAMANHO = 21;
constexpr int TAG_DADOS = 22;

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank = 0, processos = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processos);
    if (argc < 2 || argc > 3) {
        if (rank == 0) std::fprintf(stderr, "Uso: mpirun -np P ./mpi_balanceado N [--stats]\n");
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
    if ((argc == 3 && !stats) || processos > n) {
        if (rank == 0) std::fprintf(stderr, "Opcao invalida ou numero de processos maior que N.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    std::vector<int> entrada;
    std::vector<int> tamanhos(processos), deslocamentos(processos);
    int pos = 0;
    for (int r = 0; r < processos; ++r) {
        tamanhos[r] = n / processos + (r < n % processos ? 1 : 0);
        deslocamentos[r] = pos;
        pos += tamanhos[r];
    }
    if (rank == 0) {
        entrada.resize(n);
        preencher_reverso(entrada);
    }

    int tamanho_local = tamanhos[rank];
    std::vector<int> vetor(tamanho_local);
    double t_bubble = 0.0, t_merge = 0.0, t_comunicacao = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);
    double inicio_total = MPI_Wtime();
    double t = MPI_Wtime();
    MPI_Scatterv(rank == 0 ? entrada.data() : nullptr,
                 tamanhos.data(), deslocamentos.data(), MPI_INT,
                 vetor.data(), tamanho_local, MPI_INT, 0, MPI_COMM_WORLD);
    t_comunicacao += MPI_Wtime() - t;

    t = MPI_Wtime();
    bubble_sort(vetor);
    t_bubble += MPI_Wtime() - t;

    // Reduce binario: cada rank combina seu vetor com o vizinho do grupo.
    for (int passo = 1; passo < processos; passo *= 2) {
        if (rank % (2 * passo) == passo) {
            int destinatario = rank - passo;
            int tamanho = static_cast<int>(vetor.size());
            t = MPI_Wtime();
            MPI_Send(&tamanho, 1, MPI_INT, destinatario, TAG_TAMANHO, MPI_COMM_WORLD);
            MPI_Send(vetor.data(), tamanho, MPI_INT, destinatario, TAG_DADOS, MPI_COMM_WORLD);
            t_comunicacao += MPI_Wtime() - t;
            break;
        }
        if (rank % (2 * passo) == 0 && rank + passo < processos) {
            int origem = rank + passo;
            int tamanho_recebido = 0;
            t = MPI_Wtime();
            MPI_Recv(&tamanho_recebido, 1, MPI_INT, origem, TAG_TAMANHO,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            const int tamanho_esquerda = static_cast<int>(vetor.size());
            vetor.resize(tamanho_esquerda + tamanho_recebido);
            MPI_Recv(vetor.data() + tamanho_esquerda, tamanho_recebido, MPI_INT,
                     origem, TAG_DADOS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            t_comunicacao += MPI_Wtime() - t;
            t = MPI_Wtime();
            intercalar(vetor, static_cast<std::size_t>(tamanho_esquerda));
            t_merge += MPI_Wtime() - t;
        }
    }
    double segundos = MPI_Wtime() - inicio_total;
    if (rank == 0) {
        bool ok = resultado_correto(vetor);
        std::printf("RESULTADO,mpi_balanceado,%d,%d,%d,%.9f,%s\n",
                    n, processos, (n + processos - 1) / processos,
                    segundos, ok ? "OK" : "ERRO");
        if (!ok) MPI_Abort(MPI_COMM_WORLD, 2);
    }
    if (stats) {
        double medidas[3] = {t_bubble, t_merge, t_comunicacao};
        std::vector<double> todos;
        if (rank == 0) todos.resize(3 * processos);
        MPI_Gather(medidas, 3, MPI_DOUBLE,
                   rank == 0 ? todos.data() : nullptr,
                   3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        if (rank == 0) {
            for (int r = 0; r < processos; ++r) {
                std::printf("STATS,mpi_balanceado,%d,%d,%d,%d,%.9f,%.9f,%.9f\n",
                            n, processos, r, tamanhos[r],
                            todos[3*r], todos[3*r+1], todos[3*r+2]);
            }
        }
    }
    MPI_Finalize();
    return 0;
}
