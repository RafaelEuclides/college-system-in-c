#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>

#define ORDEM 3
#define ARQ_DADOS "dados.txt"
#define ARQ_INDICE "indice.idx"
#define MAX_STR 200

int i, j, k;

typedef struct Aluno {
    int id;
    char nome[100];
    char curso[100];
    int ano;
    int removido;
    int periodo;
} Aluno;

typedef struct BTreeNode {
    int folha;
    int n;
    int chaves[2 * ORDEM - 1];
    long offsets[2 * ORDEM - 1];
    struct BTreeNode *filhos[2 * ORDEM];
} BTreeNode;

BTreeNode* raiz = NULL;

// Protótipos das funções
BTreeNode* criarNo();
void inicializarArquivos();
void salvarAlunoArquivo(Aluno aluno, long* offset);
Aluno lerAlunoPorOffset(long offset); 
void inserirAluno(Aluno aluno);
Aluno buscarAluno(int id);
void listarAlunos();
void modificarAluno(int id, Aluno novo_dados_aluno);
void removerAluno(int id);
int verificarDuplicado(BTreeNode* no, int id);
BTreeNode* inserirBTree(BTreeNode* r, int chave, long offset);
void dividirNo(BTreeNode* pai, int indice_filho_cheio, BTreeNode* no_cheio);
void inserirChaveEmNoNaoCheio(BTreeNode* no, int chave, long offset);
void salvarIndiceRec(BTreeNode* no, FILE* f);
void salvarIndice();
BTreeNode* carregarIndiceRec(FILE* f);
void carregarIndiceOriginal();
void listarRec(BTreeNode* no);
int marcarAlunoComoRemovidoNoArquivo(int idRemover);
void reconstruirEMemoriaIndiceDeDadosTXT();
void liberarBTree(BTreeNode* no);


BTreeNode* criarNo() {
    BTreeNode* novo = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (!novo) {
        perror("Erro ao alocar memória para BTreeNode");
        exit(EXIT_FAILURE);
    }
    novo->folha = 1;
    novo->n = 0;
    for (i = 0; i < 2 * ORDEM; i++) {
        novo->filhos[i] = NULL;
    }
    return novo;
}

void inicializarArquivos() {
    FILE *f_dados = fopen(ARQ_DADOS, "a");
    if (f_dados) {
        fclose(f_dados);
    } else {
        perror("Erro ao inicializar arquivo de dados");
    }
}

void salvarAlunoArquivo(Aluno aluno, long* offset) {
    FILE* f = fopen(ARQ_DADOS, "a+");
    if (!f) {
        perror("Erro ao abrir arquivo de dados para salvar aluno");
        *offset = -1;
        return;
    }
    fseek(f, 0, SEEK_END);
    *offset = ftell(f);
    fprintf(f, "%d|%s|%s|%d|%d|%d|\n", aluno.id, aluno.nome, aluno.curso, aluno.ano, aluno.removido, aluno.periodo);
    fclose(f);
}

Aluno lerAlunoPorOffset(long offset) {
    FILE* f = fopen(ARQ_DADOS, "r");
    Aluno aluno = {-1, "", "", -1, 1, -1};
    char linha[MAX_STR];

    if (!f) {
        return aluno;
    }

    if (offset < 0) {
        fclose(f);
        return aluno;
    }

    fseek(f, offset, SEEK_SET);
    if (fgets(linha, MAX_STR, f) != NULL) {
        if (sscanf(linha, "%d|%[^|]|%[^|]|%d|%d|%d|",
                   &aluno.id, aluno.nome, aluno.curso,
                   &aluno.ano, &aluno.removido, &aluno.periodo) != 6) {
            aluno.id = -1;
            aluno.removido = 1;
        }
    } else {

    }
    fclose(f);
    return aluno;
}

int verificarDuplicado(BTreeNode* no, int id) {
    if (!no) return 0;
    int i = 0;
    while (i < no->n && id > no->chaves[i]) {
        i++;
    }
    if (i < no->n && id == no->chaves[i]) {
        Aluno temp = lerAlunoPorOffset(no->offsets[i]);
        if (temp.id != -1 && !temp.removido) {
            return 1;
        }
    }
    if (no->folha) {
        return 0;
    }
    return verificarDuplicado(no->filhos[i], id);
}

