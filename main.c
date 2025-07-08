#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_PAGE_ID_LEN 10 
#define PAGE_SIZE_BYTES 4096 // tamanho da pag em bytes (4kb)


// estrutura para armazenar a seq de acessos
typedef struct {
    char page_id[MAX_PAGE_ID_LEN];
} PageAccess;

// funçao aux que converte a string de tamanho de mem para bytes
long long parseMemorySize(const char * sizeStr) {
    long long value;
    char unit[5] = "";

    if (sscanf(sizeStr, "%lld%4s", &value, unit) < 1) return -1; // erro na conversão
    for (int i = 0; unit[i]; i++) unit[i] = tolower(unit[i]); // converte para minusculo

    // converte a unidade para bytes
    if(strcmp(unit, "kb") == 0) return value * 1024LL;
    if(strcmp(unit, "mb") == 0) return value * 1024LL * 1024LL;
    if(strcmp(unit, "gb") == 0) return value * 1024LL * 1024LL * 1024LL;
    if(strcmp(unit, "b") == 0) return value; // bytes
    return -1; // unidade desconhecida
}

int countDistinctPages(PageAccess * accessSequence, int numAccesses) {
    // if (numAccesses == 0) return 0;
    char ** distinctPages = malloc(numAccesses * sizeof(char *));
    int count = 0;

    for (int i = 0; i < numAccesses; i++) {
        int found = 0;
        for (int j = 0; j < count; j++) {
            if (strcmp(distinctPages[j], accessSequence[i].page_id) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            distinctPages[count++] = accessSequence[i].page_id;
            // count++;
        }
    }
    free(distinctPages);
    return count;
}

// SIMULAÇÃO DO ALGORITMO OTIMO
int runOptimalSimulation(PageAccess * accessSequence, int numAccesses, int numPhysicalPages) {
    char **frames = malloc(numPhysicalPages * sizeof(char *));
    for (int i= 0; i < numPhysicalPages; i++) frames[i] = NULL; // inicializa os frames como vazios
    int pageFaults = 0;

    for (int i = 0; i < numAccesses; i++) {
        char *currentPage = accessSequence[i].page_id;
        int pageFound = 0;

        for (int j = 0; j < numPhysicalPages; j++) {
            if (frames[j] && strcmp(frames[j], currentPage) == 0) {
                pageFound = 1;
                break;
            }
        }

        if (!pageFound) {
            pageFaults++;
            int emptySlot = -1;
            for (int j = 0; j < numPhysicalPages; j++) {
                if (frames[j] == NULL) {
                    emptySlot = j;
                    break;
                }
            }

            if (emptySlot != -1) {
                frames[emptySlot] = currentPage;
            } else {
                // mem cheia -> aplica o alg otimo
                int victim = -1;
                int farthest = -1;

                for (int j = 0; j < numPhysicalPages; j++) {
                    int next = INT_MAX;
                    for (int k = i + 1; k < numAccesses; k++) {
                        if (strcmp(frames[j], accessSequence[k].page_id) == 0) {
                            next = k;
                            break;
                        }
                    }

                    if (next > farthest) {
                        farthest = next;
                        victim = j;
                    }
                }
                frames[victim] = currentPage;
            }
        }
    }
    free(frames);
    return pageFaults;
}

// SIMULAÇÃO DO ALGORITMO FIFO
int runFifoSimulation(PageAccess * accessSequence, int numAccesses, int numPhysicalPages) {
    char **frames = malloc(numPhysicalPages * sizeof(char *));
    for (int i = 0; i < numPhysicalPages; i++) frames[i] = NULL; // inicializa os frames como vazios
    int pageFaults = 0;
    int fifoPointer = 0;

    for (int i = 0; i < numAccesses; i++) {
        char *currentPage = accessSequence[i].page_id;
        int pageFound = 0;

        for (int j = 0; j < numPhysicalPages; j++) {
            if (frames[j] && strcmp(frames[j], currentPage) == 0) {
                pageFound = 1;
                break;
            }
        }

        if (!pageFound) {
            pageFaults++;
            frames[fifoPointer] = currentPage;
            fifoPointer = (fifoPointer + 1) % numPhysicalPages; // avança
        }
    }
    free(frames);
    return pageFaults;
}

// FUNÇÃO MAIN
int main(int argc, char *argv[]) {
    // eespera os argumentos: ./programa <arquivo> <tamanho_memoria>
    if (argc < 3) {
        printf("Uso: %s <arquivo_acessos.txt> <tamanho_memoria_fisica>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("[ERRO] ao abrir o arquivo.");
        return 1;
    }

    //parsea o tamanho da memória física
    long long memBytes = parseMemorySize(argv[2]);
    if (memBytes < PAGE_SIZE_BYTES) {
        fprintf(stderr, "[ERRO] tamanho de memória física inválido: %s\n", argv[2]);
        fclose(file);
        return 1;
    }

    // if (physicalMemoryBytes == -1 || physicalMemoryBytes < PAGE_SIZE_BYTES) {
    //     fprintf(stderr, "[ERRO] tamanho de mem fisica inválido ou mt pequeno: %s\n", argv[2]);
    //     fclose(file);
    //     return 1;
    // }

    int numPages = memBytes / PAGE_SIZE_BYTES;

    //variaveis para armazenar os acessos
    PageAccess * accessSequence = malloc(100 * sizeof(PageAccess));
    int capacity = 100; // capacidade inicial para o array dinamico
    int numAccesses = 0;
    char line[256]; // buffer para ler cada linha do arquivo
    
    // accessSequence = (PageAccess *)malloc(capacity * sizeof(PageAccess));
    while (fgets(line, sizeof(line), file)) {
        char buffer[MAX_PAGE_ID_LEN];
        int lineNum;

        if (sscanf(line, "%d %s", &lineNum, buffer) == 2 || sscanf(line, "%s", buffer) == 1) {
            // sucesso! copia o id da pagina
            if (strcmp(buffer, "...") == 0) continue; // ignora linhas com apenas "..."
            strcpy(accessSequence[numAccesses].page_id, buffer);
            numAccesses++;
            if (numAccesses == capacity) {
                capacity *= 2; // dobra a capacidade se necessário
                accessSequence = realloc(accessSequence, capacity * sizeof(PageAccess));
            }
        }
    }
    fclose(file);

    //         if (sscanf(line, "%s", bufferPageId) == 1) {
    //         if (strcmp(bufferPageId, "...") == 0) {
    //             continue; // ignora linhas com apenas "..."
    //         }
    //         // sucesso! copia o id da pagina
    //         strcpy(accessSequence[numAccesses].page_id, bufferPageId);
    //         numAccesses++;
    //     }

    //     if (numAccesses == capacity) {
    //         // se a capacidade for atingida, reAloca o array
    //         capacity *= 2;
    //         // PageAccess * temp = (PageAccess *)realloc(accessSequence, capacity * sizeof(PageAccess));
    //         // if (temp == NULL) {
    //         //     perror("[ERRO] de realocaçao de mem para a seq de acessos");
    //         //     free(accessSequence);
    //         //     fclose(file);
    //         //     return 1;
    //         // }
    //         accessSequence = (PageAccess *)realloc(accessSequence, capacity * sizeof(PageAccess));
    //     }
    // }
    // fclose(file);

    // if (accessSequence == NULL) {
    //     perror("Erro de alocação de memória para a sequência de acessos");
    //     fclose(file);
    //     return 1;
    // }

        // numAccesses++;

        // redireciona o array se a capacidade for atingida
    
    int distinct = countDistinctPages(accessSequence, numAccesses);
    int optimalFaults = runOptimalSimulation(accessSequence, numAccesses, numPages);
    int fifoFaults = runFifoSimulation(accessSequence, numAccesses, numPages);

    double efficiency = (fifoFaults > 0) ? ((double)optimalFaults / fifoFaults) * 100.0 : 0.0;

    //resuktados
    printf("a memória fisica comporta %d paginas\n", numPages);
    printf("existem %d paginas distintas no aqruivo\n", distinct);
    printf("o algoritmo otimo teve %d faltas de pagina\n", optimalFaults);
    printf("o algoritmo FIFO teve %d faltas de pagina\n", fifoFaults);
    printf("atigindo %.2f%% do desempenho do algoritmo otimo\n", efficiency);

    printf("Deseja ver a simulação do FIFO? (s/n): ");
    char choice;
    scanf(" %c", &choice);

    if (choice == 's' || choice == 'S') {
        printf("oiiiiiiiiiiiiiiiiiii baiszkizs");
    // logica p contart os carregamento de cada pagima
    }

    free(accessSequence);
    return 0;
}