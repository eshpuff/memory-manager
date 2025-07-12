#include "simulator.h"

int g_verbose = 0;
int g_didaticMode = 0;

// exibe o estado atual dos frames na memória
void displayFrameState(const char* algo, int num_pages, char** frames, const char* page_id, const char* pageToReplace, int slotIndex) {
    printf("[didático:%s] page fault ('%s'): ", algo, page_id);

    if (pageToReplace == NULL) { // entrou em um espaçõ vazio. insere normal
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

// funçao aux que converte a string de tamanho de mem para bytes
long long parseMemorySize(const char * sizeStr) {
    long long value;
    char unit[5] = "";

    if (sscanf(sizeStr, "%lld%4s", &value, unit) < 1) return -1; // erro na conversão
    for (int i = 0; unit[i]; i++) unit[i] = tolower(unit[i]); // converte para minusculo
    
    //converte a unidade para bytes
    if(strcmp(unit, "kb") == 0) return value * 1024LL;
    if(strcmp(unit, "mb") == 0) return value * 1024LL * 1024LL;
    if(strcmp(unit, "gb") == 0) return value * 1024LL * 1024LL * 1024LL;
    if(strcmp(unit, "b") == 0) return value;
    return -1; // unidade desconhecida
}
