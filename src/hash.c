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

//registrar uma página na lista de paginas conhecida (tabela hash)
void registerPage(const char* page_id) {
    // calcula o "endereço" da página na tabela hash
    // transforma a string (ex: "P1") em um numero (indice do array)
    unsigned int index = hashOptimize(page_id);

    // pega o início da lista de páginasnesse endereço
    // ! colisão se tiver mais de uma página no mesmo endereço
    HashNode* current = hashTable[index];

    //percorre a lista para ver se a página já foi registrada
    while (current) {
        // se a string page_id for igual a de algum nodo existente
        if (strcmp(current->page_id, page_id) == 0) {
            // pagina ja mapeada
            return;
        }
        current = current->next;
    }

    // seo while acabou e a funçao n retornou = pagina nova

    // pag nova = incrementa contador
    g_pageCount++;

    // guarda as infos da nova pag
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (!newNode) {
        perror("falha ao alocar memoria pra tabela hash");
        exit(1);
    }

    // preenche as infromações da nova pag
    strcpy(newNode->page_id, page_id);
    newNode->fifoLoads = 0;
    newNode->optimalLoads = 0;

    newNode->next = hashTable[index];
    hashTable[index] = newNode;
}

// contar quantas vezes cada página foi carregada na memória
// objetivo: comparar o desempenho dos algoritmos
void incrementLoadCount(const char* page_id, const char* algorithm) {
    unsigned int index = hashOptimize(page_id);
    HashNode* current = hashTable[index];
    while (current) {
        if (strcmp(current->page_id, page_id) == 0) {
            if (strcmp(algorithm, "fifo") == 0) {
                current->fifoLoads++;
            } else if (strcmp(algorithm, "optimal") == 0) {
                current->optimalLoads++;
            }
            return;
        }
        current = current->next;
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
//objetivo: evitar vazamentos de memória
void cleanHashTable() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode* current = hashTable[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            free(temp);
        }
        hashTable[i] = NULL;
    }
}
