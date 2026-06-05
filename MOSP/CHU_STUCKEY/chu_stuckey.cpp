/* ---------------------------------------------------------------------------
   Simple dynamic programming solution to challenge problem
   copyright Maria Garcia de la Banda and Peter Stuckey, May 2005

   MODIFICADO PARA BENCHMARK MOSP:
   - Leitura de arquivo direta e suporte a --transpose
   - Timeout real em segundos (6 horas)
   - Exportação automática de logs para resultados_mosp.csv
   - Otimizado e alocado dinamicamente para lidar com ate 1024x1024
---------------------------------------------------------------------------- */
#define MYDEBUG 0

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>

#include "solver.h"
using namespace std;

// Variáveis de Cronometragem
auto mystart_time = chrono::high_resolution_clock::now();
double time_limit_sec = 21600.0; // 6 horas padrão

struct rusage starttime,optimaltime,totaltime,runtime,endtime,opttime;

char name[256];
bool apply_transpose = false;

int playbit[ARRAYPRODUCTS][MAXCUSTOMERS];
int prod_order[ARRAYPRODUCTS];
int cust_order[MAXCUSTOMERS];
int remove_order[MAXCUSTOMERS];

SET plays[ARRAYPRODUCTS];
SET cross[MAXCUSTOMERS];
SET ALLCUSTOMERS;
SET orig_cross[MAXCUSTOMERS];

int global_upperbound;
int global_lowerbound;
int given_upper = MAXCUSTOMERS;
int given_lower = 1;

int upperbound;
bool found_solution;
int num_relax = 0;
int close_open = 0;
int close_open_level = -1;

int CUSTOMERS; // No MOSP = Itens
int PRODUCTS;  // No MOSP = Padrões

bool benchmark = 0;
bool verbose = 0;
bool showstats = 1;
bool definite_move = 0;
bool better_move = 1;
bool old_move = 1;
bool use_hash = 1;
bool sort_moves = 1;
bool probe = 0;
bool relaxation = 1;

int relaxes = 0;
int relax_suc = 0;
int wasted = 0;

void save_log_csv(const string& status, int best_obj, double best_bound, double exec_time) {
    bool file_exists = false;
    ifstream fcheck("resultados_mosp.csv");
    if (fcheck.good()) file_exists = true;
    fcheck.close();

    ofstream csv("resultados_mosp.csv", ios::app);
    
    if (!file_exists) {
        csv << "Instancia,Modelo_Formulacao,Solver,Versao_Solver,Tempo_Limite_Seg,Tempo_Execucao_Seg,Status,Solucao_Corrente_Obj,Best_Bound,Gap_Percentual\n";
    }

    double gap = 0.0;
    if (best_obj > 0) {
        gap = ((double)(best_obj - best_bound) / best_obj) * 100.0;
    }
    
    csv << name << (apply_transpose ? "_transposta" : "") << ","
        << "DP_AStar" << ","
        << "Chu-Stuckey_C++" << ","
        << "2005" << ","
        << time_limit_sec << ","
        << exec_time << ","
        << status << ",";
        
    if (best_obj > 0 && best_obj <= CUSTOMERS) csv << best_obj;
    csv << "," << best_bound << "," << gap << "\n";
    csv.close();
}

class Node {
public:
    SET custs_open;
    SET custs;
    SET seconded;

    int level;
    int now_open;
    int free_stacks;
    int num_moves;
    SET played;
    bool dm;
    bool freeze;

    int played_queue[MAXCUSTOMERS];
    int num_played;
    int pqhead;

    SET allopenset[MAXCUSTOMERS];
    SET openset[MAXCUSTOMERS];
    SET closeset[MAXCUSTOMERS];
    int open[MAXCUSTOMERS];
    int close[MAXCUSTOMERS];
    int moves[MAXCUSTOMERS];
    double scores[MAXCUSTOMERS];

    SET mergeset;
    int num_merged;
    int removed_nodes[MAXCUSTOMERS];
    int merged_nodes[MAXCUSTOMERS];
    SET old_cross[MAXCUSTOMERS];

