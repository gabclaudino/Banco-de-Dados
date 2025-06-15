/*
    Aluno: Gabriel Claudino de Souza
    GRR: 20215730
    Disciplina: Bancos de Dados - 2025/1
    Professor: Eduardo Almeida
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OPS 10000    // maximo de operacoes total
#define MAX_TX 100       // maximo transacoes por escalonamento
#define MAX_ATTR 32      // tamanho maximo do atributo

// estrutura para representar uma operacao
typedef struct {
    int time;               // tempo de chegada
    int tid;                // identificador da transação
    char op;                // 'R', 'W' ou 'C'
    char attr[MAX_ATTR];    // atributo lido/escrito
} Op;

// representa um escalonamento (lista de operacoes)
typedef struct {
    Op ops[MAX_OPS];
    int n_ops;
} Schedule;

// grafo e auxiliares para teste de seriabilidade
int adj[MAX_TX][MAX_TX];   // matriz de adjacencia
int n_tx;                  // numero de transacoes distintas
int tx_ids[MAX_TX];        // lista de tids no escalonamento
int cor[MAX_TX];           // para DFS (0=branco,1=cinza,2=preto)

// estrutura para capturar informacoes de leituras
typedef struct {
    int tid;               // transacao que leu
    char attr[MAX_ATTR];   // atributo lido
    int rank;              // enesima leitura desse atributo por essa transacao
    int writer;            // tid do escritor de quem leu (-1 se inicial)
} ReadInfo;

// gera proxima permutacao lexicografica (retorna 1 se ha proxima)
int next_permutation(int *a, int n) {
    int i = n - 2;
    while (i >= 0 && a[i] >= a[i+1]) 
        i--;

    if (i < 0) 
        return 0;

    int j = n - 1;

    while (a[j] <= a[i]) 
        j--;

    int tmp = a[i]; 
    a[i] = a[j]; 
    a[j] = tmp;
    int l = i+1, r = n-1;

    while (l < r) { 
        tmp = a[l]; 
        a[l++] = a[r]; 
        a[r--] = tmp; 
    }

    return 1;
}


// encontra indice de um tid na lista tx_ids
int index_of(int tid) {
    for (int i = 0; i < n_tx; i++) {
        if (tx_ids[i] == tid) 
            return i;
    }
    return -1;
}

// coleta tids de um escalonamento, em ordem de aparicao
void coletar_transacoes(Schedule *sch) {
    n_tx = 0;
    for (int i = 0; i < sch->n_ops; i++) {
        int tid = sch->ops[i].tid;
        if (index_of(tid) == -1) {
            tx_ids[n_tx++] = tid;
        }
    }
}

// DFS para detectar ciclo (retorna 1 se ciclo)
int dfs_ciclo(int u) {
    cor[u] = 1;
    for (int v = 0; v < n_tx; v++) {
        if (adj[u][v]) {
            if (cor[v] == 1) 
                return 1;
            if (cor[v] == 0 && dfs_ciclo(v)) 
                return 1;
        }
    }
    cor[u] = 2;
    return 0;
}

// teste de seriabilidade por conflito
int teste_seriabilidade(Schedule *sch) {
    coletar_transacoes(sch);
    // inicializa grafo
    for (int i = 0; i < n_tx; i++)
        for (int j = 0; j < n_tx; j++)
            adj[i][j] = 0;
    
    // cria arestas para cada par de operacoes conflitantes
    for (int i = 0; i < sch->n_ops; i++) {
        Op *ei = &sch->ops[i];
        for (int j = i + 1; j < sch->n_ops; j++) {
            Op *ej = &sch->ops[j];
            if ((ei->tid != ej->tid) && (strcmp(ei->attr, ej->attr) == 0)) {
                if ((ei->op == 'W' && (ej->op == 'R' || ej->op == 'W')) ||
                    (ei->op == 'R' && ej->op == 'W')) {
                    int u = index_of(ei->tid);
                    int v = index_of(ej->tid);
                    adj[u][v] = 1;
                }
            }
        }
    }

    // detecta ciclo
    for (int i = 0; i < n_tx; i++) 
        cor[i] = 0;
    for (int i = 0; i < n_tx; i++) {
        if (cor[i] == 0 && dfs_ciclo(i)) 
        return 0; // ciclo -> nao serial
    }

    return 1; // sem ciclos -> serial
}

// gera escalonamento serial a partir de permutacao de tids
void gerar_serial(Schedule *ser, Schedule *orig, int perm[]) {
    ser->n_ops = 0;
    for (int k = 0; k < n_tx; k++) {
        int tid = tx_ids[perm[k]];
        for (int i = 0; i < orig->n_ops; i++) {
            if (orig->ops[i].tid == tid) 
                ser->ops[ser->n_ops++] = orig->ops[i];
        }
    }
}

// teste de equivalencia de visao
int teste_visao(Schedule *orig) {
    coletar_transacoes(orig);
    int m = orig->n_ops;

    // captura informações de leituras em orig
    ReadInfo orig_reads[MAX_OPS];
    int n_reads = 0;
    for (int i = 0; i < m; i++) {
        if (orig->ops[i].op == 'R') {
            // calcula rank desta leitura (quantas leituras anteriores mesmo tid+attr)
            int rank = 0;
            for (int k = 0; k < i; k++) {
                if (orig->ops[k].op == 'R' && orig->ops[k].tid == orig->ops[i].tid && strcmp(orig->ops[k].attr, orig->ops[i].attr) == 0)
                    rank++;
            }
            // identifica writer que forneceu valor (ultimo W antes)
            int writer = -1;
            for (int k = i-1; k >=0; k--) {
                if (orig->ops[k].op == 'W' && strcmp(orig->ops[k].attr, orig->ops[i].attr) == 0) {
                    writer = orig->ops[k].tid;
                    break;
                }
            }

            orig_reads[n_reads].tid = orig->ops[i].tid;
            strcpy(orig_reads[n_reads].attr, orig->ops[i].attr);
            orig_reads[n_reads].rank = rank;
            orig_reads[n_reads].writer = writer;
            n_reads++;
        }
    }
    // identifica ultimo escritor de cada atributo em orig
    int last_writer_attr[256]; int has_last[256] = {0};

    for (int i = 0; i < m; i++) {
        if (orig->ops[i].op == 'W') {
            unsigned char h = 0;
            for (char *p = orig->ops[i].attr; *p; p++) 
                h += *p;

            last_writer_attr[h] = orig->ops[i].tid;
            has_last[h] = 1;
        }
    }

    // gera permutacoes possiveis e testa
    int perm[MAX_TX]; 
    for (int i = 0; i < n_tx ; i++) 
        perm[i]=i;
    Schedule ser;
    do {
        gerar_serial(&ser, orig, perm);
        // captura leituras em ser
        ReadInfo ser_reads[MAX_OPS]; 
        int n_ser_reads = 0;
        for (int i = 0; i < ser.n_ops; i++) {
            if (ser.ops[i].op == 'R') {
                int rank = 0;
                for (int k = 0; k < i; k++) {
                    if (ser.ops[k].op == 'R' && ser.ops[k].tid == ser.ops[i].tid && strcmp(ser.ops[k].attr, ser.ops[i].attr) == 0)
                        rank++;
                }

                int writer = -1;
                for (int k = i-1; k >= 0; k--) {
                    if (ser.ops[k].op == 'W' && strcmp(ser.ops[k].attr, ser.ops[i].attr) == 0) {
                        writer = ser.ops[k].tid; 
                        break;
                    }
                }

                ser_reads[n_ser_reads].tid = ser.ops[i].tid;
                strcpy(ser_reads[n_ser_reads].attr, ser.ops[i].attr);
                ser_reads[n_ser_reads].rank = rank;
                ser_reads[n_ser_reads].writer = writer;
                n_ser_reads++;
            }
        }
        // compara leitura a leitura
        int ok = 1;
        if (n_ser_reads != n_reads) 
            ok = 0;

        for (int r = 0; r < n_reads && ok; r++) {
            // encontra leitura correspondente em ser_reads
            int found = 0;
            for (int s = 0; s < n_ser_reads; s++) {
                if (orig_reads[r].tid == ser_reads[s].tid &&
                    strcmp(orig_reads[r].attr, ser_reads[s].attr) == 0 &&
                    orig_reads[r].rank == ser_reads[s].rank) {
                    if (orig_reads[r].writer != ser_reads[s].writer) 
                        ok = 0;
                    found = 1; 
                    break;
                }
            }
            if (!found) 
                ok = 0;
        }

        // compara ultimo escritor
        for (int h = 0; h < 256 && ok; h++) {
            if (has_last[h]) {
                // em ser: busca ultimo W para esse h
                int lw = -1; int present = 0;
                for (int i = 0; i < ser.n_ops; i++) {
                    if (ser.ops[i].op == 'W') {
                        unsigned char hh = 0;
                        for (char *p = ser.ops[i].attr; *p; p++) 
                            hh += *p;
                        if (hh == h) { 
                            lw = ser.ops[i].tid; 
                            present=1; 
                        }
                    }
                }
                if (!present || lw!= last_writer_attr[h]) 
                    ok = 0;
            }
        }
        if (ok) 
            return 1;
    } while (next_permutation(perm, n_tx));
    return 0;
}

// processa um escalonamento completo e imprime resultado
void processar_escalonamento(Schedule *sch, int id) {
    coletar_transacoes(sch);
    printf("%d ", id);
    for (int i = 0; i < n_tx; i++) {
        printf("%d%s", tx_ids[i], i+1<n_tx?",":"");
    }
    printf(" %s %s\n",
        teste_seriabilidade(sch)?"SS":"NS",
        teste_visao(sch)?"SV":"NV");
}

// verifica se tid ja este na lista
int presente(int *arr, int n, int tid) {
    for (int i = 0; i < n; i++) 
        if (arr[i] == tid) 
            return 1;
    return 0;
}

int main() {
    Schedule sch; sch.n_ops = 0;
    int active_tids[MAX_TX]; int n_active=0;
    int schedule_id=1;
    while (1) {
        Op op;
        if (scanf("%d %d %c %s", &op.time, &op.tid, &op.op, op.attr)!=4) break;
        sch.ops[sch.n_ops++]=op;
        if (op.op=='R'||op.op=='W') {
            if (!presente(active_tids, n_active, op.tid)) active_tids[n_active++]=op.tid;
        } else if (op.op=='C') {
            for (int i=0;i<n_active;i++) if (active_tids[i]==op.tid) {
                active_tids[i]=active_tids[--n_active]; break;
            }
        }
        if (n_active==0 && sch.n_ops>0) {
            processar_escalonamento(&sch, schedule_id++);
            sch.n_ops=0; n_active=0;
        }
    }
    return 0;
}