void inserirAluno(Aluno aluno) {
    if (verificarDuplicado(raiz, aluno.id)) {
        printf("\nErro: Já existe um aluno ativo com o ID %d.\n", aluno.id);
        return;
    }
    long offset;
    aluno.removido = 0;
    salvarAlunoArquivo(aluno, &offset);
    if (offset == -1) {
        printf("\nErro ao salvar aluno no arquivo de dados.\n");
        return;
    }
    raiz = inserirBTree(raiz, aluno.id, offset);
    salvarIndice();
    printf("\nAluno com ID %d inserido com sucesso!\n", aluno.id);
}

BTreeNode* inserirBTree(BTreeNode* r, int chave, long offset) {
    if (!r) {
        BTreeNode* novo_no = criarNo();
        novo_no->chaves[0] = chave;
        novo_no->offsets[0] = offset;
        novo_no->n = 1;
        return novo_no;
    }

    if (r->n == 2 * ORDEM - 1) {
        BTreeNode* nova_raiz = criarNo();
        nova_raiz->folha = 0;
        nova_raiz->filhos[0] = r;
        dividirNo(nova_raiz, 0, r);
        inserirChaveEmNoNaoCheio(nova_raiz, chave, offset);
        return nova_raiz;
    } else {
        inserirChaveEmNoNaoCheio(r, chave, offset);
        return r;
    }
}

void dividirNo(BTreeNode* pai, int indice_filho_cheio, BTreeNode* filho_cheio) {
    BTreeNode* novo_irmao = criarNo();
    novo_irmao->folha = filho_cheio->folha;
    novo_irmao->n = ORDEM - 1;
    int j;

    for (j = 0; j < ORDEM - 1; j++) {
        novo_irmao->chaves[j] = filho_cheio->chaves[j + ORDEM];
        novo_irmao->offsets[j] = filho_cheio->offsets[j + ORDEM];
    }

    if (!filho_cheio->folha) {
        for (j = 0; j < ORDEM; j++) {
            novo_irmao->filhos[j] = filho_cheio->filhos[j + ORDEM];
        }
    }

    filho_cheio->n = ORDEM - 1;

    for (j = pai->n; j > indice_filho_cheio; j--) {
        pai->filhos[j + 1] = pai->filhos[j];
    }
    pai->filhos[indice_filho_cheio + 1] = novo_irmao;

    for (j = pai->n - 1; j >= indice_filho_cheio; j--) {
        pai->chaves[j + 1] = pai->chaves[j];
        pai->offsets[j + 1] = pai->offsets[j];
    }

    pai->chaves[indice_filho_cheio] = filho_cheio->chaves[ORDEM - 1];
    pai->offsets[indice_filho_cheio] = filho_cheio->offsets[ORDEM - 1];
    pai->n++;
}

void inserirChaveEmNoNaoCheio(BTreeNode* no, int chave, long offset) {
    int i = no->n - 1;

    if (no->folha) {
        while (i >= 0 && chave < no->chaves[i]) {
            no->chaves[i + 1] = no->chaves[i];
            no->offsets[i + 1] = no->offsets[i];
            i--;
        }
        
        no->chaves[i + 1] = chave;
        no->offsets[i + 1] = offset;
        no->n++;
    } else {
        while (i >= 0 && chave < no->chaves[i]) {
            i--;
        }
        i++;

        if (no->filhos[i]->n == 2 * ORDEM - 1) {
            dividirNo(no, i, no->filhos[i]);
            if (chave > no->chaves[i]) {
                i++;
            }
        }
        inserirChaveEmNoNaoCheio(no->filhos[i], chave, offset);
    }
}

void salvarIndiceRec(BTreeNode* no, FILE* f) {
    if (no) {
        fwrite(&no->folha, sizeof(int), 1, f);
        fwrite(&no->n, sizeof(int), 1, f);
        fwrite(no->chaves, sizeof(int), 2 * ORDEM - 1, f);
        fwrite(no->offsets, sizeof(long), 2 * ORDEM - 1, f);

        if (!no->folha) {
            for (i = 0; i <= no->n; i++) {
                salvarIndiceRec(no->filhos[i], f);
            }
        }
    }
}

void salvarIndice() {
    FILE* f = fopen(ARQ_INDICE, "wb");
    if (!f) {
        perror("Erro ao abrir arquivo de índice para salvar");
        return;
    }
    if (raiz) {
         salvarIndiceRec(raiz, f);
    }
    fclose(f);
}

BTreeNode* carregarIndiceRecInterno(FILE* f) {
    BTreeNode* no = NULL;
    int folha_lida;

    if (fread(&folha_lida, sizeof(int), 1, f) != 1) {
        return NULL;
    }

    no = criarNo();
    no->folha = folha_lida;
    fread(&no->n, sizeof(int), 1, f);
    fread(no->chaves, sizeof(int), 2 * ORDEM - 1, f);
    fread(no->offsets, sizeof(long), 2 * ORDEM - 1, f);

    if (!no->folha) {
        for (i = 0; i <= no->n; i++) {
            no->filhos[i] = carregarIndiceRecInterno(f);
        }
    }
    return no;
}