    void search();
    inline void initnode();
    inline void relax();
    inline void findunmerge();
    inline void mergenodes(int a, int b);
    inline void unmergenodes();
    inline void mergemoves();
    inline void finddefinitemove();
    inline void prunenotopen();
    inline void findoldmoves();
    inline void rankmoves();
    inline void sortmoves();
    inline void pruneworsemoves();
    inline void playmove();
    inline void finish();
    inline void getsolution();
    inline void extendsolution();
    inline void dumpnode();
    inline bool checkcompress();
};

// ALOCAÇÃO DINÂMICA: Substitui o array de escopo global para não estourar RAM
Node* nodes = nullptr;

void Node::initnode() {
    level = this-nodes;
    num_moves = 0;
    num_played = 0;
    played = emptyset;
    pqhead = 0;
    dm = false;
}

void Node::search() {
    if (probe && ccc[process_id] >= 100000) return;
    initnode();

    for (int i = 0; i < CUSTOMERS; i++) {
        if (!in(i,custs)) continue;
        if (cross[i].subset(custs_open)) custs.removebit(i);
    }

    if (custs == emptyset) {getsolution(); return;}
    // Deixado comentado conforme sua versão original:
    // if (use_hash && hash(custs_open & custs,custs)) {reuse[process_id]++; return;}

    now_open = (custs_open & custs).nbitsp();
    free_stacks = upperbound - now_open;        
    assert(now_open < upperbound);

    for (int i = 0; i < CUSTOMERS; i++) {
        if (!in(i,custs)) continue;
        loopcount[process_id]++;
        openset[i] = cross[i] - custs_open;
        open[i] = openset[i].nbitsp();
        if (open[i] <= free_stacks) moves[num_moves++] = i;
    }

    mergemoves();

    if (definite_move) finddefinitemove();
    if (close_open || old_move) findoldmoves();
    if (close_open || better_move) pruneworsemoves();
    if (sort_moves) sortmoves();
    if (probe || close_open) prunenotopen();

    while (num_moves) {
        playmove();
        if (found_solution) break;
        if (close_open || better_move) pruneworsemoves();
    }
    finish();
}

inline void Node::mergemoves() {
    int t = 0;
    for (int i = 0; i < num_moves; i++) {
        int best = moves[i];
        int best_i = i;
        for (int j = i+1; j < num_moves; j++) {
            if (open[moves[j]] < open[best] || (open[moves[j]] == open[best] && openset[moves[j]] < openset[best])) {
                best = moves[j];
                best_i = j;
            }
        }
        moves[best_i] = moves[i];
        int cur = t ? moves[t-1] : -1;
        if (cur == -1 || openset[best] != openset[cur]) {
            cur = best;
            moves[t++] = cur;
            allopenset[cur] = emptyset;
            closeset[cur] = emptyset;
            close[cur] = 0;
        }
        allopenset[cur] = allopenset[cur] + cross[best];
        closeset[cur].addbit(best);
        close[cur]++;
    }
    num_moves = t;
}

inline void Node::relax() {
    mergeset = emptyset;
    num_merged = 0;

    while (custs.nbitsp() > upperbound) {
        double w[MAXCUSTOMERS];
        int best_j[MAXCUSTOMERS];

        for (int i = 0; i < CUSTOMERS; i++) {
            if (!in(i,custs)) continue;
            w[i] = 1.0/(cross[i]-custs_open).nbitsp();
        }

        double safest = 1e100;
        int best_i = -1;

        for (int i = 0; i < CUSTOMERS; i++) {
            if (in(i,custs_open)) continue;
            double total_danger = 0;
            double total_weight = 0;
            double most_dangerous = -1;
            best_j[i] = -1;
            for (int j = 0; j < CUSTOMERS; j++) {
                if (!in(j,custs) || !in(j,cross[i]) || i == j) continue;
                double danger = w[j] * (cross[i] - custs_open - cross[j]).nbitsp();
                total_danger += danger;
                total_weight += w[j];
                if (danger > most_dangerous) {
                    most_dangerous = danger;
                    best_j[i] = j;
                }
            }
            if (best_j[i] == -1) continue;
            total_danger -= most_dangerous;
            double safety = total_danger / total_weight;
            if (safety < safest) {
                safest = safety;
                best_i = i;
                best_j[best_i] = best_j[i];
            }
        }
        if (best_i == -1) return;
        mergenodes(best_i,best_j[best_i]);
    }
}

