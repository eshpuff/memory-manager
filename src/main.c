#include "simulator.h"

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
        perror("[ERRO] ao abrir o arquivo.");
        return 1;
    }

    // inicia a tabela hash para contar páginas distintas
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
                printf("[FIFO] processando acesso %d...\n", numAccesses);
            }

            // insere a página na tabela hash e conta as páginas distintas
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

                // captura a vítima e slot  para o log didático do fifo
                char* victimPageId = NULL;
                if (fifoFrames[fifoPointer]) {
                    victimPageId = strdup(fifoFrames[fifoPointer]); // copia o nome da vítima
                }
                int slotIndex = fifoPointer;

                if (g_verbose) {
                    char* old_page = fifoFrames[fifoPointer] ? fifoFrames[fifoPointer] : "empty";
                    printf("[FIFO] page fault #%d (acesso #%d): página '%s' não encontrada, substituindo '%s'.\n",
                           fifoFaults, numAccesses, buffer, old_page);
                }

                free(fifoFrames[fifoPointer]);
                fifoFrames[fifoPointer] = strdup(buffer);
                if(!fifoFrames[fifoPointer]){
                    perror("[ERRO] falha ao alocar memória pro fifo");
                    exit(1);
                }

                // chama a função didática com os argumentos corretos
                if (g_didaticMode && g_verbose) {
                    displayFrameState("fifo", numPages, fifoFrames, buffer, victimPageId, slotIndex);
                }

                free(victimPageId); //libera a cópia

                fifoPointer = (fifoPointer + 1) % numPages;
            }
        }
    }
    fclose(file);
    printf("\nexecutando o ótimo...\n");

    //variaveis para armazenar os acessos
    PageAccess * accessSequence = (PageAccess*)malloc(numAccesses * sizeof(PageAccess));
    if (!accessSequence) {
        perror("[ERRO] ao alocar memória pros acessos");
        return 1;
    }

    file = fopen(filename, "r");
    if (!file) {
        perror("[ERRO] ao reabrir o arquivo de acessos");
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

    printf("\nRELATÓRIO:\n");

    double efficiency = (optimalFaults > 0 && fifoFaults > 0) ? ((double)optimalFaults / fifoFaults) * 100.0 : 100.0;
    if(fifoFaults == optimalFaults) efficiency = 100.0;
    if(fifoFaults < optimalFaults) efficiency = 0.0;

    // resultados
    printf("a memória física comporta %d páginas.\n", numPages);
    printf("há %d páginas distintas no arquivo.\n", g_pageCount);

    //estimativa do tamanho da tabela de pag
    long long tableSize = (long long)g_pageCount * 8;
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

    //libera memória
    free(accessSequence);
    cleanHashTable();
    for (int i = 0; i < numPages; i++) {
        free(fifoFrames[i]);
    }
    free(fifoFrames);

    printf("\nterminou!\n");
    return 0;
}
