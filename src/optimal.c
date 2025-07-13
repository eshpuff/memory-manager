#include "simulator.h"

// funçao aux pra verificar presença usando a tabela hash aux
static int isPageInFrames(PresenceNode* presenceMap[], const char* page_id) {
    unsigned int index = hashOptimize(page_id);
    PresenceNode* current = presenceMap[index];

    while(current) {
        if(strcmp(current->page_id, page_id) == 0) return 1;
        current = current->next;
    }
    return 0;
}

// função aux para adicionar ou remover da tabela de presença
static void updatePresence(PresenceNode* presenceMap[], const char* page_id, int add) {
    unsigned int index = hashOptimize(page_id);

    if (add) {
        PresenceNode* newNode = (PresenceNode*)malloc(sizeof(PresenceNode));
        if (!newNode) {
            perror("falha ao alocar memoria para nodo de presença");
            exit(1);
        }

        strcpy(newNode->page_id, page_id);
        newNode->next = presenceMap[index];
        presenceMap[index] = newNode;
    } else {
        PresenceNode* current = presenceMap[index];
        PresenceNode* prev = NULL;


        while(current) {
            if(strcmp(current->page_id, page_id) == 0) {
                if(prev) prev->next = current->next;
                else presenceMap[index] = current->next;
                free(current);
                return;
            }

            prev = current;
            current = current->next;
        }
    }
}

// função auxiliar do otimo para encontrar o prox uso de uma pag
static int getNextUse(HashNode* node, int currentIndex) {
    // Avança o cursor ate encontrar um uso futuro que seja depois do indice atual
    while (node->nextUsePointer < node->numFutureUses &&
           node->futureUses[node->nextUsePointer] <= currentIndex) {
        node->nextUsePointer++;
    }

    // se ainda tiver usos futuros retorna o proximo
    if (node->nextUsePointer < node->numFutureUses) {
        return node->futureUses[node->nextUsePointer];
    }

    return INT_MAX;
}

int runOptimalSimulation(PageAccess * accessSequence, int numAccesses, int numPhysicalPages) {
    char **frames = malloc(numPhysicalPages * sizeof(char *));
    for (int i = 0; i < numPhysicalPages; i++) frames[i] = NULL;
    int pageFaults = 0;

    PresenceNode* presenceMap[HASH_TABLE_SIZE];
    for(int i = 0; i < HASH_TABLE_SIZE; i++) presenceMap[i] = NULL;


    for (int i = 0; i < numAccesses; i++) {
        if (i > 0 && i % LOG_INTERVAL == 0) {
            printf("[otimo] processando acesso %d de %d...\n", i, numAccesses);
        }

        char *currentPage = accessSequence[i].page_id;
        int pageFound = isPageInFrames(presenceMap, currentPage);

        char* victimPageId = NULL;
        int slotIndex = -1;

        if (!pageFound) {
            pageFaults++;
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
                updatePresence(presenceMap, currentPage, 1); // adiciona na tabela de presença
                slotIndex = emptySlot;
                if (g_verbose) {
                    printf("inserido em: %d.\n", emptySlot);
                }
            } else {
                int victim = -1;
                int farthest = -1;

                for (int j = 0; j < numPhysicalPages; j++) {
                    HashNode* frameNode = findNode(frames[j]);
                    int nextUse = getNextUse(frameNode, i);

                    if (nextUse == INT_MAX) {
                        victim = j;
                        break;
                    }

                    if (nextUse > farthest) {
                        farthest = nextUse;
                        victim = j;
                    }
                }

                victimPageId = strdup(frames[victim]);
                slotIndex = victim;

                if (g_verbose) {
                    printf("substituindo página '%s' na posição %d por '%s'.\n", frames[victim], victim, currentPage);
                }
                
                updatePresence(presenceMap, frames[victim], 0); // Remove vítima
                frames[victim] = currentPage;
                updatePresence(presenceMap, currentPage, 1);     // Adiciona nova página
            }
        }

        if (g_didaticMode && g_verbose && !pageFound) {
            displayFrameState("otimo", numPhysicalPages, frames, currentPage, victimPageId, slotIndex);
        }

        free(victimPageId);
    }
    
    // limpeza da tabela hash de presença
    for(int i = 0; i < HASH_TABLE_SIZE; i++) {
        PresenceNode* current = presenceMap[i];
        while(current) {
            PresenceNode* temp = current;
            current = current->next;
            free(temp);
        }
    }

    free(frames);
    return pageFaults;
}