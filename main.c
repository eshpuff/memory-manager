// gcc main.c -o main.exe
// ./main.exe <nome_do_arquivo>.txt <tamanho>
// ./main.exe <nome_do_arquivo>.txt <tamanho> -v  || pra caso queira ver alocação em tempo real

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_PAGE_ID_LEN 10 
#define PAGE_SIZE_BYTES 4096 // tamanho da pag em bytes (4kb)
#define LOG_INTERVAL 50000   // intervalo de logs
#define DIDATIC_MODE_ACTIVATOR 32768 // limite de memoria p ativar o modo didatico
#define HASH_TABLE_SIZE 19997

// g na frente pra indicar q eh global
int g_verbose = 0; // verbose serve p ativar logs em tempo real
int g_didaticMode = 0;

// tabela hash p armazenar paginas e contadores 
typedef struct HashNode {
    char page_id[MAX_PAGE_ID_LEN];
    int fifoLoads;      
    int optimalLoads;   
    struct HashNode* next;
} HashNode;


HashNode* hashTable[HASH_TABLE_SIZE];
int g_pageCount = 0; // contar paginas distintas

// hash para mapear o id da página a um indice na tabela hash
unsigned int hashOptimize(const char* key) {
    unsigned long int value = 5381;
    int c;
    while ((c = *key++)) {
        value = ((value << 5) + value) + c; // value * 33 + c
    }
    return value % HASH_TABLE_SIZE;
}

// inicializa a tabela hash
void hashInit() {
    g_pageCount = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        hashTable[i] = NULL;
    }
}

