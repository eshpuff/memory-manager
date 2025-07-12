#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_PAGE_ID_LEN 10 
#define PAGE_SIZE_BYTES 4096 // tamanho da pag em bytes (4kb)
#define LOG_INTERVAL 50000 // intervalo de logs
#define DIDATIC_MODE_ACTIVATOR 32768 // limite de memoria p ativar o modo didatico
#define HASH_TABLE_SIZE 19997

// g para indicar que eh global
extern int g_verbose; //verbose serve parra ativar logs em tempo real
extern int g_didaticMode;
extern int g_pageCount;

// estrutura para armazenar a seq de acessos
typedef struct {
    char page_id[MAX_PAGE_ID_LEN];
} PageAccess;

// tabela hash p armazenar paginas e contadores
typedef struct HashNode {
    char page_id[MAX_PAGE_ID_LEN];
    int fifoLoads;
    int optimalLoads;
    struct HashNode* next;
} HashNode;

extern HashNode* hashTable[HASH_TABLE_SIZE];

unsigned int hashOptimize(const char* key); // hash para armazenar o id da pagina a um indice na tabela hash
void hashInit(); //inicializa a tabela hash
void registerPage(const char* page_id); //registrar uma pagina na lista de pagins conhecidas (tabela hahs)
void incrementLoadCount(const char* page_id, const char* algorithm); // conta quantas veze cada pagina foi carregada na memoria (compara o desempenho dos alga)
void loadSummary(); //printa tabela de carregamento
void cleanHashTable(); //libera a memoria da tabela hash (evida vazamentos de memória)

void displayFrameState(const char* algo, int num_pages, char** frames, const char* page_id, const char* pageToReplace, int slotIndex); //exibe o estado atual dos frames na memória
long long parseMemorySize(const char * sizeStr); //funçao aux que converte a string de tamanho de mem para bytes
int runOptimalSimulation(PageAccess * accessSequence, int numAccesses, int numPhysicalPages);

#endif