inline void Node::findunmerge() {
    int old_removed_nodes[MAXCUSTOMERS];
    int old_merged_nodes[MAXCUSTOMERS];
    int old_cust_order[MAXCUSTOMERS];
    int old_num_merged = num_merged;

    for (int i = 0; i < CUSTOMERS; i++) old_cust_order[i] = cust_order[i];
    for (int i = 0; i < num_merged; i++) {
        old_removed_nodes[i] = removed_nodes[i];
        old_merged_nodes[i] = merged_nodes[i];
    }

    for (int i = old_num_merged; i--; ) {
        while (num_merged) unmergenodes();
        for (int j = 0; j < CUSTOMERS; j++) cust_order[j] = old_cust_order[j];
        for (int j = 0; j < old_num_merged; j++) if (i != j) {
            if (!in(old_removed_nodes[j],cross[old_merged_nodes[j]])) break;
            mergenodes(old_removed_nodes[j], old_merged_nodes[j]);
        }
        if (num_merged != old_num_merged-1) continue;
        removed_nodes[num_merged] = old_removed_nodes[i];
        extendsolution();
        if (!found_solution) return;
    }

    while (num_merged) unmergenodes();
    for (int j = 0; j < CUSTOMERS; j++) cust_order[j] = old_cust_order[j];
    for (int j = 0; j < old_num_merged-1; j++) mergenodes(old_removed_nodes[j], old_merged_nodes[j]);
    
    removed_nodes[num_merged] = old_removed_nodes[num_merged];
    extendsolution();
}

inline void Node::mergenodes(int a, int b) {
    removed_nodes[num_merged] = a;
    merged_nodes[num_merged] = b;
    old_cross[num_merged] = cross[b];

    for (int i = 0; i < CUSTOMERS; i++) {
        if (in(i,cross[a])) cross[i].addbit(b);
    }
    cross[b] = cross[b] + cross[a];

    mergeset.addbit(a);
    custs.removebit(a);
    custs_open.addbit(a);
    num_relax++;
    num_merged++;
}

inline void Node::unmergenodes() {
    int m = --num_merged;
    int rn = removed_nodes[m];
    int mn = merged_nodes[m];
    SET oc = old_cross[m];

    cross[mn] = oc;
    for (int i = 0; i < CUSTOMERS; i++) {
        if (!in(i,oc)) cross[i].removebit(mn);
    }
    mergeset.removebit(rn);
    custs.addbit(rn);
    custs_open.removebit(rn);
    num_relax--;
}

inline void Node::finddefinitemove() {
    for (int i = 0; i < num_moves; i++) {
        int s = moves[i];
        if (close[s] >= open[s]) {
            definite[process_id]++;
            dm = true;
        }
    }
}

inline void Node::prunenotopen() {
    if (!now_open) return;
    int s, t;
    for (s = t = 0; s < num_moves; s++) {
        if ((custs_open & closeset[moves[s]]) == emptyset) continue;
        moves[t++] = moves[s];
    }
    num_moves = t;
}

inline void Node::findoldmoves() {
    if (this == nodes) return;
    if ((this-1)->freeze) return;
    int m = 0;

    for (int i = 0; i < num_moves; i++) {
        int s = moves[i];
        int n = open[s] - (this-1)->close[s] + ((this-1)->custs - custs).nbitsp();
        if (in(s,((this-1)->played)) && n <= free_stacks) {
            played = played + closeset[s];
            played_queue[num_played++] = s;
        } else moves[m++] = s;
    }
    num_moves = m;
}

inline void Node::rankmoves() {
    for (int i = 0; i < num_moves; i++) {
        int s = moves[i];
        scores[s] = open[s] - close[s];
    }
}

inline void Node::sortmoves() {
    rankmoves();
    for (int i = 0; i < num_moves; i++) {
        int best_i = i;
        for (int j = i+1; j < num_moves; j++) {
            if (scores[moves[j]] < scores[moves[best_i]]) best_i = j;
        }
        int tmp = moves[i];
        moves[i] = moves[best_i];;
        moves[best_i] = tmp;
    }
}

