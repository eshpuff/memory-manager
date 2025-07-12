#include "simulator.h"

int runOptimalSimulation(PageAccess * accessSequence, int numAccesses, int numPhysicalPages) {
    char **frames = malloc(numPhysicalPages * sizeof(char *));
    for (int i = 0; i < numPhysicalPages; i++) frames[i] = NULL; // inicializa os frames como vazios
    int pageFaults = 0;

    for (int i = 0; i < numAccesses; i++) {
        if (i > 0 && i % LOG_INTERVAL == 0) {
            printf("[otimo] processando acesso %d de %d...\n", i, numAccesses);
        }

        char *currentPage = accessSequence[i].page_id;
        int pageFound = 0;

        for (int j = 0; j < numPhysicalPages; j++) {
            if (frames[j] && strcmp(frames[j], currentPage) == 0) {
                pageFound = 1;
                break;
            }
        }

        char* victimPageId = NULL;
        int slotIndex = -1;

        if (!pageFound) {
            pageFaults++;
            // incementa o contador de carregamento pro otimo
            incrementLoadCount(currentPage, "optimal");

            if (g_verbose) {
                printf("[otimo] page fault #%d (acesso #%d): página '%s' não encontrada.\n", pageFaults, i + 1, currentPage);
            }

            int emptySlot = -1;
            for (int j = 0; j < numPhysicalPages; j++) {
                if (frames[j] == NULL) {
                    emptySlot = j;
                    break;
                }
            }

            if (emptySlot != -1) {
                frames[emptySlot] = currentPage;
                slotIndex = emptySlot; // guarda o slot para o log
                if (g_verbose) {
                    printf("inserido em: %d.\n", emptySlot);
                }
            } else {
                // mem cheia -> aplicao alg otimo para escolher qual substituir
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

                victimPageId = strdup(frames[victim]); // copia o nome da vítima p log
                slotIndex = victim; // guarda o slot pro log

                if (g_verbose) {
                    printf("substituindo página '%s' na posição %d por '%s'.\n", frames[victim], victim, currentPage);
                }

                frames[victim] = currentPage;
            }
        }

        if (g_didaticMode && g_verbose && !pageFound) {
            displayFrameState("otimo", numPhysicalPages, frames, currentPage, victimPageId, slotIndex);
        }

        free(victimPageId); // libera a memória da cópia se foi feita
    }

    free(frames);
    return pageFaults;
}
