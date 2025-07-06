#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_PAGE_ID_LEN 10 
#define PAGE_SIZE_BYTES 4096 // tamanho da pag em bytes (4kb)


// estrutura para armazenar a seq de acessos
typedef struct {
    char page_id[MAX_PAGE_ID_LEN];
} PageAccess;

long long parseMemorySize(const char * sizeStr) {
    long long value;
    char unit[5];

    if (sscanf(sizeStr, "%lld%4s", &value, unit) < 1) {
        return -1; // erro na conversão
    }

    for (int i = 0; unit[i]; i++) {
        unit[i] = tolower(unit[i]); // converte para minusculo
    }

    // converte a unidade para bytes
    if(strcmp(unit, "kb") == 0) {
        return value * 1024LL;
    } else if(strcmp(unit, "mb") == 0) {
        return value * 1024LL * 1024LL;
    } else if(strcmp(unit, "gb") == 0) {
        return value * 1024LL * 1024LL * 1024LL;
    } else if(strcmp(unit, "b") == 0) {
        return value; // bytes
    } else {
        return -1; // unidade desconhecida
    }
}

int main(int argc, char *argv[]) {
    // eespera os argumentos: ./programa <arquivo> <tamanho_memoria>
    if (argc < 3) {
        printf("Uso: %s <arquivo_acessos.txt> <tamanho_memoria_fisica>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("erro ao abrir o arq de acessos");
        return 1;
    }

    //parsea o tamanho da memória física
    long long physicalMemoryBytes = parseMemorySize(argv[2]);
    if (physicalMemoryBytes == -1 || physicalMemoryBytes < PAGE_SIZE_BYTES) {
        fprintf(stderr, "[ERRO] tamanho de mem fisica inválido ou mt pequeno: %s\n", argv[2]);
        fclose(file);
        return 1;
    }

    int numPhysicalPages = physicalMemoryBytes / PAGE_SIZE_BYTES;

    //variaveis para armazenar os acessos
    PageAccess * accessSequence = NULL;
    int numAccesses = 0;
    int capacity = 100; // capacidade inicial para o array dinamico

    accessSequence = (PageAccess *)malloc(capacity * sizeof(PageAccess));
    if (accessSequence == NULL) {
        perror("Erro de alocação de memória para a sequência de acessos");
        fclose(file);
        return 1;
    }

    char line[256]; // buffer q le cada linha
    int lineNum;

    while (fgets(line, sizeof(line), file) != NULL) {
        
        int matched = sscanf(line, "%d %s", &lineNum, accessSequence[numAccesses].page_id);

        if (matched == 2) {
        } else if (sscanf(line, "%s", accessSequence[numAccesses].page_id) == 1) {
            if (strcmp(accessSequence[numAccesses].page_id, "...") == 0) {
                continue; 
            }
        } else {
            fprintf(stderr, "[AVISO] linha mal formatada ignorada: %s", line);
            continue;
        }

        numAccesses++;

        if (numAccesses == capacity) {
            capacity *= 2;
            PageAccess * temp = (PageAccess *)realloc(accessSequence, capacity * sizeof(PageAccess));
            if (temp == NULL) {
                perror("[ERRO] de realocaçao de mem para a seq de acessos");
                free(accessSequence);
                fclose(file);
                return 1;
            }
            accessSequence = temp;
        }
    }

    fclose(file);

    printf("A memória física comporta %d páginas. \n", numPhysicalPages);

    char **physicalMemoryFrames = (char **)malloc(numPhysicalPages * sizeof(char *));
    if (physicalMemoryFrames == NULL) {
        perror("Erro de alocação de memória para os frames da memória física");
        free(accessSequence);
        return 1;
    }

    //inicilaiza os frames como null
    for (int i = 0; i < numPhysicalPages; i++) {
        physicalMemoryFrames[i] = NULL;
    }

    printf("Total de acessos lidos: %d\n", numAccesses);
    printf("Primeiros 10 acessos:\n");
    for (int i = 0; i < (numAccesses > 10 ? 10 : numAccesses); i++) {
        printf("%s\n", accessSequence[i].page_id);
    }

    free(accessSequence);

    free(physicalMemoryFrames);

    return 0;
}