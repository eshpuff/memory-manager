# Simulador de Gerenciamento de Memória Virtual

Este projeto é uma implementação em C de um simulador de algoritmos de substituição de página, desenvolvido para a disciplina de Sistemas Operacionais. O programa calcula e compara o número de faltas de página (page faults) para os algoritmos **Ótimo** e **FIFO**.

---

### Sobre o Projeto

O objetivo deste trabalho é simular o comportamento do gerenciador de memória de um sistema operacional. O programa recebe como entrada um arquivo de texto contendo uma sequência de acessos a páginas de memória e o tamanho total da memória física disponível para a simulação

A partir desses dados, ele executa dois algoritmos de substituição de página:
1.  **Algoritmo Ótimo**: O algoritmo ideal, que serve como base de comparação. Ele sempre substitui a página que levará mais tempo para ser acessada no futuro.
2.  **Algoritmo FIFO (First-In, First-Out)**: Um algoritmo simples que substitui a página que está há mais tempo na memória.

Ao final, o programa apresenta um relatório comparativo de desempenho entre os dois algoritmos.

---

### 🛠 Pré-requisitos

Para compilar e executar este projeto, tu vai precisar de:
* `gcc` (GNU Compiler Collection)
* `make`

---

### ⚙️ Como Compilar

O projeto utiliza um `Makefile` para simplificar o processo. Para compilar, basta executar o seguinte comando no terminal, na raiz do projeto:

```bash
make
```

Isso vai compilar todos os arquivos-fonte da pasta `src/` e criar o executável em `bin/main.exe`.

---

### Como Executar

Após a compilação, execute o programa utilizando o seguinte formato:

```bash
./bin/main.exe <arquivo_de_entrada> <tamanho_memoria> [OPÇÃO]
```

**Argumentos:**
* `<arquivo_de_entrada>`: Caminho para o arquivo de texto com a sequência de acessos a páginas.
* `<tamanho_memoria>`: Tamanho da memória física a ser simulada. [cite_start]Suporta os sufixos `KB`, `MB`, `GB` (e.g., `8MB`, `1GB`, `32KB`).
* `[OPÇÃO]`:
    * `-v`: Ativa os logs em tempo real.

---

### 🚀 Exemplos de Uso

**Exemplo 1: Execução padrão**

```bash
./bin/main.exe acessos.txt 8MB
```

**Saída Esperada:**
```
executando o fifo...
[FIFO] processando acesso 50000...

executando o ótimo...
[otimo] iniciando pre-processamento do arquivo de referencias...
[otimo] pre-processamento concluido.
[otimo] processando acesso 50000 de 100000...

RELATÓRIO:
a memória física comporta 2048 páginas.
há 15000 páginas distintas no arquivo.
estimativa do tamanho da tabela de páginas (1 nível): 120000 bytes (117.19 KB).
com o algoritmo ÓTIMO ocorrem 16385 faltas de página.
com o algoritmo FIFO ocorrem 20137 faltas de página,
desempenho do FIFO em relação ao OTIMO: 81.36%

deseja listar o número de carregamentos (s/n)? n

terminou!
```

**Exemplo 2: Listando os carregamentos**

```bash
./bin/main.exe acessos.txt 8MB
```

**Saída (após escolher 's'):**
```
...
deseja listar o número de carregamentos (s/n)? s

--- carregamentos por página ---
página     otimo      fifo
----------------------------------
I0         2          57
D0         1          1
D2         1          1
...
```

---

### 📂 Estrutura do Projeto

```
.
├── bin/
│   └── main.exe
├── include/
│   └── simulator.h
├── src/
│   ├── main.c
│   ├── hash.c
│   ├── optimal.c
│   └── utils.c
├── Makefile
└── README.md
```

* **`bin/`**: Contém os arquivos executáveis após a compilação.
* **`include/`**: Contém os arquivos de cabeçalho (`.h`).
* **`src/`**: Contém os arquivos de código-fonte (`.c`).
* **`Makefile`**: Arquivo com as regras para compilação e limpeza do projeto.

---
