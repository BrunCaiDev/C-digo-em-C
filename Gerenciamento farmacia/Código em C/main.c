#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_MED        100
#define MAX_CARRINHO   20
#define NUM_CATEGORIAS 4
#define DIAS_SEMANA    7
#define ESTOQUE_MINIMO 10
#define ARQUIVO        "farmacia.dat"

typedef struct {
    int   codigo;
    char  nome[50];
    int   categoria;   /* 0 a 3, indice em nomeCategoria */
    float preco;
    int   quantidade;
    int   controlado;  /* 1 = exige receita */
} Medicamento;

/* ---------- dados globais ---------- */
Medicamento estoque[MAX_MED];                        /* vetor de estruturas */
int totalMed = 0;
float vendasSemana[DIAS_SEMANA][NUM_CATEGORIAS];     /* matriz: dia x categoria */

const char *nomeCategoria[NUM_CATEGORIAS] = {
    "Analgesicos", "Antibioticos", "Vitaminas", "Higiene"
};
const char *nomeDia[DIAS_SEMANA] = {
    "Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"
};

/* ---------- utilitarios ---------- */
void limparBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int lerInteiro(const char *msg) {
    int v;
    printf("%s", msg);
    while (scanf("%d", &v) != 1) {
        limparBuffer();
        printf("Valor invalido. %s", msg);
    }
    limparBuffer();
    return v;
}

float lerReal(const char *msg) {
    float v;
    printf("%s", msg);
    while (scanf("%f", &v) != 1) {
        limparBuffer();
        printf("Valor invalido. %s", msg);
    }
    limparBuffer();
    return v;
}

void lerTexto(const char *msg, char *destino, int tam) {
    printf("%s", msg);
    fgets(destino, tam, stdin);
    destino[strcspn(destino, "\n")] = '\0';
}

/* ---------- arquivos ---------- */
void salvarDados(void) {
    FILE *f = fopen(ARQUIVO, "wb");
    if (f == NULL) {
        printf("Erro ao salvar dados!\n");
        return;
    }
    fwrite(&totalMed, sizeof(int), 1, f);
    fwrite(estoque, sizeof(Medicamento), totalMed, f);
    fwrite(vendasSemana, sizeof(vendasSemana), 1, f);
    fclose(f);
}

void carregarDados(void) {
    FILE *f = fopen(ARQUIVO, "rb");
    if (f == NULL) return;  /* primeira execucao: arquivo ainda nao existe */
    if (fread(&totalMed, sizeof(int), 1, f) != 1) totalMed = 0;
    if (totalMed < 0 || totalMed > MAX_MED) totalMed = 0;
    fread(estoque, sizeof(Medicamento), totalMed, f);
    fread(vendasSemana, sizeof(vendasSemana), 1, f);
    fclose(f);
}

/* ---------- funcoes do estoque ---------- */
int buscarPorCodigo(int codigo) {
    int i;
    for (i = 0; i < totalMed; i++) {
        if (estoque[i].codigo == codigo) return i;
    }
    return -1;
}

void cadastrarMedicamento(void) {
    Medicamento m;
    int i;

    if (totalMed >= MAX_MED) {
        printf("Estoque cheio!\n");
        return;
    }
    m.codigo = lerInteiro("Codigo: ");
    if (buscarPorCodigo(m.codigo) != -1) {
        printf("Ja existe um medicamento com esse codigo.\n");
        return;
    }
    lerTexto("Nome: ", m.nome, sizeof(m.nome));

    printf("Categorias:\n");
    for (i = 0; i < NUM_CATEGORIAS; i++)
        printf("  %d - %s\n", i, nomeCategoria[i]);
    do {
        m.categoria = lerInteiro("Categoria: ");
    } while (m.categoria < 0 || m.categoria >= NUM_CATEGORIAS);

    do {
        m.preco = lerReal("Preco (R$): ");
    } while (m.preco <= 0);
    do {
        m.quantidade = lerInteiro("Quantidade inicial: ");
    } while (m.quantidade < 0);
    m.controlado = lerInteiro("Exige receita? (1-Sim / 0-Nao): ") == 1;

    estoque[totalMed++] = m;
    salvarDados();
    printf("Medicamento cadastrado com sucesso!\n");
}

void listarEstoque(void) {
    int i;
    if (totalMed == 0) {
        printf("Nenhum medicamento cadastrado.\n");
        return;
    }
    printf("\n%-6s %-25s %-13s %9s %5s %s\n",
           "Cod", "Nome", "Categoria", "Preco", "Qtd", "Receita");
    for (i = 0; i < totalMed; i++) {
        printf("%-6d %-25s %-13s %9.2f %5d %s\n",
               estoque[i].codigo, estoque[i].nome,
               nomeCategoria[estoque[i].categoria],
               estoque[i].preco, estoque[i].quantidade,
               estoque[i].controlado ? "Sim" : "Nao");
    }
}

void consultarMedicamento(void) {
    int pos = buscarPorCodigo(lerInteiro("Codigo: "));
    if (pos == -1) {
        printf("Medicamento nao encontrado.\n");
        return;
    }
    printf("Nome: %s | Categoria: %s | Preco: R$ %.2f | Estoque: %d\n",
           estoque[pos].nome, nomeCategoria[estoque[pos].categoria],
           estoque[pos].preco, estoque[pos].quantidade);
}

void reporEstoque(void) {
    int pos = buscarPorCodigo(lerInteiro("Codigo: "));
    int qtd;
    if (pos == -1) {
        printf("Medicamento nao encontrado.\n");
        return;
    }
    qtd = lerInteiro("Quantidade a adicionar: ");
    if (qtd <= 0) {
        printf("Quantidade invalida.\n");
        return;
    }
    estoque[pos].quantidade += qtd;
    salvarDados();
    printf("Novo estoque de %s: %d\n", estoque[pos].nome, estoque[pos].quantidade);
}