void carregarIndiceOriginal() {
    FILE* f = fopen(ARQ_INDICE, "rb");
    if (!f) {
        raiz = NULL;
        return;
    }
    liberarBTree(raiz);
    raiz = carregarIndiceRecInterno(f);
    fclose(f);
}


void reconstruirEMemoriaIndiceDeDadosTXT() {
    FILE* f_dados = fopen(ARQ_DADOS, "r");

    liberarBTree(raiz); 
    raiz = NULL;       

    if (!f_dados) {
        return;
    }

    char linha[MAX_STR];
    Aluno aluno_temp;
    long record_offset = 0;

    record_offset = ftell(f_dados);
    while (fgets(linha, MAX_STR, f_dados) != NULL) {
        if (sscanf(linha, "%d|%[^|]|%[^|]|%d|%d|%d|",
                   &aluno_temp.id, aluno_temp.nome, aluno_temp.curso,
                   &aluno_temp.ano, &aluno_temp.removido, &aluno_temp.periodo) == 6) {
            if (!aluno_temp.removido) {
                raiz = inserirBTree(raiz, aluno_temp.id, record_offset);
            }
        }
        record_offset = ftell(f_dados);
    }
    fclose(f_dados);
}


void listarRec(BTreeNode* no) {
    if (no) {
        int i_local;
        for (i_local = 0; i_local < no->n; i_local++) {
            if (!no->folha) {
                listarRec(no->filhos[i_local]);
            }
            Aluno aluno = lerAlunoPorOffset(no->offsets[i_local]);
            if (aluno.id != -1 && !aluno.removido) {
                printf("ID: %05d | Nome: %s | Curso: %s | Ano: %d | Período: %d\n",
                       aluno.id, aluno.nome, aluno.curso, aluno.ano, aluno.periodo);
            }
        }
        if (!no->folha) {
            listarRec(no->filhos[i_local]);
        }
    }
}

void listarAlunos() {
    printf("\n--- Lista de Alunos ---\n\n");
    if (!raiz) {
        printf("Nenhum aluno cadastrado ou índice vazio.\n");
    } else {
        listarRec(raiz);
    }
    printf("\n-----------------------\n\n");
}


void modificarAluno(int id_modificar, Aluno novos_dados_aluno) {
    Aluno aluno_existente = buscarAluno(id_modificar);
    if (aluno_existente.id == -1 || aluno_existente.removido) {
        printf("\nErro ao modificar: Aluno com ID %d não encontrado ou já removido.\n", id_modificar);
        return;
    }

    FILE* f_read = fopen(ARQ_DADOS, "r");
    FILE* f_temp = fopen("temp_dados.txt", "w");

    if (!f_read || !f_temp) {
        perror("Erro ao abrir arquivos para modificação");
        if (f_read) fclose(f_read);
        if (f_temp) fclose(f_temp);
        return;
    }

    char linha[MAX_STR];
    int aluno_modificado_no_arquivo = 0;
    novos_dados_aluno.id = id_modificar;
    novos_dados_aluno.removido = 0;  

    while (fgets(linha, MAX_STR, f_read) != NULL) {
        int id_linha_atual;
        if (sscanf(linha, "%d|", &id_linha_atual) == 1 && id_linha_atual == id_modificar) {
            fprintf(f_temp, "%d|%s|%s|%d|%d|%d|\n",
                    novos_dados_aluno.id, novos_dados_aluno.nome, novos_dados_aluno.curso,
                    novos_dados_aluno.ano, novos_dados_aluno.removido, novos_dados_aluno.periodo);
            aluno_modificado_no_arquivo = 1;
        } else {
            fputs(linha, f_temp);
        }
    }
    fclose(f_read);
    fclose(f_temp);

    if (aluno_modificado_no_arquivo) {
        remove(ARQ_DADOS);
        rename("temp_dados.txt", ARQ_DADOS);

        reconstruirEMemoriaIndiceDeDadosTXT();
        salvarIndice();

        printf("\nAluno com ID %d modificado com sucesso!\n", id_modificar);
    } else {
        remove("temp_dados.txt");
        printf("\nErro ao modificar: Aluno com ID %d não encontrado no arquivo de dados durante a reescrita.\n", id_modificar);
    }
}


