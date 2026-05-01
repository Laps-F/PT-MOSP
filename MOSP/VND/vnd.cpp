#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <set>
#include <numeric>
#include <random>
#include <string>
#include <chrono>
#include <filesystem>
#include "results.h"

// Atalho para facilitar a passagem de tempo
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

// Estrutura genérica simulando os dados da instância (Matriz M)
struct Instance {
    int num_customers; // Peças/Pilhas
    int num_products;  // Padrões/Estágios
    std::vector<std::vector<bool>> pattern_contains_piece; 
};

// Estrutura da solução
struct Solution {
    std::vector<int> sequencia_de_padroes;
    int cost; // Máximo de pilhas abertas simultaneamente
};

// ========================================================================
// LEITURA DA INSTÂNCIA
// ========================================================================
bool ReadInstanceFromFile(const std::string& filename, Instance& inst) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    file >> inst.num_customers >> inst.num_products;
    inst.pattern_contains_piece.assign(inst.num_products, std::vector<bool>(inst.num_customers, false));

    int val;
        for (int j = 0; j < inst.num_products; ++j) {
    for (int i = 0; i < inst.num_customers; ++i) {
            file >> val;
            if (val == 1) {
                inst.pattern_contains_piece[j][i] = true;
            }
        }
    }

    file.close();
    return true;
}

// ========================================================================
// Função utilitária para checar o tempo
// ========================================================================
bool IsTimeUp(TimePoint start_time, double time_limit) {
    if (time_limit <= 0.0) return false;
    auto current_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = current_time - start_time;
    return elapsed.count() >= time_limit;
}

// ========================================================================
// AVALIAÇÃO DO CUSTO
// ========================================================================
int EvaluateCost(const Solution& s, const Instance& inst) {
    int max_open_stacks = 0;
    std::vector<int> first_stage(inst.num_customers, -1);
    std::vector<int> last_stage(inst.num_customers, -1);

    for (int stage = 0; stage < inst.num_products; ++stage) {
        int current_pattern = s.sequencia_de_padroes[stage];
        for (int c = 0; c < inst.num_customers; ++c) {
            if (inst.pattern_contains_piece[current_pattern][c]) {
                if (first_stage[c] == -1) first_stage[c] = stage; 
                last_stage[c] = stage; 
            }
        }
    }

    for (int stage = 0; stage < inst.num_products; ++stage) {
        int current_open_stacks = 0;
        for (int c = 0; c < inst.num_customers; ++c) {
            if (first_stage[c] != -1 && stage >= first_stage[c] && stage <= last_stage[c]) {
                current_open_stacks++;
            }
        }
        if (current_open_stacks > max_open_stacks) {
            max_open_stacks = current_open_stacks;
        }
    }

    return max_open_stacks;
}

// ========================================================================
// BUSCA LOCAL CIRÚRGICA (REDUCE C1S)
// ========================================================================
std::set<int> GetBottleneckPieces(const Solution& s, const Instance& inst) {
    std::set<int> G;
    std::vector<int> first_stage(inst.num_customers, -1);
    std::vector<int> last_stage(inst.num_customers, -1);

    for (int stage = 0; stage < inst.num_products; ++stage) {
        int current_pattern = s.sequencia_de_padroes[stage];
        for (int c = 0; c < inst.num_customers; ++c) {
            if (inst.pattern_contains_piece[current_pattern][c]) {
                if (first_stage[c] == -1) first_stage[c] = stage;
                last_stage[c] = stage;
            }
        }
    }

    for (int stage = 0; stage < inst.num_products; ++stage) {
        int current_open_stacks = 0;
        std::set<int> active_pieces;
        for (int c = 0; c < inst.num_customers; ++c) {
            if (first_stage[c] != -1 && stage >= first_stage[c] && stage <= last_stage[c]) {
                current_open_stacks++;
                active_pieces.insert(c);
            }
        }
        if (current_open_stacks == s.cost) {
            G.insert(active_pieces.begin(), active_pieces.end());
        }
    }
    return G;
}

int CalculateSimilarity(int pattern, const std::set<int>& G, const Instance& inst) {
    int similarity = 0;
    for (int piece : G) {
        if (inst.pattern_contains_piece[pattern][piece]) similarity++;
    }
    return similarity;
}

