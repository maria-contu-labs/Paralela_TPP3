#ifndef ORDENACAO_H
#define ORDENACAO_H

#include <algorithm>
#include <cstddef>
#include <vector>

// Bubble Sort pedido no enunciado: vetor inversamente ordenado como pior caso.
inline void bubble_sort(std::vector<int>& vetor) {
    const std::size_t n = vetor.size();
    bool trocou = true;
    for (std::size_t i = 0; i + 1 < n && trocou; ++i) {
        trocou = false;
        for (std::size_t j = 0; j + i + 1 < n; ++j) {
            if (vetor[j] > vetor[j + 1]) {
                std::swap(vetor[j], vetor[j + 1]);
                trocou = true;
            }
        }
    }
}

inline void preencher_reverso(std::vector<int>& vetor) {
    for (std::size_t i = 0; i < vetor.size(); ++i) {
        vetor[i] = static_cast<int>(vetor.size() - i);
    }
}

// Intercala duas metades ordenadas, inclusive se o tamanho for impar.
inline void intercalar(std::vector<int>& vetor, std::size_t tam_esquerda) {
    std::vector<int> auxiliar(vetor.size());
    std::size_t i = 0, j = tam_esquerda, k = 0;
    while (i < tam_esquerda && j < vetor.size()) {
        if (vetor[i] <= vetor[j]) auxiliar[k++] = vetor[i++];
        else auxiliar[k++] = vetor[j++];
    }
    while (i < tam_esquerda) auxiliar[k++] = vetor[i++];
    while (j < vetor.size()) auxiliar[k++] = vetor[j++];
    vetor.swap(auxiliar);
}

inline bool resultado_correto(const std::vector<int>& vetor) {
    if (vetor.empty()) return false;
    if (vetor.front() != 1 || vetor.back() != static_cast<int>(vetor.size())) return false;
    for (std::size_t i = 1; i < vetor.size(); ++i) {
        // Verifica tambem preservacao de todos os elementos, sem repeticoes.
        if (vetor[i] != static_cast<int>(i + 1)) return false;
    }
    return true;
}

#endif
