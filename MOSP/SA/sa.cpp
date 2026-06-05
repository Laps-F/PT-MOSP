#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <string>
#include <tuple>
#include <cstring>
#include <bitset>
#include <filesystem> 

#include "results.h"
#include "../MCNH/HNCM.h"

using namespace std;

// Constante igual ao do PT-MOSP
const int MAX_PIECES = 1000;

// Representação Estrutural da Instância MOSP
struct MOSPInstance {
    int num_items;
    int num_patterns;
    vector<vector<int>> items_per_pattern; 
    vector<int> total_patterns_per_item;
    
    // Estruturas de aceleração copiadas do PT-MOSP
    vector<bitset<MAX_PIECES>> pattern_bitsets;
    vector<vector<int>> patterns_per_piece;
};

// -------------------------------------------------------------------------
// AVALIAÇÃO E DECODIFICAÇÃO (Idêntica ao PT-MOSP)
// Recebe a sequência de PEÇAS, decodifica para padrões e avalia o custo
// -------------------------------------------------------------------------
int evaluate(const MOSPInstance& mosp, const vector<int>& piece_sequence) {
    // 1. Decodificação: Peças -> Padrões (Regra da Última Peça)
    vector<int> patternSequence;
    patternSequence.reserve(mosp.num_patterns);
    
    bitset<MAX_PIECES> piecesRead;
    bitset<MAX_PIECES> patternUsed;

    for (int piece : piece_sequence) {
        piecesRead.set(piece);
        const auto& candidatos = mosp.patterns_per_piece[piece];
        auto notRead = ~piecesRead;

        for (int pattern : candidatos) {
            if (!patternUsed.test(pattern) && (mosp.pattern_bitsets[pattern] & notRead).none()) {
                patternSequence.push_back(pattern);
                patternUsed.set(pattern);
            }
        }
    }

    // 2. Avaliação Padrão do Custo (Pilhas Abertas)
    const int OPEN = 1, CLOSED = 0;
    vector<int> stack(mosp.num_items, CLOSED);
    vector<int> vet = mosp.total_patterns_per_item;

    int openStacks = 0;
    int closedStacks = 0;
    int maxOpenStacks = -1;

    for (int index : patternSequence) {
        for (int peca : mosp.items_per_pattern[index]) {
            vet[peca]--;

            if (stack[peca] == CLOSED) {
                openStacks++;
                stack[peca] = OPEN;
            }

            if (vet[peca] == 0) {
                closedStacks++;
                stack[peca] = CLOSED;
            }
        }

        if (openStacks > maxOpenStacks) {
            maxOpenStacks = openStacks;
        }

        openStacks -= closedStacks;
        closedStacks = 0;
    }

    return maxOpenStacks;
}

// -------------------------------------------------------------------------
// OPERADORES DE VIZINHANÇA: Random Insertion (Idêntico ao PT-MOSP)
// -------------------------------------------------------------------------
pair<int, int> apply_random_insertion(vector<int>& seq, mt19937& rng) {
    uniform_int_distribution<int> distPattern(0, seq.size() - 1);
    
    int pieceFrom = 0, pieceTo = 0;
    do {
        pieceFrom = distPattern(rng);
        pieceTo = distPattern(rng);
    } while (pieceFrom == pieceTo);

    int element = seq[pieceFrom];
    seq.erase(seq.begin() + pieceFrom);

    if (pieceTo > pieceFrom) {
        pieceTo--; // ajuste porque o vetor ficou menor após remoção
    }

    seq.insert(seq.begin() + pieceTo, element);
    
    // Retorna os índices originais sorteados para o UNDO
    return {pieceFrom, pieceTo};
}

void undo_random_insertion(vector<int>& seq, int originalFrom, int originalTo) {
    // Localiza onde o elemento foi parar após o deslocamento
    int currentPos = (originalTo > originalFrom) ? (originalTo - 1) : originalTo;
    
    int element = seq[currentPos];
    seq.erase(seq.begin() + currentPos);
    seq.insert(seq.begin() + originalFrom, element);
}