Solution ReduceC1s(Solution s, const Instance& inst, TimePoint start_time, double time_limit) {
    bool improvement;
    do {
        improvement = false;
        
        // Checagem de tempo
        if (IsTimeUp(start_time, time_limit)) break;
        
        std::set<int> G = GetBottleneckPieces(s, inst);
        
        std::vector<int> L;
        for (int p : s.sequencia_de_padroes) {
            if (CalculateSimilarity(p, G, inst) > 0) L.push_back(p);
        }
        
        std::sort(L.begin(), L.end(), [&](int p1, int p2) {
            return CalculateSimilarity(p1, G, inst) > CalculateSimilarity(p2, G, inst);
        });
        
        for (int p : L) {
            // Checagem de tempo no laço interno
            if (IsTimeUp(start_time, time_limit)) break;

            auto it = std::find(s.sequencia_de_padroes.begin(), s.sequencia_de_padroes.end(), p);
            if (it != s.sequencia_de_padroes.end()) {
                s.sequencia_de_padroes.erase(it);
            }
            
            Solution best_insertion = s; 
            best_insertion.cost = 999999;
            
            for (size_t i = 0; i <= s.sequencia_de_padroes.size(); ++i) {
                Solution temp_s = s;
                temp_s.sequencia_de_padroes.insert(temp_s.sequencia_de_padroes.begin() + i, p);
                temp_s.cost = EvaluateCost(temp_s, inst);
                
                if (temp_s.cost < best_insertion.cost) {
                    best_insertion = temp_s;
                }
            }
            
            if (best_insertion.cost < s.cost) {
                s = best_insertion;
                improvement = true; 
            } else {
                s = best_insertion; 
            }
        }
    } while (improvement);

    return s;
}

// ========================================================================
// GERAÇÃO DOS VIZINHOS (K-SWAP AGRUPADO E NVND)
// ========================================================================
void CombinationsHelper(int n, int k, int start, std::vector<int>& current, std::vector<std::vector<int>>& result) {
    if (current.size() == k) {
        result.push_back(current);
        return;
    }
    for (int i = start; i < n; ++i) {
        current.push_back(i);
        CombinationsHelper(n, k, i + 1, current, result);
        current.pop_back();
    }
}

std::vector<std::vector<int>> GenerateCombinations(int n, int k) {
    std::vector<std::vector<int>> result;
    std::vector<int> current;
    CombinationsHelper(n, k, 0, current, result);
    return result;
}

Solution ApplySwap(const Solution& s, const std::vector<int>& selected_groups, int gamma, std::mt19937& rng) {
    Solution s_linha = s;
    std::vector<int> shuffled_groups = selected_groups;
    std::shuffle(shuffled_groups.begin(), shuffled_groups.end(), rng);

    for (size_t i = 0; i < selected_groups.size(); ++i) {
        int original_idx = selected_groups[i] * gamma;
        int new_idx = shuffled_groups[i] * gamma;
        
        for (int j = 0; j < gamma; ++j) {
            if (original_idx + j < s.sequencia_de_padroes.size() && new_idx + j < s.sequencia_de_padroes.size()) {
                s_linha.sequencia_de_padroes[original_idx + j] = s.sequencia_de_padroes[new_idx + j];
            }
        }
    }
    return s_linha;
}

Solution NVND(Solution initial_solution, const Instance& inst, bool verbose, TimePoint start_time, double time_limit) {
    Solution s = initial_solution;
    bool improvement;
    int gamma = 2; 
    int k_max = 5; 
    
    std::random_device rd;
    std::mt19937 rng(rd());

    do {
        if (IsTimeUp(start_time, time_limit)) {
            if(verbose) std::cout << "\n[!] Tempo limite de " << time_limit << "s atingido! Abortando NVND." << std::endl;
            break;
        }

        int k = 2; 
        improvement = false;
        int num_groups = s.sequencia_de_padroes.size() / gamma;

        while (k <= k_max && k <= num_groups) {
            if (IsTimeUp(start_time, time_limit)) break;

            bool found_better_in_k = false;
            std::vector<std::vector<int>> all_possible_swaps = GenerateCombinations(num_groups, k);
            std::shuffle(all_possible_swaps.begin(), all_possible_swaps.end(), rng);

            for (const auto& swap_combination : all_possible_swaps) {
                if (IsTimeUp(start_time, time_limit)) break;

                Solution s_linha = ApplySwap(s, swap_combination, gamma, rng);
                s_linha.cost = EvaluateCost(s_linha, inst);
                s_linha = ReduceC1s(s_linha, inst, start_time, time_limit);
                
                if (s_linha.cost < s.cost) {
                    s = s_linha;
                    improvement = true;
                    found_better_in_k = true;
                    break;
                }
            }
            if (found_better_in_k) break; 
            k++; 
        }
    } while (improvement);

    return s;
}

// ========================================================================
// GERAÇÃO DOS VIZINHOS (JANELAS DESLIZANTES - NSD)
// ========================================================================
std::vector<std::pair<int, int>> GenerateWindowPairs(int num_patterns, int omega) {
    std::vector<std::pair<int, int>> pairs;
    for (int i = 0; i <= num_patterns - omega; ++i) {
        for (int j = i + omega; j <= num_patterns - omega; ++j) {
            pairs.push_back({i, j});
        }
    }
    return pairs;
}

Solution ApplyWindowSwap(const Solution& s, int idx1, int idx2, int omega) {
    Solution s_linha = s;
    for (int k = 0; k < omega; ++k) {
        std::swap(s_linha.sequencia_de_padroes[idx1 + k], s_linha.sequencia_de_padroes[idx2 + k]);
    }
    return s_linha;
}

