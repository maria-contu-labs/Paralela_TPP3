#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include "ordenacao.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::fprintf(stderr, "Uso: %s <numero_de_elementos>\n", argv[0]);
        return 1;
    }
    char* fim = nullptr;
    long long lido = std::strtoll(argv[1], &fim, 10);
    if (*fim != '\0' || lido < 1 || lido > std::numeric_limits<int>::max()) {
        std::fprintf(stderr, "Tamanho do vetor invalido.\n");
        return 1;
    }
    int n = static_cast<int>(lido);
    std::vector<int> vetor(n);
    preencher_reverso(vetor);

    auto inicio = std::chrono::steady_clock::now();
    bubble_sort(vetor);
    auto fim_tempo = std::chrono::steady_clock::now();
    double segundos = std::chrono::duration<double>(fim_tempo - inicio).count();

    bool ok = resultado_correto(vetor);
    std::printf("RESULTADO,sequencial,%d,1,%d,%.9f,%s\n",
                n, n, segundos, ok ? "OK" : "ERRO");
    return ok ? 0 : 2;
}