// registrar uma página na lista de páginas conhecidas (tabela hash)
void registerPage(const char* page_id) {
    // calcula o "endereço" da página na tabela hash
    // transforma a string (ex: "P1") em um numero (indice do array)
    unsigned int index = hashOptimize(page_id);

    // pega o início da lista de páginas nesse endereço
    // !! colisão se tiver mais de uma página no mesmo endereço
    HashNode* current = hashTable[index];

    // percorre a lista para ver se a página já foi registrada
    while (current) {
        // se a string do 'page_id' for igual a de algum nodo existente
        if (strcmp(current->page_id, page_id) == 0) {
            // pagina ja mapeada
            return;
        }
        current = current->next;
    }

    // se o while acabou e a funcao n retornou = pagina nova

    // pag nova = incrementa contador
    g_pageCount++;

    // guarda infos da nova pag
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (!newNode) {
        perror("falha ao alocar memoria pra tabela hash");
        exit(1);
    }
    
    // preenche as informações da nova pag
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

// printa a tabela de carregamento 
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
// objetivo: evitar vazamentos de memória
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

// exibe o estado atual dos frames na memória
void displayFrameState(const char* algo, int num_pages, char** frames, const char* page_id, const char* pageToReplace, int slotIndex) {
    printf("  [didático:%s] page fault ('%s'): ", algo, page_id);

    if (pageToReplace == NULL) { // entrou em um espaço vazio. insere normal
        printf("página inserida no slot %d.", slotIndex);
    } else {
        // substitui um slot usado
        printf("página '%s' foi substituída por '%s' no slot %d.", pageToReplace, page_id, slotIndex);
    }

    printf(" memória atual: [");
    for (int i = 0; i < num_pages; i++) {
        if (frames[i]) {
            printf(" %s ", frames[i]);
        } else {
            printf(" --- ");
        }
    }
    printf("]\n");
}

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

int runOptimalSimulation(PageAccess * accessSequence, int numAccesses, int numPhysicalPages) {
    char **frames = malloc(numPhysicalPages * sizeof(char *));
    for (int i= 0; i < numPhysicalPages; i++) frames[i] = NULL; // inicializa os frames como vazios
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
            // incrementa o contador de carregamento pro otimo
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
                // mem cheia -> aplica o alg otimo para escolher qual página substituir
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

// nova main com streaming pro fifo
int main(int argc, char *argv[]) {
    // eespera os argumentos: ./programa <arquivo> <tamanho_memoria>
    if (argc < 3) {
        printf("Uso: %s <arquivo.txt> <memoria> [-v]\n", argv[0]);
        return 1;
    }

    char* filename = argv[1];
    char* mem_size_str = argv[2];

    if (argc > 3) {
        if (strcmp(argv[3], "--verbose") == 0 || strcmp(argv[3], "-v") == 0) {
            g_verbose = 1;
            printf("logs em tempo real executando.\n");
        }
    }
    
    //parsea o tamanho da memória física
    long long memBytes = parseMemorySize(mem_size_str);
    if (memBytes < PAGE_SIZE_BYTES) {
        fprintf(stderr, "tamanho de memória física inválido: %s\n", mem_size_str);
        return 1;
    }
    
    int numPages = memBytes / PAGE_SIZE_BYTES;

    // modo didático se a memoria for menor que 32kb
    if (memBytes <= DIDATIC_MODE_ACTIVATOR) {
        g_didaticMode = 1;
        printf("modo didático true para memória de %s.\n", mem_size_str);
    }

    int numAccesses = 0;
    int fifoFaults = 0;
    int optimalFaults = 0;
    
    printf("\nexecutando o fifo...\n");

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("erro ao abrir o arquivo.");
        return 1;
    }
    
    // inicia a tabela hash pra contar páginas distintas
    hashInit();
    char **fifoFrames = (char**)malloc(numPages * sizeof(char*));
    for(int i = 0; i < numPages; i++) fifoFrames[i] = NULL;
    int fifoPointer = 0;

    char line[256];
    
    while (fgets(line, sizeof(line), file)) {
        char buffer[MAX_PAGE_ID_LEN];
        int lineNum;

        if (sscanf(line, "%d %s", &lineNum, buffer) == 2 || sscanf(line, "%s", buffer) == 1) {
            if (strcmp(buffer, "...") == 0) continue;

            numAccesses++;

            if (numAccesses > 0 && numAccesses % LOG_INTERVAL == 0) {
                printf("[fifo] processando acesso %d...\n", numAccesses);
            }
            
            // insere a página na tabela hash e conta páginas distintas
            registerPage(buffer);

            // fifo em tempo real
            int pageFound = 0;
            for (int j = 0; j < numPages; j++) {
                if (fifoFrames[j] && strcmp(fifoFrames[j], buffer) == 0) {
                    pageFound = 1;
                    break;
                }
            }
            
            if (!pageFound) {
                fifoFaults++;
                incrementLoadCount(buffer, "fifo");

                // captura a vítima e o slot para o log didático do fifo
                char* victimPageId = NULL;
                if (fifoFrames[fifoPointer]) {
                    victimPageId = strdup(fifoFrames[fifoPointer]); // copia o nome da vítima
                }
                int slotIndex = fifoPointer;

                if (g_verbose) {
                    char* old_page = fifoFrames[fifoPointer] ? fifoFrames[fifoPointer] : "empty";
                    printf("[fifo] page fault #%d (acesso #%d): página '%s' não encontrada, substituindo '%s'.\n",
                           fifoFaults, numAccesses, buffer, old_page);
                }

                free(fifoFrames[fifoPointer]);
                fifoFrames[fifoPointer] = strdup(buffer);
                if(!fifoFrames[fifoPointer]){
                    perror("falha ao alocar memória pro fifo");
                    exit(1);
                }
                
                // chama a função didática com os argumentos corretos
                if (g_didaticMode && g_verbose) {
                    displayFrameState("fifo", numPages, fifoFrames, buffer, victimPageId, slotIndex);
                }
                
                free(victimPageId); // libera a cópia

                fifoPointer = (fifoPointer + 1) % numPages;
            }
        }
    }
    fclose(file);
    printf("\nexecutando o otimo...\n");

    //variaveis para armazenar os acessos
    PageAccess * accessSequence = (PageAccess*)malloc(numAccesses * sizeof(PageAccess));
    if (!accessSequence) {
        perror("erro ao alocar memória pros acessos");
        return 1;
    }

    file = fopen(filename, "r");
    if (!file) {
        perror("erro ao reabrir o arquivo de acessos");
        free(accessSequence);
        return 1;
    }
    
    int currentAccess = 0;
    while (fgets(line, sizeof(line), file)) {
        char buffer[MAX_PAGE_ID_LEN];
        int lineNum;

        if (sscanf(line, "%d %s", &lineNum, buffer) == 2 || sscanf(line, "%s", buffer) == 1) {
            if (strcmp(buffer, "...") == 0) continue;
            strcpy(accessSequence[currentAccess].page_id, buffer);
            currentAccess++;
        }
    }
    fclose(file);

    // roda o algoritmo otimo
    optimalFaults = runOptimalSimulation(accessSequence, numAccesses, numPages);
    
    printf("\nresultados :DD\n");

    double efficiency = (optimalFaults > 0 && fifoFaults > 0) ? ((double)optimalFaults / fifoFaults) * 100.0 : 100.0;
    if(fifoFaults == optimalFaults) efficiency = 100.0;
    if(fifoFaults < optimalFaults) efficiency = 0.0;

    // resultados
    printf("a memória física comporta %d páginas.\n", numPages);
    printf("há %d páginas distintas no arquivo.\n", g_pageCount);

    // estimativa do tamanho da tabela de pag
    long long tableSize = (long long)g_pageCount * 8; // estimando 8 bytes por entrada
    printf("estimativa do tamanho da tabela de páginas (1 nível): %lld bytes (%.2f KB).\n",
           tableSize, (double)tableSize / 1024.0);

    printf("com o algoritmo otimo ocorrem %d faltas de página.\n", optimalFaults);
    printf("com o algoritmo fifo ocorrem %d faltas de página,\n", fifoFaults);
    printf("desempenho do fifo em relação ao otimo: %.2f%%\n", efficiency);

    printf("\ndeseja listar o numero de carregamentos (s/n)? ");

    char choice;
    scanf(" %c", &choice);

    if (choice == 's' || choice == 'S') {
        loadSummary();
    }

    // libera memória
    free(accessSequence);
    cleanHashTable();
    for (int i = 0; i < numPages; i++) {
        free(fifoFrames[i]);
    }
    free(fifoFrames);
    
    printf("\nterminou!\n");
    return 0;
}