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

## Atualizacao: etapa de validacao e versao de arvore com trabalho nos pais

A implementacao `mpi_arvore_balanceada.cpp` adapta a ESTRATEGIA de
`mpi_mergesort.c` (https://github.com/gary-funck/parallel-merge-sort):
cada rank delega a metade direita a um ajudante, mas CONTINUA A ORDENAR
localmente a metade esquerda. Assim, ranks internos tambem fazem Bubble Sort,
ao somente os ranks folha. Ao contrario do Merge Sort do artigo,
a ordenacao local permanece Bubble Sort; isso melhora a comparacao do efeito
da estrategia de trabalho. Nesta variante, delta=1; tambem ocorre conquista quando nao ha mais ranks ajudantes. Para garantir que todos os processos da arvore recebam trabalho, exige-se N >= P+1 para P>1.

- `mpi_arvore.cpp`: arvore do pseudocodigo do professor: so folhas ordenam.
- `mpi_arvore_balanceada.cpp`: arvore com pais que tambem ordenam (estrategia
  adaptada da referencia, nao e copia do codigo original).
- `mpi_balanceado.cpp`: todos ordenam partes quase iguais, Scatterv e reducao.

Para compilar tudo: `make`

Teste funcional local (contagens impares etc; apenas P ate 7):
`bash verificar_correcao.sh`

Teste funcional estendido apenas em alocacao que comporte P=31:
`TESTE_GRANDE=1 bash verificar_correcao.sh`

Testes exploratorios no Mac (3 repeticoes, 100.000 elementos, ate 7 ranks):
`bash testes_locais.sh 3 100000`

Ao entrar numa alocacao com os dois nos exclusivos na Atlantica,
sem oversubscription:
`CONFIRMO_ALOCACAO=1 bash testes_atlantica.sh 10000 3`

Mais tarde repita para N=100000 e N=1000000 conforme tempo e limites da
alocacao. N=1000000 com Bubble Sort sequencial pode demorar muitos minutos;
nao execute esse teste sem reservar tempo de cluster suficiente.

As saidas ficam em `resultados/`, incluindo CSV de tempo mediano,
speed-up e eficiencia. Registre sistema, CPU, compilador, MPI e politica de
mapeamento dos ranks. Os casos P=63..1023 necessitam planejamento de
oversubscription permitido pelo professor e pelos administradores do cluster.

### Limitacoes para descrever no relatorio

- O codigo original do GitHub usa Merge Sort recursivo com Insertion Sort para
  vetores pequenos e entrada pseudoaleatoria. Comparar seu tempo diretamente
  com Bubble Sort sobre entrada inversa nao isola o balanceamento.
- Comparacoes entre as TRES variantes Bubble Sort tampouco isolam totalmente
  balanceamento: a distribuicao em blocos muda o numero de comparacoes e
  trocas executadas pelo Bubble Sort quadratico.
- Superlinearidade (speed-up maior que P) pode ocorrer por mudanca no trabalho
  e nos caches, nao e prova de escalonamento milagroso.
- Evite comparar tempos do Mac com os da Atlantica. Sempre compare na mesma
  maquina e configuracao, e preferencialmente use mediana de repeticoes.
- A versao de arvore balanceada ainda requer validacao MPI real: o pacote
  foi analisado estaticamente aqui, mas este ambiente nao possui mpirun/mpic++.

## Analise de carga por rank/nivel

```bash
bash testar_balanceamento.sh 10000 7
```

O arquivo `resultados/balanceamento_N10000_P7_por_nivel.csv` resume quantidade
de elementos Bubble Sort e maiores tempos por faixa de ranks. Na versao
`mpi_arvore_balanceada`, o nivel calculado a partir do numero do rank e uma
FAIXA DE RANKS, nao a profundidade exata da arvore de ajudantes; para
analise rigorosa da profundidade, usar o mapeamento pai/filho de cada versao.