int marcarAlunoComoRemovidoNoArquivo(int idRemover) {
    FILE* f_read = fopen(ARQ_DADOS, "r");
    FILE* f_temp = fopen("temp_remove.txt", "w");

    if (!f_read || !f_temp) {
        perror("Erro ao abrir arquivos para marcar remoção");
        if (f_read) fclose(f_read);
        if (f_temp) fclose(f_temp);
        return 0;
    }

    char linha[MAX_STR];
    int encontrado_e_marcado = 0;
    Aluno aluno_temp_parse;

    while (fgets(linha, MAX_STR, f_read) != NULL) {
        if (sscanf(linha, "%d|%[^|]|%[^|]|%d|%d|%d|",
                   &aluno_temp_parse.id, aluno_temp_parse.nome, aluno_temp_parse.curso,
                   &aluno_temp_parse.ano, &aluno_temp_parse.removido, &aluno_temp_parse.periodo) == 6) {

            if (aluno_temp_parse.id == idRemover) {
                if (aluno_temp_parse.removido == 1) {
                    fprintf(f_temp, "%d|%s|%s|%d|1|%d|\n",
                            aluno_temp_parse.id, aluno_temp_parse.nome, aluno_temp_parse.curso,
                            aluno_temp_parse.ano, aluno_temp_parse.periodo);
                } else {
                    fprintf(f_temp, "%d|%s|%s|%d|1|%d|\n",
                            aluno_temp_parse.id, aluno_temp_parse.nome, aluno_temp_parse.curso,
                            aluno_temp_parse.ano, aluno_temp_parse.periodo);
                }
                encontrado_e_marcado = 1;
            } else {
                fputs(linha, f_temp);
            }
        } else {
            fputs(linha, f_temp);
        }
    }

    fclose(f_read);
    fclose(f_temp);

    if (encontrado_e_marcado) {
        remove(ARQ_DADOS);
        rename("temp_remove.txt", ARQ_DADOS);
        return 1;
    } else {
        remove("temp_remove.txt");
        return 0; 
    }
}

void removerAluno(int id) {
    long offset_aluno = -1;
    BTreeNode* no_atual = raiz;
    while (no_atual) {
        int i = 0;
        while (i < no_atual->n && id > no_atual->chaves[i]) i++;
        if (i < no_atual->n && id == no_atual->chaves[i]) {
            offset_aluno = no_atual->offsets[i];
            break;
        }
        if (no_atual->folha) break;
        no_atual = no_atual->filhos[i];
    }

    if (offset_aluno == -1) {
        printf("\nErro: Aluno com ID %d não encontrado no índice.\n", id);
        return;
    }

    Aluno aluno_para_remover = lerAlunoPorOffset(offset_aluno);
    if (aluno_para_remover.id == -1) {
        printf("\nErro ao ler dados do aluno com ID %d do arquivo para remoção.\n", id);
        return;
    }
    if (aluno_para_remover.removido == 1) {
        printf("\nAluno com ID %d já está marcado como removido.\n", id);
        return;
    }

    if (marcarAlunoComoRemovidoNoArquivo(id)) {
        printf("\nAluno com ID %d marcado como removido logicamente.\n", id);
    } else {
        printf("\nErro ao tentar marcar aluno com ID %d como removido no arquivo de dados.\n", id);
    }
}

Aluno buscarAluno(int id) {
    BTreeNode* no_atual = raiz;
    while (no_atual) {
        int i = 0;
        while (i < no_atual->n && id > no_atual->chaves[i]) {
            i++;
        }
        if (i < no_atual->n && id == no_atual->chaves[i]) {
            Aluno a = lerAlunoPorOffset(no_atual->offsets[i]);
            if (a.id != -1 && !a.removido) {
                return a;
            } else {
                break;
            }
        }
        if (no_atual->folha) {
            break;
        }
        no_atual = no_atual->filhos[i];
    }
    Aluno vazio = {-1, "", "", -1, 1, -1};
    return vazio;
}

void liberarBTree(BTreeNode* no) {
    if (no) {
        if (!no->folha) {
            for (i = 0; i <= no->n; i++) {
                liberarBTree(no->filhos[i]);
            }
        }
        free(no);
    }
}


