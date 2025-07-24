// p executar: make / make clean |   ./bin/main.exe <arq.txt> <memoria> // no modo didatico: -v

#include "simulator.h"

// djb2 hash function
unsigned int presenceHash(const char* key) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_TABLE_SIZE; // index = centro da tabela
}

int main(int argc, char *argv[]) {
    // eespera os argumentos: ./programa <arquivo> <tamanho memoria>
    if (argc < 3) {
        printf("Uso: %s <arquivo.txt> <memoria> [-v]\n", argv[0]);
        return 1;
    }

    char* filename = argv[1];
    char* mem_size_str = argv[2];

    // ativa logs mais detalhados
    if (argc > 3 && (strcmp(argv[3], "--verbose") == 0 || strcmp(argv[3], "-v") == 0)) {
        g_verbose = 1;
        printf("logs em tempo real executando.\n");
    }

    //parsea o tamanho da memória física
    long long memBytes = parseMemorySize(mem_size_str);
    if (memBytes < PAGE_SIZE_BYTES) {
        fprintf(stderr, "tamanho de memória física inválido: %s\n", mem_size_str);
        return 1;
    }

    // calcula quantas pag cabe na memoria fisica
    int numPages = memBytes / PAGE_SIZE_BYTES;

    if (memBytes <= DIDATIC_MODE_ACTIVATOR) {
        g_didaticMode = 1;
        printf("modo didático true para memória de %s.\n", mem_size_str);

        // ativa verbose automaticamente para mem <= 32KB
        if (!g_verbose) {
            g_verbose = 1;
            printf("verbose ativado automaticamente para memória pequena.\n");
        }
    }


    //inicio da contagem de acessos e registro de pags
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("[ERRO] ao abrir o arquivo.");
        return 1;
    }

    // inicia a tabela hash para contar páginas distintas
    hashInit();

    // cria array q armazena sequencia de acessos de pag
    PageAccess* tempAccessSequence = malloc(100000 * sizeof(PageAccess)); 
    int capacity = 100000;
    int numAccesses = 0;
    char line[256];

    // le o arq e extrai o id das pag
    while (fgets(line, sizeof(line), file)) {
        char buffer[MAX_PAGE_ID_LEN];
        if (sscanf(line, "%*d %s", buffer) == 1 || sscanf(line, "%s", buffer) == 1) {
            if (strcmp(buffer, "...") == 0) continue;
            
            
            if (numAccesses >= capacity) {
                capacity *= 2; // se precisar
                tempAccessSequence = realloc(tempAccessSequence, capacity * sizeof(PageAccess));
            }
            // salva o id da pag no buffer 
            strcpy(tempAccessSequence[numAccesses].page_id, buffer);
            registerPage(buffer);
            numAccesses++;
        }
    }
    fclose(file);
    
     // ajusta o tam do array pra quantidade exata de acessos
    PageAccess* accessSequence = realloc(tempAccessSequence, numAccesses * sizeof(PageAccess));
    
    // SIMULAÇÃO FIFO
    printf("\nexecutando o fifo...\n");

    int fifoFaults = 0;
    
    char **fifoFrames = (char**)malloc(numPages * sizeof(char*));
    for(int i = 0; i < numPages; i++) fifoFrames[i] = NULL;

    int fifoPointer = 0; // aponta p qual quadro vai ser substituido quando tiver page fault

    PresenceNode* fifoPresenceMap[HASH_TABLE_SIZE] = {NULL}; // tabela hash q armazena as pag da mem fisica

    // processa a sequencia de acessos 
    for (int i = 0; i < numAccesses; i++) {
        char* currentPageId = accessSequence[i].page_id;

        if (i > 0 && i % LOG_INTERVAL == 0) {
            printf("[FIFO] processando acesso %d...\n", i);
        }

        //usa a posicao do hash para verificar se a pag ta na mem
        unsigned int hash_idx = presenceHash(currentPageId);
        PresenceNode* p_node = fifoPresenceMap[hash_idx]; // verifica se a pag ta na memoria fisica

        int pageFound = 0;

        while(p_node) {
            if(strcmp(p_node->page_id, currentPageId) == 0) {
                pageFound = 1;
                break;
            }
            p_node = p_node->next;
        }

        if (!pageFound) { // page fault
            fifoFaults++;
            incrementLoadCount(currentPageId, "fifo");

            char* victimPageId = NULL;

            if (fifoFrames[fifoPointer]) {
                victimPageId = strdup(fifoFrames[fifoPointer]);
                
                unsigned int victimHashIndex = presenceHash(victimPageId);
                PresenceNode* curr = fifoPresenceMap[victimHashIndex];

                PresenceNode* prev = NULL;

                while(curr) {
                    if(strcmp(curr->page_id, victimPageId) == 0) {
                        if(prev) prev->next = curr->next;
                        else fifoPresenceMap[victimHashIndex] = curr->next;
                        free(curr);
                        break;
                    }
                    prev = curr;
                    curr = curr->next;
                }
            }
            int slotIndex = fifoPointer; // salva o quadro q foi substituido

            if (g_verbose) {
                char* old_page = victimPageId ? victimPageId : "empty";
                printf("[FIFO] page fault #%d (acesso #%d): página '%s' não encontrada, substituindo '%s'.\n",
                       fifoFaults, i + 1, currentPageId, old_page);
            }

            free(fifoFrames[fifoPointer]);

            //coloca nova pag no quadro 
            fifoFrames[fifoPointer] = strdup(currentPageId);

            // adiciona a nova pag na tabela de presença
            PresenceNode* new_p_node = malloc(sizeof(PresenceNode));
            strcpy(new_p_node->page_id, currentPageId);
            new_p_node->next = fifoPresenceMap[hash_idx];
            fifoPresenceMap[hash_idx] = new_p_node;

            if (g_didaticMode && g_verbose) {
                displayFrameState("fifo", numPages, fifoFrames, currentPageId, victimPageId, slotIndex);
            }

            free(victimPageId);
            fifoPointer = (fifoPointer + 1) % numPages;
        }
    }

    // SIMULAÇÃO ÓTIMO
    printf("\nexecutando o ótimo...\n");
    preprocessOptimal(accessSequence, numAccesses); // pre processamento
    int optimalFaults = runOptimalSimulation(accessSequence, numAccesses, numPages);


    // RELATÓRIO FINAL
    printf("\nRELATÓRIO:\n");

    // comparacao dos algoritmos
    double efficiency = (fifoFaults > 0) ? ((double)optimalFaults / fifoFaults) * 100.0 : 0.0;

    if (fifoFaults > 0) {
       efficiency = (1.0 - (double)(fifoFaults - optimalFaults) / fifoFaults) * 100.0;
    } else {
       efficiency = 100.0;
    }


    printf("a memória física comporta %d páginas.\n", numPages);
    printf("há %d páginas distintas no arquivo.\n", g_pageCount);
    
    long long tableSize = (long long)g_pageCount * 8; // estimativa de 8 bytes por entrada
    
    printf("estimativa do tamanho da tabela de páginas (1 nível): %lld bytes (%.2f KB).\n",
           tableSize, (double)tableSize / 1024.0);

    printf("com o algoritmo ÓTIMO ocorrem %d faltas de página.\n", optimalFaults);
    printf("com o algoritmo FIFO ocorrem %d faltas de página,\n", fifoFaults);
    printf("desempenho do FIFO em relação ao OTIMO: %.2f%%\n", efficiency);
    
    printf("\ndeseja listar o número de carregamentos (s/n)? ");
    char choice;
    scanf(" %c", &choice);
    
    if (choice == 's' || choice == 'S') {
        loadSummary();
    }

    free(accessSequence);
    cleanHashTable();
    
    for (int i = 0; i < numPages; i++) free(fifoFrames[i]);
    free(fifoFrames);
    
    // limpa a tabela de presença do fifo
    for(int i = 0; i < HASH_TABLE_SIZE; ++i) {
        PresenceNode* curr = fifoPresenceMap[i];
        while(curr) {
            PresenceNode* temp = curr;
            curr = curr->next;
            free(temp);
        }
    }

    printf("\nterminou!\n");
    return 0;
}