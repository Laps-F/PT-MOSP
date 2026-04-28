#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <numeric>
#include <random>
#include <string>

// ========================================================================
// ESTRUTURAS DE DADOS
// ========================================================================
struct Instance {
    int num_customers; // Peças/Pilhas
    int num_products;  // Padrões/Estágios
    
    // Matriz original (usada em buscas específicas)
    std::vector<std::vector<bool>> pattern_contains_piece; 

    // --- ESTRUTURAS OTIMIZADAS PARA AVALIAÇÃO RÁPIDA ---
    // pieces_in_pattern[p] contém apenas os IDs dos clientes atendidos pelo padrão 'p'
    std::vector<std::vector<int>> pieces_in_pattern; 
    
    // piece_frequency[c] contém a quantidade total de padrões que contêm o cliente 'c'
    std::vector<int> piece_frequency; 
};

struct Solution {
    std::vector<int> sequencia_de_padroes;
    int cost; // Máximo de pilhas abertas simultaneamente (Z_MOSP)
};

// ========================================================================
// LEITURA E PRÉ-PROCESSAMENTO
// ========================================================================
Instance ReadInstance() {
    Instance inst;
    if (!(std::cin >> inst.num_customers >> inst.num_products)) {
        std::cerr << "Erro ao ler as dimensoes. Verifique o arquivo de entrada." << std::endl;
        exit(1);
    }

    inst.pattern_contains_piece.assign(inst.num_products, std::vector<bool>(inst.num_customers, false));
    inst.pieces_in_pattern.resize(inst.num_products);
    inst.piece_frequency.assign(inst.num_customers, 0);

    for (int p = 0; p < inst.num_products; ++p) {
    for (int c = 0; c < inst.num_customers; ++c) {
            int val;
            std::cin >> val;
            if (val == 1) {
                inst.pattern_contains_piece[p][c] = true;
                // Popula as estruturas otimizadas
                inst.pieces_in_pattern[p].push_back(c);
                inst.piece_frequency[c]++;
            }
        }
    }
    return inst;
}