inline void Node::pruneworsemoves() {
    while (pqhead < num_played) {
        int s = played_queue[pqhead++];
        int m = 0;
        for (int i = 0; i < num_moves; i++) {
            int t = moves[i];
            if (in(t,played)) continue;
            if ((openset[s] + openset[t]).nbitsp() <= close[s] + open[t]) {
                played.addbit(t);
                played_queue[num_played++] = t;
            } else moves[m++] = t;
        }
        num_moves = m;
    }
}

inline void Node::playmove() {
    Node *next = this+1;
    next->custs_open = custs_open + openset[moves[0]];
    next->custs = custs - closeset[moves[0]];
    next->seconded = seconded + (allopenset[moves[0]] & custs_open);
    next->search();
    played.addbit(moves[0]);
    played_queue[num_played++] = moves[0];
    num_moves--;
    for (int i = 0; i < num_moves; i++) moves[i] = moves[i+1];
}

inline void Node::finish() {
    if (probe && ccc[process_id] >= 100000) return;
    ccc[process_id]++;

    auto current_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = current_time - mystart_time;
    
    if (elapsed.count() >= time_limit_sec) {
        int best_found = upperbound + 1;
        printf("\n[TIMEOUT] Limite de tempo de %.2fs excedido.\n", time_limit_sec);
        
        save_log_csv("FEASIBLE", best_found, given_lower, elapsed.count());
        delete[] nodes;
        exit(0);
    }
}

inline void Node::getsolution() {
    int customer_index = 0;
    found_solution = true;

    for (int i = 0; i < level; i++) {
        SET real_closeset = nodes[i].custs - nodes[i+1].mergeset - nodes[i+1].custs;
        for (int j = 0; j < CUSTOMERS; j++) {
            if (!in(j,real_closeset)) continue;
            cust_order[customer_index++] = j;
        }
    }
}

inline void Node::extendsolution() {
    int n = CUSTOMERS - num_relax;
    cust_order[n-1] = removed_nodes[num_merged];

    SET my_custs_open = emptyset;
    SET my_custs = ALLCUSTOMERS;

    for (int i = 0; i <= level; i++) my_custs = my_custs - nodes[i].mergeset;

    for (int i = 0; i < n; i++) {
        int s = cust_order[i];
        int best_j = i;
        for (int j = i+1; j < n; j++) {
            int t = cust_order[j];
            if ((cross[t]-my_custs_open).subset(cross[s]-my_custs_open)) {
                s = t;
                best_j = j;
            }
        }
        for (int j = best_j; j > i; j--) cust_order[j] = cust_order[j-1];
        cust_order[i] = s;
        my_custs_open = my_custs_open + cross[s];
        if ((my_custs_open & my_custs).nbitsp() > upperbound) {
            found_solution = false;
            return;
        }
        my_custs.removebit(s);
    }
}

int getproductorder() {
    int best = 0;
    int product_index = 0;
    SET custs_open = emptyset;
    SET custs = ALLCUSTOMERS;
    bool played[ARRAYPRODUCTS];

    for (int i = 0; i < PRODUCTS; i++) played[i] = false;

    for (int i = 0; i < CUSTOMERS; i++) {
        custs_open = custs_open + cross[cust_order[i]];
        for (int j = 0; j < PRODUCTS; j++) {
            if (!played[j] && plays[j].subset(custs_open)) {
                played[j] = true;
                prod_order[product_index++] = j;
            }
        }
        int n = (custs_open & custs).nbitsp();
        best = safe_max(best,n);
        custs.removebit(cust_order[i]);
    }
    return best;
}

