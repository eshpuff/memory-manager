#include "simulator.h"

// funçao aux pra verificar presença da pag usando a tabela hash aux
// otimizacao da rapidez do algoritmo
static int isPageInFrames(PresenceNode* presenceMap[], const char* page_id) {
    unsigned int index = hashOptimize(page_id);
    PresenceNode* current = presenceMap[index];

    // percorre a lista e procura a pagina 
    while(current) {
        if(strcmp(current->page_id, page_id) == 0) return 1; // achou
        current = current->next;
    }
    return 0; // n achou
}

// função aux para adicionar ou remover da tabela de presença
// manter controle de quantas pag tao nos quadros de pag 
static void updatePresence(PresenceNode* presenceMap[], const char* page_id, int add) {
    unsigned int index = hashOptimize(page_id); 

    if (add) { // adiciona a pag
        PresenceNode* newNode = (PresenceNode*)malloc(sizeof(PresenceNode));
        if (!newNode) { // verifica se deu p alocar
            perror("falha ao alocar memoria para nodo de presença");
            exit(1);
        }

        strcpy(newNode->page_id, page_id);
        newNode->next = presenceMap[index]; // insere no inicio da lista
        presenceMap[index] = newNode;
    } else { // remove a pag
        PresenceNode* current = presenceMap[index];
        PresenceNode* prev = NULL;

        // percorre a lista de presença p achar a pag
        while(current) {
            if(strcmp(current->page_id, page_id) == 0) { // achou
                if(prev) prev->next = current->next;
                else presenceMap[index] = current->next;
                free(current); // libera memoria
                return;
            }

            prev = current;
            current = current->next;
        }
    }
}

// função auxiliar do otimo para encontrar o prox uso de uma pag
// serve pro otimo encontrar a pag que vai ser usada mais tarde
static int getNextUse(HashNode* node, int currentIndex) {
    // avança o cursor ate encontrar um uso futuro que seja depois do indice atual
    while (node->nextUsePointer < node->numFutureUses &&
           node->futureUses[node->nextUsePointer] <= currentIndex) {
        node->nextUsePointer++;
    }

    // se ainda tiver usos futuros retorna o proximo
    if (node->nextUsePointer < node->numFutureUses) {
        return node->futureUses[node->nextUsePointer];
    }

    return INT_MAX; // infinito
}

// simulacao main
// retorna a quantidade de faltas de pag q ocorreram
int runOptimalSimulation(PageAccess * accessSequence, int numAccesses, int numPhysicalPages) {

    //aloca pros quadros da mem fisica
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

        char* victimPageId = NULL; // armazena pag que vai ser removida no futuro
        int slotIndex = -1;

        if (!pageFound) {
            pageFaults++;
            incrementLoadCount(currentPage, "optimal");

            if (g_verbose) {
                printf("[otimo] page fault #%d (acesso #%d): página '%s' não encontrada.\n", pageFaults, i + 1, currentPage);
            }

            // procura um slot vazio p bota a pag nova
            int emptySlot = -1;
            for (int j = 0; j < numPhysicalPages; j++) {
                if (frames[j] == NULL) {
                    emptySlot = j; 
                    break;
                }
            }

            // aloca no espaço vazio 
            if (emptySlot != -1) {
                frames[emptySlot] = currentPage;
                updatePresence(presenceMap, currentPage, 1); // adiciona na tabela de presença
                slotIndex = emptySlot;
                if (g_verbose) {
                    printf("inserido em: %d.\n", emptySlot);
                }
            } else {
                int victim = -1;
                int farthest = -1; // mais longe de ser usado dnv

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
                
                updatePresence(presenceMap, frames[victim], 0); // remove vítima
                frames[victim] = currentPage;
                updatePresence(presenceMap, currentPage, 1);     // adiciona nova página
            }
        }

        // verbose p mostra em tempo real cada troca
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

    free(frames); // libera mem fisica
    return pageFaults;
}