#include "simulator.h"

int g_pageCount = 0;
HashNode* hashTable[HASH_TABLE_SIZE];

// hash para mapear o id da página a um indice na tabela hash
unsigned int hashOptimize(const char* key) {
    unsigned long int value = 5381;
    int c;
    while ((c = *key++)) {
        value = ((value << 5) + value) + c; // value * 33 + c
    }
    return value % HASH_TABLE_SIZE;
}

//inicializa a tabela hash
void hashInit() {
    g_pageCount = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        hashTable[i] = NULL;
    }
}

// encontra um nodo na tabela hash (funçao aux)
HashNode* findNode(const char* page_id) {
    unsigned int index = hashOptimize(page_id);
    HashNode* current = hashTable[index];
    while (current) {
        if (strcmp(current->page_id, page_id) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}


//registrar uma página na lista de paginas conhecida (tabela hash)
void registerPage(const char* page_id) {
    if (findNode(page_id) != NULL) {
        return; // pag ja registrada
    }

    // pag nova = incrementa contador
    g_pageCount++;

    unsigned int index = hashOptimize(page_id);
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (!newNode) {
        perror("falha ao alocar memoria pra tabela hash");
        exit(1);
    }

    // preenche as infromações da nova pag
    strcpy(newNode->page_id, page_id);
    newNode->fifoLoads = 0;
    newNode->optimalLoads = 0;

    // inicializa campos de pre processamento
    newNode->futureUses = NULL;
    newNode->numFutureUses = 0;
    newNode->futureUsesCapacity = 0;
    newNode->nextUsePointer = 0;

    newNode->next = hashTable[index];
    hashTable[index] = newNode;
}

// pre processa os acessos para o algoritmo otimo
void preprocessOptimal(PageAccess* accessSequence, int numAccesses) {
    printf("[OTIMO] iniciando pre processamento do arquivo de referencias...\n");
    for (int i = 0; i < numAccesses; i++) {
        HashNode* node = findNode(accessSequence[i].page_id);
        if (node) {
            //se o array de usos futuros ta cheio, realoca com mais espaço
            if (node->numFutureUses >= node->futureUsesCapacity) {
                node->futureUsesCapacity = (node->futureUsesCapacity == 0) ? 8 : node->futureUsesCapacity * 2;
                node->futureUses = (int*)realloc(node->futureUses, node->futureUsesCapacity * sizeof(int));
                if (!node->futureUses) {
                    perror("falha ao realocar memoria para usos futuros");
                    exit(1);
                }
            }
            // add o indice do acesso atual ao array
            node->futureUses[node->numFutureUses++] = i;
        }
    }
     printf("[OTIMO] pre processamento concluido!\n");
}

// contar quantas vezes cada página foi carregada na memória
// objetivo: comparar o desempenho dos algoritmos
void incrementLoadCount(const char* page_id, const char* algorithm) {
    HashNode* current = findNode(page_id);
    if (current) {
        if (strcmp(algorithm, "fifo") == 0) {
            current->fifoLoads++;
        } else if (strcmp(algorithm, "optimal") == 0) {
            current->optimalLoads++;
        }
    }
}

//prita toda tabela de carregamento
void loadSummary() {
    printf("\n--- carregamentos por página ---\n");
    printf("%-10s %-10s %-10s\n", "página", "otimo", "fifo");
    printf("----------------------------------\n");
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* current = hashTable[i];
        while (current) {
            printf("%-10s %-10d %-10d\n", current-> page_id, current-> optimalLoads, current-> fifoLoads);
            current = current->next;
        }
    }
}

// libera a memória da tabela hash
void cleanHashTable() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* current = hashTable[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            // libera a memória do array de usos futuros
            free(temp->futureUses);
            free(temp);
        }
        hashTable[i] = NULL;
    }
}