// ========================================================================
// AVALIAÇÃO OTIMIZADA DO CUSTO (O "DECODER")
// ========================================================================
int EvaluateCost(const Solution& s, const Instance& inst) {
    int max_open_stacks = 0;
    int current_open_stacks = 0;

    // thread_local evita realocações dinâmicas de memória (ganho gigante de performance)
    thread_local std::vector<int> seen_count;
    if (seen_count.size() < inst.num_customers) {
        seen_count.resize(inst.num_customers, 0);
    } else {
        std::fill(seen_count.begin(), seen_count.begin() + inst.num_customers, 0);
    }

    for (int stage = 0; stage < inst.num_products; ++stage) {
        int pattern = s.sequencia_de_padroes[stage];

        // 1. Abertura das pilhas
        for (int piece : inst.pieces_in_pattern[pattern]) {
            if (seen_count[piece] == 0) {
                current_open_stacks++; // Abre nova pilha
            }
            seen_count[piece]++;
        }

        // 2. Registra o gargalo
        if (current_open_stacks > max_open_stacks) {
            max_open_stacks = current_open_stacks;
        }

        // 3. Fechamento das pilhas
        for (int piece : inst.pieces_in_pattern[pattern]) {
            if (seen_count[piece] == inst.piece_frequency[piece]) {
                current_open_stacks--; // Fecha a pilha
            }
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
        int pattern = s.sequencia_de_padroes[stage];
        for (int c : inst.pieces_in_pattern[pattern]) {
            if (first_stage[c] == -1) first_stage[c] = stage;
            last_stage[c] = stage;
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

Solution ReduceC1s(Solution s, const Instance& inst) {
    bool improvement;
    do {
        improvement = false;
        std::set<int> G = GetBottleneckPieces(s, inst);
        
        std::vector<int> L;
        for (int p : s.sequencia_de_padroes) {
            if (CalculateSimilarity(p, G, inst) > 0) L.push_back(p);
        }
        
        std::sort(L.begin(), L.end(), [&](int p1, int p2) {
            return CalculateSimilarity(p1, G, inst) > CalculateSimilarity(p2, G, inst);
        });
        
        for (int p : L) {
            auto it = std::find(s.sequencia_de_padroes.begin(), s.sequencia_de_padroes.end(), p);
            if (it != s.sequencia_de_padroes.end()) s.sequencia_de_padroes.erase(it);
            
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
                // std::cout << "      [ReduceC1s] Gargalo resolvido! Custo caiu de " << s.cost << " para " << best_insertion.cost << " (Padrao " << p + 1 << " reinserido)\n";
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
// GERAÇÃO DOS VIZINHOS (K-SWAP AGRUPADO)
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

// ========================================================================
// ALGORITHM 2: NVND
// ========================================================================
Solution NVND(Solution initial_solution, const Instance& inst) {
    Solution s = initial_solution;
    bool improvement;
    
    // Parâmetros da Meta-heurística (conforme o artigo)
    int gamma = 2; 
    int k_max = 5; 
    int nvnd_iteration = 1;
    int num_groups = s.sequencia_de_padroes.size() / gamma;
    
    std::random_device rd;
    std::mt19937 rng(rd());

    // --- PAINEL INICIAL DE DEBUG ---
    std::cout << "\n============================================" << std::endl;
    std::cout << "=== INICIANDO OTIMIZACAO NVND ===" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "-> Clientes (Pilhas): " << inst.num_customers << std::endl;
    std::cout << "-> Padroes (Estagios): " << inst.num_products << std::endl;
    std::cout << "-> Total de Blocos (gamma=" << gamma << "): " << num_groups << std::endl;
    std::cout << "-> Limite da Vizinhanca (K_max): " << k_max << " blocos" << std::endl;
    std::cout << "-> Custo (Z_MOSP) Inicial: " << s.cost << std::endl;
    std::cout << "============================================\n" << std::endl;

    do {
        std::cout << "\n[Iteracao NVND: " << nvnd_iteration++ << "] Custo Atual: " << s.cost << std::endl;
        int k = 2; 
        improvement = false;
        
        while (k <= k_max && k <= num_groups) {
            bool found_better_in_k = false;
            
            std::vector<std::vector<int>> all_possible_swaps = GenerateCombinations(num_groups, k);
            std::cout << "  -> Explorando Vizinhanca k=" << k << " (" << all_possible_swaps.size() << " combinacoes de " << k << " blocos)" << std::endl;
            
            // Randomiza a ordem de exploração (First-Improvement rigoroso)
            std::shuffle(all_possible_swaps.begin(), all_possible_swaps.end(), rng);

            int eval_count = 0;
            for (const auto& swap_combination : all_possible_swaps) {
                eval_count++;
                
                if (eval_count % 500 == 0) {
                    std::cout << "    Testando vizinho " << eval_count << " de " << all_possible_swaps.size() << "...\r";
                    std::cout.flush();
                }

                Solution s_linha = ApplySwap(s, swap_combination, gamma, rng);
                s_linha.cost = EvaluateCost(s_linha, inst);
                
                // Aplica a busca cirúrgica
                s_linha = ReduceC1s(s_linha, inst);
                
                if (s_linha.cost < s.cost) {
                    std::cout << "                                                                 \r"; 
                    std::cout << "    *** FIRST-IMPROVEMENT ACEITO! Custo Global caiu de " << s.cost << " para " << s_linha.cost << " ***" << std::endl;
                    s = s_linha;
                    improvement = true;
                    found_better_in_k = true;
                    break; 
                }
            }
            
            if (!found_better_in_k && all_possible_swaps.size() >= 500) {
                 std::cout << "                                                                 \r";
            }
            
            if (found_better_in_k) {
                std::cout << "  -> Retornando para k=2 devido a melhoria." << std::endl;
                break; 
            } else {
                std::cout << "  -> Sem melhoria no k=" << k << ". Avancando para k=" << k + 1 << "..." << std::endl;
            }
            k++; 
        }
        
    } while (improvement);

    std::cout << "\n=== Fim do NVND (Otimo Local Alcancado) ===" << std::endl;
    return s;
}

// ========================================================================
// MAIN
// ========================================================================
int main() {
    Instance inst = ReadInstance();

    // Cria a solução trivial inicial (0, 1, 2, ..., P-1)
    Solution initial_solution;
    initial_solution.sequencia_de_padroes.resize(inst.num_products);
    std::iota(initial_solution.sequencia_de_padroes.begin(), initial_solution.sequencia_de_padroes.end(), 0);
    initial_solution.cost = EvaluateCost(initial_solution, inst);

    // Executa a meta-heurística
    Solution best_solution = NVND(initial_solution, inst);

    // Exibe os resultados finais
    std::cout << "\n============================================\n";
    std::cout << "RESULTADO FINAL\n";
    std::cout << "Melhor Custo (Z_MOSP): " << best_solution.cost << std::endl;
    std::cout << "Sequencia de Padroes: ";
    for (int p : best_solution.sequencia_de_padroes) {
        std::cout << p + 1 << " "; // Somamos 1 apenas para exibir no formato humano (1 a P)
    }
    std::cout << "\n============================================\n";

    return 0;
}