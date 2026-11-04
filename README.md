# TPP3 — MPI, Divisao e Conquista (D&C)

## Arquivos

- `ordenacao.h`: Bubble Sort, vetor em ordem reversa e intercalação segura (inclusive N ímpar).
- `sequencial.cpp`: versão sequencial Bubble Sort (referência).
- `mpi_arvore.cpp`: versão SPMD do pseudocódigo, com árvore binária completa. Somente as folhas fazem Bubble Sort.
- `mpi_balanceado.cpp`: comparação controlada adicional. Todos os ranks fazem Bubble Sort em parcelas equivalentes e depois participam de intercalações por redução em árvore. **Não é a implementação do GitHub.**
- `Makefile`: compilação com `g++` e `mpic++`.
- `executar_amostra.sh`: testes pequenos para validar a correção.
- `medicoes.sh`: comandos de exemplo para medições; executar APENAS em alocação apropriada.
- `calcular_metricas.py`: converte resultados em CSV e calcula speed-up / eficiência.

## Compilar

```bash
make
```

## Testar localmente (com MPI instalado)

```bash
./sequencial 1000
mpirun -np 3 ./mpi_arvore 1000 --stats
mpirun -np 3 ./mpi_balanceado 1000 --stats
```

Saída de cada programa:

```text
RESULTADO,versao,N,processos,delta,segundos,OK
```

Saída opcional de estatísticas (um registro por processo):

```text
STATS,versao,N,processos,rank,elementos_ordenados_por_bubble,tempo_bubble,tempo_intercalacao,tempo_MPI_e_espera
```

`delta` é o tamanho máximo do vetor antes da ordenação em folha na árvore básica. Na versão balanceada, a coluna mostra o tamanho máximo dos blocos recebidos por processo.

### Parâmetros do enunciado

- `N`: 10.000, 100.000 e 1.000.000.
- `P` inicial: 1, 3, 7, 15 e 31 processos (31 envolve HT se a alocação for conforme solicitado).
- `P` extra: 63, 127, 255, 511, 1023 processos. **Exigem oversubscription ou alocação específica**, e por isso serão potencialmente muito mais lentos; não executar sem verificar regras do cluster.
- Usar dois nós exclusivos, segundo as condições do enunciado, e repetição de medições em condições comparáveis.
- Medir o tempo de ordenação no processo raiz (inclusive distribuição, intercalação e esperas; exclui alocação e inicialização do vetor), do mesmo modo nas duas versões MPI.
- Na sequencial, medir apenas o Bubble Sort, com vetor inicial criado fora da medição.
- A versão com 1 rank permite avaliar o overhead MPI da versão base sem comunicação efetiva, além do sequencial puro.

## Versão indicada pelo professor (separada)

O repositório `https://github.com/gary-funck/parallel-merge-sort` contém `mpi_mergesort.c` e `get_time.c`.
Esse algoritmo utiliza **Merge Sort** (e insertion sort em subvetores muito pequenos), e cria trabalho paralelo de modo diferente. Sua execução pode demonstrar a alternativa proposta pelo enunciado, mas a diferença em tempos NÃO representa apenas ganho de balanceamento, pois inclui mudança de algoritmo e de dados de entrada (a versão do repositório usa dados pseudoaleatórios; esta base usa ordem reversa).

Como base para executar separadamente, com código original e citação da licença GNU GPL:

```bash
git clone https://github.com/gary-funck/parallel-merge-sort.git referencia
cd referencia
mpicc -O3 mpi_mergesort.c get_time.c -lm -o mpi_mergesort
mpirun -np 7 ./mpi_mergesort 10000
```

Para comparação direta, padronizar os dados de entrada; considerar ainda que Bubble Sort quadrático e Merge Sort O(N log N) têm complexidades diferentes.

## Calculadora de speed-up / eficiência

Rode os experimentos gravando saídas em um arquivo só, por exemplo:

```bash
./sequencial 10000 | tee resultados.log
mpirun -np 3 ./mpi_arvore 10000 | tee -a resultados.log
mpirun -np 3 ./mpi_balanceado 10000 | tee -a resultados.log
python3 calcular_metricas.py resultados.log > metricas.csv
```

Speed-up: `Tseq(N) / Tpar(N,P)`. Eficiência: `speedup / P`. Para medidas com varias repetições, usar uma medida representativa consistente (ex.: mediana), e não misturar execuções locais com o cluster.

## Cuidados

- Evitar debug/prints do vetor nas medidas, como pede o enunciado.
- Não usar `std::sort` no lugar do Bubble Sort.
- A `intercalar` fornecida no PDF tem condição que pode acessar índices fora do vetor; a nossa trata os limites primeiro.
- Vetores de tamanhos ímpares são divididos em `N/2` e `N-N/2`.
- `--stats` cria saídas para diagnóstico; não usá-la nas medições principais se quiser diminuir a perturbação.
- A máquina em que este pacote foi preparado não tem MPI instalado. O código sequencial foi compilado e executado; os programas MPI devem ser compilados e validados no ambiente com `mpic++` antes de serem utilizados no cluster.
