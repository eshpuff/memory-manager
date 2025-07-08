#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include "uthash.h"

#define MAX_PAGE_ID_LEN 20
#define PAGE_SIZE_BYTES 4096 // tamanho da pag em bytes (4kb)

// estrutura para armazenar a seq de acessos
typedef struct {
    char page_id_str[MAX_PAGE_ID_LEN];
    int page_id_int;
    UT_hash_handle hh;
} PageMapEntry;

typedef struct {
    int page_id_int;
} PageAccess;

// função aux que converte a string de tamanho de mem para bytes
long long parseMemorySize(const char *sizeStr) {
    long long value;
    char unit[5] = "";

    sscanf(sizeStr, "%lld%4s", &value, unit);
    for (int i = 0; unit[i]; i++) unit[i] = tolower(unit[i]);
    
    if (strcmp(unit, "kb") == 0) return value * 1024LL;
    if (strcmp(unit, "mb") == 0) return value * 1024LL * 1024LL;
    if (strcmp(unit, "gb") == 0) return value * 1024LL * 1024LL * 1024LL;
    if (strcmp(unit, "b") == 0 || unit[0] == '\0') return value;
    return -1; // unidade desconhecida
}

// SIMULAÇÃO DO ALGORITMO OTIMO
int runOptimalSimulation(PageAccess *accessSequence, int numAccesses, int numPhysicalPages, int distinctPageCount) {
    if (numPhysicalPages == 0) return numAccesses;

    int pageFaults = 0;
    int *frames = malloc(numPhysicalPages * sizeof(int));
    bool *inMemoryFlags = calloc(distinctPageCount, sizeof(bool));
    
    for(int i = 0; i < numPhysicalPages; i++) frames[i] = -1; 

    for (int i = 0; i < numAccesses; i++) {
        int currentPageId = accessSequence[i].page_id_int;
        if (inMemoryFlags[currentPageId]) {
            continue; 
        }

        pageFaults++;
        int emptySlot = -1;
        for (int j = 0; j < numPhysicalPages; j++) {
            if (frames[j] == -1) {
                emptySlot = j;
                break;
            }
        }

        if (emptySlot != -1) {
            frames[emptySlot] = currentPageId;
            inMemoryFlags[currentPageId] = true;
        } else {
            int victim = -1;
            int farthest = i;
            for (int j = 0; j < numPhysicalPages; j++) {
                int nextUse = INT_MAX;
                for (int k = i + 1; k < numAccesses; k++) {
                    if (frames[j] == accessSequence[k].page_id_int) {
                        nextUse = k;
                        break;
                    }
                }
                if (nextUse > farthest) {
                    farthest = nextUse;
                    victim = j;
                }
            }
            if (victim == -1) {
                victim = 0;
            }
            inMemoryFlags[frames[victim]] = false;
            frames[victim] = currentPageId;
            inMemoryFlags[currentPageId] = true;
        }
    }

    free(frames);
    free(inMemoryFlags);
    return pageFaults;
}

// ALGORITMO FIFO
int runFifoSimulation(PageAccess *accessSequence, int numAccesses, int numPhysicalPages, int distinctPageCount) {
    if (numPhysicalPages == 0) return numAccesses;

    int pageFaults = 0;
    int fifoPointer = 0;
    int *frames = malloc(numPhysicalPages * sizeof(int));
    bool *inMemoryFlags = calloc(distinctPageCount, sizeof(bool));
    for(int i = 0; i < numPhysicalPages; i++) frames[i] = -1;

    int pagesInMemory = 0;

    for (int i = 0; i < numAccesses; i++) {
        int currentPageId = accessSequence[i].page_id_int;

        if (inMemoryFlags[currentPageId]) {
            continue; // Page Hit
        }
        
        pageFaults++;

        if (pagesInMemory < numPhysicalPages) {
            frames[pagesInMemory] = currentPageId;
            inMemoryFlags[currentPageId] = true;
            fifoPointer = (pagesInMemory + 1) % numPhysicalPages;
            pagesInMemory++;
        } else {
            int victimPageId = frames[fifoPointer];
            inMemoryFlags[victimPageId] = false;
            
            frames[fifoPointer] = currentPageId;
            inMemoryFlags[currentPageId] = true;
            fifoPointer = (fifoPointer + 1) % numPhysicalPages;
        }
    }
    
    free(frames);
    free(inMemoryFlags);
    return pageFaults;
}

// FUNÇÃO MAIN 

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <arquivo_acessos.txt> <tamanho_memoria>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    long long physicalMemoryBytes = parseMemorySize(argv[2]);

    if (physicalMemoryBytes < 0) {
        fprintf(stderr, "ERRO: Tamanho de memória física inválido: %s\n", argv[2]);
        return 1;
    }
    int numPhysicalPages = physicalMemoryBytes / PAGE_SIZE_BYTES;

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("ERRO: Nao foi possivel abrir o arquivo de acessos");
        return 1;
    }

    PageAccess *accessSequence = malloc(sizeof(PageAccess) * 1000); // Capacidade inicial
    int numAccesses = 0;
    int capacity = 1000;

    PageMapEntry *pageMap = NULL;
    int distinctPageCount = 0;
    
    char line[256];
    char pageIdBuffer[MAX_PAGE_ID_LEN];

    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "%*d %s", pageIdBuffer) == 1 || sscanf(line, "%s", pageIdBuffer) == 1) {
            if (strcmp(pageIdBuffer, "...") == 0) continue;

            PageMapEntry *entry;
            HASH_FIND_STR(pageMap, pageIdBuffer, entry);

            if (entry == NULL) {
                entry = malloc(sizeof(PageMapEntry));
                strcpy(entry->page_id_str, pageIdBuffer);
                entry->page_id_int = distinctPageCount++;
                HASH_ADD_STR(pageMap, page_id_str, entry);
            }

            if (numAccesses >= capacity) {
                capacity *= 2;
                accessSequence = realloc(accessSequence, capacity * sizeof(PageAccess));
                if (!accessSequence) {
                    perror("Falha ao realocar memoria");
                    exit(EXIT_FAILURE);
                }
            }
            accessSequence[numAccesses++].page_id_int = entry->page_id_int;
        }
    }
    fclose(file);

    int optimalFaults = runOptimalSimulation(accessSequence, numAccesses, numPhysicalPages, distinctPageCount);
    int fifoFaults = runFifoSimulation(accessSequence, numAccesses, numPhysicalPages, distinctPageCount);

    double efficiency = (fifoFaults > 0) ? ((double)optimalFaults / fifoFaults) * 100.0 : 100.0;
    if (optimalFaults == fifoFaults) efficiency = 100.0;

    printf("A memória fisica comporta %d paginas\n", numPhysicalPages);
    printf("Existem %d paginas distintas no arquivo\n", distinctPageCount);
    printf("Com o algoritmo Otimo ocorrem %d faltas de pagina\n", optimalFaults);
    printf("Com o algoritmo FIFO ocorrem %d faltas de pagina\n", fifoFaults);
    printf("Atingindo %.2f%% do desempenho do Otimo.\n", efficiency);
    
    free(accessSequence);

    PageMapEntry *current_entry, *tmp;
    HASH_ITER(hh, pageMap, current_entry, tmp) {
        HASH_DEL(pageMap, current_entry);
        free(current_entry);
    }
    printf("Deseja ver a simulação do FIFO? (s/n): ");
    char choice;
    scanf(" %c", &choice);

    if (choice == 's' || choice == 'S') {
        printf("oiiiiiiiiiiiiiiiiiii baiszkizs");
    // logica p contart os carregamento de cada pagima
    }
    
    return 0;
}