Solution NSD(Solution initial_solution, const Instance& inst, bool verbose, TimePoint start_time, double time_limit) {
    Solution s = initial_solution;
    bool improvement;
    int omega = 2; 
    int nsd_iteration = 1;
    
    std::random_device rd;
    std::mt19937 rng(rd());

    if(verbose){
        std::cout << "\n============================================" << std::endl;
        std::cout << "=== INICIANDO OTIMIZACAO NSD ===" << std::endl;
        std::cout << "-> Clientes (Pilhas): " << inst.num_customers << std::endl;
        std::cout << "-> Padroes (Estagios): " << inst.num_products << std::endl;
        std::cout << "-> Custo Inicial: " << s.cost << std::endl;
        std::cout << "-> Tempo Limite: " << time_limit << "s" << std::endl;
        std::cout << "============================================\n" << std::endl;
    }

    do {
        // Checagem no início da iteração NSD
        if (IsTimeUp(start_time, time_limit)) {
            if(verbose) std::cout << "\n[!] Tempo limite de " << time_limit << "s atingido! Abortando NSD." << std::endl;
            break;
        }

        if(verbose) std::cout << "\n[Iteracao NSD: " << nsd_iteration++ << "] Custo Atual: " << s.cost << std::endl;
        improvement = false;
        
        std::vector<std::pair<int, int>> window_pairs = GenerateWindowPairs(s.sequencia_de_padroes.size(), omega);
        std::shuffle(window_pairs.begin(), window_pairs.end(), rng);

        for (const auto& wp : window_pairs) {
            // Checagem rigorosa a cada par de janela avaliado!
            if (IsTimeUp(start_time, time_limit)) {
                improvement = false; // Garante a quebra do laço do-while externo
                break;
            }
            
            Solution s_linha = ApplyWindowSwap(s, wp.first, wp.second, omega);
            s_linha.cost = EvaluateCost(s_linha, inst);
            
            // O ReduceC1s também tem a própria checagem de tempo interna
            s_linha = ReduceC1s(s_linha, inst, start_time, time_limit);
            
            if (s_linha.cost < s.cost) {
                if(verbose) {
                    std::cout << "    *** FIRST-IMPROVEMENT ACEITO! Custo caiu para " << s_linha.cost << " ***" << std::endl;
                }
                s = s_linha;
                improvement = true;
                break; 
            }
        }
    } while (improvement);

    if(verbose) std::cout << "\n=== Fim do NSD ===" << std::endl;
    return s;
}

// ========================================================================
// MAIN (CLI Equivalente ao BRKGA)
// ========================================================================
int main(int argc, char* argv[]) {
    // Inicia o relógio O MAIS CEDO POSSÍVEL para contar o tempo real de execução
    auto start_time = std::chrono::high_resolution_clock::now();

    if (argc < 2) {
        std::cerr << "Uso: ./mainVND <caminho_para_instancia> [--INST <instance>] [--OUTDIR <outdir>] [--VERBOSE <0|1>] [--TIME_LIMIT <seconds>]" << std::endl;
        return 1;
    }

    int exec = 0;
    std::string outDir = "resultados";
    bool verbose = false;
    double time_limit = 1080.0; // Padrão: 18 minutos

    std::vector<std::string> arguments(argv + 1, argv + argc);
    std::string fn = arguments[0];

    // Read arguments
    for (size_t i = 1; i < arguments.size(); i += 2) {
        if (arguments[i] == "--INST")
            exec = std::stoi(arguments[i + 1]);
        else if (arguments[i] == "--OUTDIR")
            outDir = arguments[i + 1];
        else if (arguments[i] == "--VERBOSE")
            verbose = (std::stoi(arguments[i + 1]) != 0);
        else if (arguments[i] == "--TIME_LIMIT")
            time_limit = std::stod(arguments[i + 1]);
    }

    std::filesystem::create_directories(outDir + "/VND");
    outDir += "/VND";

    Instance inst;
    if (!ReadInstanceFromFile(fn, inst)) {
        return 1;
    }

    // Solução Inicial trivial (ordem 0, 1, 2... N-1)
    Solution initial_solution;
    initial_solution.sequencia_de_padroes.resize(inst.num_products);
    std::iota(initial_solution.sequencia_de_padroes.begin(), initial_solution.sequencia_de_padroes.end(), 0);
    initial_solution.cost = EvaluateCost(initial_solution, inst);

    // Executa o VND passando o tempo de início e o limite
    Solution best_solution = NSD(initial_solution, inst, verbose, start_time, time_limit);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

    // Extrai resultados
    SolResult res;
    res.evalSol = best_solution.cost;
    res.maxNumberPiecesPerPatern = 0;
    res.sol = best_solution.sequencia_de_padroes;
    int trocas = 0; 

    // Salva resultados via results.h
    saveResults(fn, res, static_cast<int>(elapsed.count()), trocas, outDir, exec, 2);

    return 0;
}