int main() {
    setlocale(LC_ALL, "Portuguese");
    inicializarArquivos();

    reconstruirEMemoriaIndiceDeDadosTXT();
    salvarIndice();

    int op;
    Aluno aluno_entrada;
    char buffer_entrada[MAX_STR];
    char *endptr;

    while (1) {
        printf("--- Menu ---\n");
        printf("1. Inserir Aluno\n");
        printf("2. Buscar Aluno por ID\n");
        printf("3. Listar Alunos\n");
        printf("4. Modificar Aluno\n");
        printf("5. Remover Aluno (Logicamente)\n");
        printf("6. Sair\n");
        printf("Escolha uma opção: ");

        if (fgets(buffer_entrada, sizeof(buffer_entrada), stdin) == NULL) {

            break;
        }

        buffer_entrada[strcspn(buffer_entrada, "\n")] = 0;


        op = strtol(buffer_entrada, &endptr, 10);

        if (*endptr != '\0' || buffer_entrada == endptr) {
            if (strlen(buffer_entrada) == 0 && op == 0) { // Permite entrada vazia se op for 0, mas ainda pode ser inválido
            } else {
                printf("Entrada inválida. Por favor, digite um número.\n");
                while(getchar()!='\n');
                continue;
            }
        }


        switch (op) {
            case 1:
                printf("ID: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                aluno_entrada.id = strtol(buffer_entrada, &endptr, 10);
                if (*endptr != '\n' && *endptr != '\0') { printf("ID inválido.\n"); break; }

                printf("Nome: ");
                fgets(aluno_entrada.nome, sizeof(aluno_entrada.nome), stdin);
                aluno_entrada.nome[strcspn(aluno_entrada.nome, "\n")] = '\0';

                printf("Curso: ");
                fgets(aluno_entrada.curso, sizeof(aluno_entrada.curso), stdin);
                aluno_entrada.curso[strcspn(aluno_entrada.curso, "\n")] = '\0';

                printf("Ano: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                aluno_entrada.ano = strtol(buffer_entrada, &endptr, 10);
                if (*endptr != '\n' && *endptr != '\0') { printf("Ano inválido.\n"); break; }

                printf("Período: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                aluno_entrada.periodo = strtol(buffer_entrada, &endptr, 10);
                if (*endptr != '\n' && *endptr != '\0') { printf("Período inválido.\n"); break; }

                aluno_entrada.removido = 0;
                inserirAluno(aluno_entrada);
                break;

            case 2:
                printf("ID do aluno a buscar: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                int id_busca = strtol(buffer_entrada, &endptr, 10);
                if (*endptr != '\n' && *endptr != '\0') { printf("ID inválido.\n"); break; }

                Aluno aluno_buscado = buscarAluno(id_busca);
                if (aluno_buscado.id != -1 && !aluno_buscado.removido) {
                    printf("ID: %05d | Nome: %s | Curso: %s | Ano: %d | Período: %d\n",
                           aluno_buscado.id, aluno_buscado.nome, aluno_buscado.curso, aluno_buscado.ano, aluno_buscado.periodo);
                } else {
                    printf("Aluno com ID %d não encontrado ou removido.\n", id_busca);
                }
                break;

            case 3:
                listarAlunos();
                break;

            case 4:
                printf("ID do aluno a modificar: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                int id_modificar = strtol(buffer_entrada, &endptr, 10);
                 if (*endptr != '\n' && *endptr != '\0') { printf("ID inválido.\n"); break; }

                Aluno novos_dados_aluno;
                printf("Novo Nome: ");
                fgets(novos_dados_aluno.nome, sizeof(novos_dados_aluno.nome), stdin);
                novos_dados_aluno.nome[strcspn(novos_dados_aluno.nome, "\n")] = '\0';

                printf("Novo Curso: ");
                fgets(novos_dados_aluno.curso, sizeof(novos_dados_aluno.curso), stdin);
                novos_dados_aluno.curso[strcspn(novos_dados_aluno.curso, "\n")] = '\0';

                printf("Novo Ano: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                novos_dados_aluno.ano = strtol(buffer_entrada, &endptr, 10);
                if (*endptr != '\n' && *endptr != '\0') { printf("Ano inválido.\n"); break; }

                printf("Novo Período: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                novos_dados_aluno.periodo = strtol(buffer_entrada, &endptr, 10);
                if (*endptr != '\n' && *endptr != '\0') { printf("Período inválido.\n"); break; }
                
                modificarAluno(id_modificar, novos_dados_aluno);
                break;

            case 5:
                printf("ID do aluno a remover: ");
                fgets(buffer_entrada, sizeof(buffer_entrada), stdin);
                int id_remover = strtol(buffer_entrada, &endptr, 10);
                if (*endptr != '\n' && *endptr != '\0') { printf("ID inválido.\n"); break; }
                removerAluno(id_remover);
                break;

            case 6:
                printf("Saindo...\n");
                liberarBTree(raiz);
                exit(0);

            default:
                printf("Opção inválida.\n");
        }
    }

    liberarBTree(raiz);
    return 0;
}