void alertaEstoqueBaixo(void) {
    int i, achou = 0;
    printf("\nItens com estoque abaixo de %d unidades:\n", ESTOQUE_MINIMO);
    for (i = 0; i < totalMed; i++) {
        if (estoque[i].quantidade < ESTOQUE_MINIMO) {
            printf("  [%d] %s - %d un.\n", estoque[i].codigo,
                   estoque[i].nome, estoque[i].quantidade);
            achou = 1;
        }
    }
    if (!achou) printf("  Nenhum item em nivel critico.\n");
}

/* ---------- venda ---------- */
int quantidadeNoCarrinho(const int cods[], const int qtds[], int n, int codigo) {
    int i, soma = 0;
    for (i = 0; i < n; i++)
        if (cods[i] == codigo) soma += qtds[i];
    return soma;
}

void realizarVenda(void) {
    int cods[MAX_CARRINHO], qtds[MAX_CARRINHO];
    int n = 0, i, dia, codigo, qtd, pos, disponivel;
    float total = 0, desconto = 0;
    char resp;

    printf("\n--- Nova venda (codigo 0 para finalizar) ---\n");
    while (n < MAX_CARRINHO) {
        codigo = lerInteiro("Codigo do medicamento: ");
        if (codigo == 0) break;

        pos = buscarPorCodigo(codigo);
        if (pos == -1) {
            printf("Medicamento nao encontrado.\n");
            continue;
        }
        if (estoque[pos].controlado) {
            printf("%s exige receita. Receita apresentada? (s/n): ", estoque[pos].nome);
            scanf(" %c", &resp);
            limparBuffer();
            if (resp != 's' && resp != 'S') {
                printf("Venda deste item nao autorizada.\n");
                continue;
            }
        }
        qtd = lerInteiro("Quantidade: ");
        disponivel = estoque[pos].quantidade - quantidadeNoCarrinho(cods, qtds, n, codigo);
        if (qtd <= 0 || qtd > disponivel) {
            printf("Quantidade invalida. Disponivel: %d\n", disponivel);
            continue;
        }
        cods[n] = codigo;
        qtds[n] = qtd;
        n++;
        total += qtd * estoque[pos].preco;
        printf("Item adicionado. Subtotal: R$ %.2f\n", total);
    }

    if (n == 0) {
        printf("Venda cancelada (carrinho vazio).\n");
        return;
    }

    if (total >= 100.0f) {          /* regra de negocio: 5% acima de R$ 100 */
        desconto = total * 0.05f;
        printf("Desconto de 5%% aplicado: -R$ %.2f\n", desconto);
    }
    printf("TOTAL A PAGAR: R$ %.2f\n", total - desconto);

    printf("Confirmar venda? (s/n): ");
    scanf(" %c", &resp);
    limparBuffer();
    if (resp != 's' && resp != 'S') {
        printf("Venda cancelada.\n");
        return;
    }

    do {
        dia = lerInteiro("Dia da semana (0=Dom ... 6=Sab): ");
    } while (dia < 0 || dia >= DIAS_SEMANA);

    for (i = 0; i < n; i++) {
        pos = buscarPorCodigo(cods[i]);
        estoque[pos].quantidade -= qtds[i];
        vendasSemana[dia][estoque[pos].categoria] +=
            qtds[i] * estoque[pos].preco * (1.0f - (desconto / total));
    }
    salvarDados();
    printf("Venda concluida!\n");
}

/* ---------- relatorio (matriz) ---------- */
void relatorioVendas(void) {
    int d, c;
    float totalDia, totalGeral = 0;

    printf("\n--- Vendas da semana (R$) ---\n%-5s", "Dia");
    for (c = 0; c < NUM_CATEGORIAS; c++) printf("%14s", nomeCategoria[c]);
    printf("%12s\n", "Total");

    for (d = 0; d < DIAS_SEMANA; d++) {
        totalDia = 0;
        printf("%-5s", nomeDia[d]);
        for (c = 0; c < NUM_CATEGORIAS; c++) {
            printf("%14.2f", vendasSemana[d][c]);
            totalDia += vendasSemana[d][c];
        }
        printf("%12.2f\n", totalDia);
        totalGeral += totalDia;
    }
    printf("Faturamento total da semana: R$ %.2f\n", totalGeral);
}

/* ---------- menu ---------- */
int main(void) {
    int opcao;
    carregarDados();

    do {
        printf("\n===== FARMACIA - MENU =====\n");
        printf("1 - Cadastrar medicamento\n");
        printf("2 - Listar estoque\n");
        printf("3 - Consultar medicamento\n");
        printf("4 - Realizar venda\n");
        printf("5 - Repor estoque\n");
        printf("6 - Alerta de estoque baixo\n");
        printf("7 - Relatorio de vendas\n");
        printf("0 - Sair\n");
        opcao = lerInteiro("Opcao: ");

        switch (opcao) {
            case 1: cadastrarMedicamento(); break;
            case 2: listarEstoque();        break;
            case 3: consultarMedicamento(); break;
            case 4: realizarVenda();        break;
            case 5: reporEstoque();         break;
            case 6: alertaEstoqueBaixo();   break;
            case 7: relatorioVendas();      break;
            case 0: printf("Encerrando...\n"); break;
            default: printf("Opcao invalida!\n");
        }
    } while (opcao != 0);

    return 0;
}