void readdata(const string& filename, bool transpose) {
    ifstream file(filename);
    if (!file.is_open()) {
        printf("ERRO: Nao foi possivel abrir o arquivo %s\n", filename.c_str());
        exit(1);
    }

    int M, N;
    file >> M >> N;

    if (transpose) {
        CUSTOMERS = N; // Itens
        PRODUCTS = M;  // Padroes
    } else {
        CUSTOMERS = M;
        PRODUCTS = N;
    }

    if (CUSTOMERS > MAXCUSTOMERS || PRODUCTS > ARRAYPRODUCTS) {
        printf("ERRO: Dimensoes excedem limites do solver.h! Aumente MAXCUSTOMERS e ARRAYPRODUCTS para pelo menos %d\n", max(M, N));
        exit(1);
    }

    for (int i = 0; i < PRODUCTS; i++) plays[i] = emptyset;
    for (int i = 0; i < CUSTOMERS; i++) cross[i] = emptyset;

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int val;
            file >> val;
            if (val == 1) {
                if (transpose) {
                    playbit[i][j] = 1;
                    plays[i].addbit(j);
                } else {
                    playbit[j][i] = 1;
                    plays[j].addbit(i);
                }
            } else {
                if (transpose) playbit[i][j] = 0;
                else playbit[j][i] = 0;
            }
        }
    }
    file.close();

    for (int i = 0; i < PRODUCTS; i++) {
        for (int j = 0; j < CUSTOMERS; j++) 
            if (in(j,plays[i]))
                cross[j] = cross[j] + plays[i];
    }

    for (int i = 0; i < CUSTOMERS; i++) {
        ALLCUSTOMERS.addbit(i);
        orig_cross[i] = cross[i];
    }
}

int astar(int upper, int lower) {
    int best = upper+1;
    for (int i = upper; i >= lower; i--) {
        upperbound = i;
        best = i+1;
        found_solution = false;
        if (relaxation && !probe) {
            nodes[0].relax();
            close_open = true;
        }
        while (true) {
            nodes[0].search();
            if (!found_solution) {
                if (!close_open) break;
                close_open = false;
                continue;
            }
            while (nodes[0].num_merged && found_solution) nodes[0].findunmerge();
            if (!nodes[0].num_merged && found_solution) break;
        }
        if (found_solution) best = getproductorder();
        
        auto curr = chrono::high_resolution_clock::now();
        chrono::duration<double> elap = curr - mystart_time;
        if (showstats) if (!probe || found_solution) printf("BACK (%d) asmin = %d, tempo = %.2fs\n", i, best, elap.count());
        
        if (!found_solution) break;
        i = best;
    }
    return best;
}

int solve() {
    int min = given_upper+1;
    if (probe) {
        min = astar(min-1,given_lower);
        probe = false;
    }
    min = astar(min-1,given_lower);
    return min;
}

void initsolver() {
    nodes[0].custs_open = emptyset;
    nodes[0].custs = ALLCUSTOMERS;
    nodes[0].seconded = emptyset;
}

int main(int argc, char *argv[]) {
    string filename = "";
    
    LIMIT[process_id] = 5000000000;
    given_lower = 1;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            filename = argv[++i];
            strcpy(name, filename.c_str());
        } else if (arg == "--transpose") {
            apply_transpose = true;
        } else if (arg == "-time" && i + 1 < argc) {
            time_limit_sec = stod(argv[++i]);
        } else if (arg == "-l" && i + 1 < argc) {
            given_lower = stoi(argv[++i]);
        } else if (arg == "-x") {
            benchmark = 1; showstats = 0; verbose = 0;
        }
    }

    if (filename == "") {
        printf("Uso: %s -f <arquivo.txt> [-time 21600] [--transpose]\n", argv[0]);
        fflush(stdout);
        exit(1);
    }

    printf("--- Instancia: %s %s ---\n", name, apply_transpose ? "(Transposta)" : "");
    printf("Tempo Limite: %.2fs\n\n", time_limit_sec);
    fflush(stdout);

    readdata(filename, apply_transpose);
    
    // ALOCAÇÃO DINÂMICA (Evita Segmentation Fault / Stack Overflow)
    nodes = new Node[MAXCUSTOMERS];
    
    initsolver();
    
    mystart_time = chrono::high_resolution_clock::now();
    
    int min_val = solve();
    
    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - mystart_time;

    printf("\n[SUCESSO] OTIMALIDADE PROVADA!\n");
    printf("Custo Otimo: %d | Tempo: %.2fs\n", min_val, elapsed.count());
    fflush(stdout);
    
    save_log_csv("OPTIMAL", min_val, min_val, elapsed.count());
    
    delete[] nodes;

    return 0;
}