// -------------------------------------------------------------------------
// MOTOR DO SIMULATED ANNEALING
// -------------------------------------------------------------------------
tuple<vector<int>, int, long long> simulated_annealing(const MOSPInstance& mosp, vector<int> initial_piece_seq) {
    double t_init = 10.0;
    double t_end = 0.01;
    int num_steps = 11;
    double time_per_step = 110.0; 
    
    vector<double> temperatures(num_steps);
    double inv_t_init = 1.0 / t_init;
    double inv_t_end = 1.0 / t_end;
    
    for (int i = 0; i < num_steps; i++) {
        double inv_t = inv_t_init + i * (inv_t_end - inv_t_init) / (num_steps - 1.0);
        temperatures[i] = 1.0 / inv_t;
    }
    
    vector<int> current_piece_seq = initial_piece_seq;
    int current_cost = evaluate(mosp, current_piece_seq);
    
    vector<int> best_piece_seq = current_piece_seq;
    int best_cost = current_cost;
    long long total_trocas = 0; 
    
    mt19937 rng(1337); // Semente fixa para repetibilidade, se necessário
    uniform_real_distribution<double> dist_prob(0.0, 1.0);
    
    // cout << "--- Iniciando Simulated Annealing (Vizinhança: Insertion) ---\n";
    // cout << "Custo da Solucao Inicial MCNH: " << current_cost << "\n";
    
    for (int step = 0; step < num_steps; step++) {
        double T = temperatures[step];
        // cout << "Faixa " << step + 1 << "/" << num_steps << " | Temp: " << T << "\n";
        
        auto step_start_time = chrono::steady_clock::now();
        
        while (true) {
            auto current_time = chrono::steady_clock::now();
            chrono::duration<double> elapsed = current_time - step_start_time;
            
            if (elapsed.count() >= time_per_step) break;
            
            // Aplica a perturbação
            auto [originalFrom, originalTo] = apply_random_insertion(current_piece_seq, rng);
            
            int neighbor_cost = evaluate(mosp, current_piece_seq);
            int delta_f = neighbor_cost - current_cost;
            
            // Critério de Aceitação de Metropolis
            if (delta_f <= 0 || dist_prob(rng) < exp(-delta_f / T)) {
                current_cost = neighbor_cost;
                total_trocas++; 
                
                if (current_cost < best_cost) {
                    best_piece_seq = current_piece_seq;
                    best_cost = current_cost;
                }
            } else {
                // Reverte in-place
                undo_random_insertion(current_piece_seq, originalFrom, originalTo);
            }
        }
        // cout << "  -> Melhor global atual: " << best_cost << "\n";
    }
    
    return {best_piece_seq, best_cost, total_trocas};
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <arquivo.txt> [--INST <id>] [--OUTDIR <dir>]\n";
        return 1;
    }

    string filename = argv[1];
    int inst_id = 0;
    string outdir = ".";
    int replicas = 1;

    for (int i = 2; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--INST" && i + 1 < argc) inst_id = stoi(argv[++i]);
        else if (arg == "--OUTDIR" && i + 1 < argc) outdir = argv[++i];
    }

    std::filesystem::create_directories(outdir + "/SA");
    outdir += "/SA";

    // -------------------------------------------------------------------------
    // LEITURA DO ARQUIVO E INICIALIZAÇÃO DAS ESTRUTURAS (Base-zero garantida)
    // -------------------------------------------------------------------------
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro: Nao foi possivel abrir o arquivo: " << filename << "\n";
        return 1;
    }

    MOSPInstance mosp;
    // O padrão aqui assume formatação onde num_patterns vem antes de num_items
    // Se seus arquivos usarem ordem invertida, inverta estas duas variáveis conforme seu PT-MOSP
    if (!(file >> mosp.num_patterns >> mosp.num_items)) {
        cerr << "Erro: Falha ao ler dimensoes.\n";
        return 1;
    }

    mosp.items_per_pattern.resize(mosp.num_patterns);
    mosp.total_patterns_per_item.assign(mosp.num_items, 0);
    mosp.pattern_bitsets.resize(mosp.num_patterns);
    mosp.patterns_per_piece.resize(mosp.num_items);

    int max_pieces_per_pattern = 0;

    // Assumindo leitura "dos outros arquivos" (Padrões nas colunas e peças nas linhas)
    // Caso seja formato Rafael, inverta o i e j de acordo com o construtor original do PT
    for (int j = 0; j < mosp.num_patterns; j++) {
    for (int i = 0; i < mosp.num_items; i++) {
            int value;
            file >> value;
            if (value == 1) {
                mosp.items_per_pattern[j].push_back(i); 
                mosp.total_patterns_per_item[i]++;
            }
        }
    }
    file.close();

    for (int j = 0; j < mosp.num_patterns; j++) {
        if ((int)mosp.items_per_pattern[j].size() > max_pieces_per_pattern) {
            max_pieces_per_pattern = mosp.items_per_pattern[j].size();
        }
        // Configurando os bitsets
        for (int p : mosp.items_per_pattern[j]) {
            mosp.pattern_bitsets[j].set(p);
        }
    }

    for (int pattern = 0; pattern < mosp.num_patterns; ++pattern) {
        for (int piece = 0; piece < mosp.num_items; ++piece) {
            if (mosp.pattern_bitsets[pattern].test(piece)) {
                mosp.patterns_per_piece[piece].push_back(pattern);
            }
        }
    }

    // -------------------------------------------------------------------------
    // CONSTRUÇÃO DA SOLUÇÃO INICIAL COM MCNH (Como no PT-MOSP)
    // -------------------------------------------------------------------------
    int* solutionHNCM = nullptr;
    int resultHNCM = 0, solutionSize = 0;   
    double timeHNCM = 0.0;

    char* filename_cstr = new char[filename.size() + 1];
    strcpy(filename_cstr, filename.c_str());

    mainMethod(&resultHNCM, &timeHNCM, filename_cstr, &solutionHNCM, &solutionSize, 1); 

    vector<int> initial_piece_seq;
    if (solutionHNCM != nullptr && solutionSize == mosp.num_items) {
        for(int i = 0; i < solutionSize; i++) {
            initial_piece_seq.push_back(solutionHNCM[i]);
        }
        delete[] solutionHNCM;
    } else {
        cerr << "Aviso: MCNH falhou. Inicializando sequencialmente.\n";
        for (int i = 0; i < mosp.num_items; i++) initial_piece_seq.push_back(i);
    }
    delete[] filename_cstr;

    // -------------------------------------------------------------------------
    // EXECUÇÃO SA
    // -------------------------------------------------------------------------
    auto global_start_time = chrono::steady_clock::now();
    
    auto [best_seq, best_cost, trocas] = simulated_annealing(mosp, initial_piece_seq);
    
    auto global_end_time = chrono::steady_clock::now();
    int elapsed_seconds = chrono::duration_cast<chrono::seconds>(global_end_time - global_start_time).count() + timeHNCM;

    // Salvar Resultados
    SolResult result;
    result.evalSol = (double)best_cost; 
    result.maxNumberPiecesPerPatern = max_pieces_per_pattern;
    result.sol = best_seq; 
    result.construcTime = timeHNCM; 

    saveResults(filename, result, elapsed_seconds, trocas, outdir, inst_id, replicas);
    
    // cout << "\n[CONCLUIDO] Custo Final: " << best_cost << " | Salvo em: " << outdir << "\n";

    return 0;
}