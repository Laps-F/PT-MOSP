#include <iostream>
#include <vector>
#include <algorithm>
#include <set>
#include <numeric>
#include <random>
#include <string>

// Estrutura genérica simulando os dados da instância (Matriz M)
struct Instance {
    int num_customers; // Peças/Pilhas
    int num_products;  // Padrões/Estágios
    
    // pattern_contains_piece[p][c] == true se o produto 'p' atende o cliente 'c'
    // Note: Em C++, os índices começarão em 0.
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
Instance ReadInstance() {
    Instance inst;
    if (!(std::cin >> inst.num_customers >> inst.num_products)) {
        std::cerr << "Erro ao ler as dimensoes." << std::endl;
        exit(1);
    }

    inst.pattern_contains_piece.assign(inst.num_products, std::vector<bool>(inst.num_customers, false));

    for (int p = 0; p < inst.num_products; ++p) {
        for (int c = 0; c < inst.num_customers; ++c) {
            int val;
            std::cin >> val;
            if (val == 1) {
                inst.pattern_contains_piece[p][c] = true;
            }
        }
    }
    return inst;
}

// ========================================================================
// AVALIAÇÃO DO CUSTO (O CÁLCULO DE PILHAS ABERTAS)
// ========================================================================
// Calcula o máximo de pilhas abertas simultaneamente para uma dada sequência
int EvaluateCost(const Solution& s, const Instance& inst) {
    int max_open_stacks = 0;
    
    // Para cada cliente (peça), descobrimos em qual estágio a pilha abre e fecha
    std::vector<int> first_stage(inst.num_customers, -1);
    std::vector<int> last_stage(inst.num_customers, -1);

    for (int stage = 0; stage < inst.num_products; ++stage) {
        int current_pattern = s.sequencia_de_padroes[stage];
        for (int c = 0; c < inst.num_customers; ++c) {
            if (inst.pattern_contains_piece[current_pattern][c]) {
                if (first_stage[c] == -1) {
                    first_stage[c] = stage; // Pilha abriu
                }
                last_stage[c] = stage; // Atualiza o fechamento para o estágio mais recente visto
            }
        }
    }

    // Agora, para cada estágio, contamos quantas pilhas estão abertas (propriedade dos 1s consecutivos)
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

// Retorna o conjunto G de peças que compõem o gargalo
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

    // Identifica o(s) estágio(s) gargalo
    for (int stage = 0; stage < inst.num_products; ++stage) {
        int current_open_stacks = 0;
        std::set<int> active_pieces;
        
        for (int c = 0; c < inst.num_customers; ++c) {
            if (first_stage[c] != -1 && stage >= first_stage[c] && stage <= last_stage[c]) {
                current_open_stacks++;
                active_pieces.insert(c);
            }
        }
        
        // Se este estágio atinge o custo máximo, as peças dele fazem parte do gargalo
        if (current_open_stacks == s.cost) {
            G.insert(active_pieces.begin(), active_pieces.end());
        }
    }
    return G;
}

// Calcula a similaridade de um padrão p com o conjunto de gargalo G
int CalculateSimilarity(int pattern, const std::set<int>& G, const Instance& inst) {
    int similarity = 0;
    for (int piece : G) {
        if (inst.pattern_contains_piece[pattern][piece]) {
            similarity++;
        }
    }
    return similarity;
}

// Algorithm 1: ReduceC1s
Solution ReduceC1s(Solution s, const Instance& inst) {
    bool improvement;
    do {
        improvement = false;
        
        std::set<int> G = GetBottleneckPieces(s, inst);
        
        std::vector<int> L;
        for (int p : s.sequencia_de_padroes) {
            if (CalculateSimilarity(p, G, inst) > 0) {
                L.push_back(p);
            }
        }
        
        std::sort(L.begin(), L.end(), [&](int p1, int p2) {
            return CalculateSimilarity(p1, G, inst) > CalculateSimilarity(p2, G, inst);
        });
        
        for (int p : L) {
            auto it = std::find(s.sequencia_de_padroes.begin(), s.sequencia_de_padroes.end(), p);
            if (it != s.sequencia_de_padroes.end()) {
                s.sequencia_de_padroes.erase(it);
            }
            
            Solution best_insertion = s; // Cópia de segurança caso não melhore
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
// GERAÇÃO DOS VIZINHOS (K-SWAP AGRUPADO)
// ========================================================================
// Helper: Gera combinações (N escolhe K) de grupos
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
    
    int gamma = 2; // Tamanho dos grupos
    int k_max = 5; // Limite do K-swap
    
    std::random_device rd;
    std::mt19937 rng(rd());

    do {
        int k = 2; 
        improvement = false;
        int num_groups = s.sequencia_de_padroes.size() / gamma;

        while (k <= k_max && k <= num_groups) {
            bool found_better_in_k = false;
            
            // 1. Gera todas as combinações de troca de k grupos
            std::vector<std::vector<int>> all_possible_swaps = GenerateCombinations(num_groups, k);
            
            // 2. Explora randomicamente (First-Improvement)
            std::shuffle(all_possible_swaps.begin(), all_possible_swaps.end(), rng);

            for (const auto& swap_combination : all_possible_swaps) {
                // Aplica o swap
                Solution s_linha = ApplySwap(s, swap_combination, gamma, rng);
                s_linha.cost = EvaluateCost(s_linha, inst);
                
                // Aplica a busca cirúrgica
                s_linha = ReduceC1s(s_linha, inst);
                
                if (s_linha.cost < s.cost) {
                    s = s_linha;
                    improvement = true;
                    found_better_in_k = true;
                    break; // First improvement
                }
            }
            
            if (found_better_in_k) {
                break; // Volta para k=2 no do-while externo
            }
            k++; 
        }
        
    } while (improvement);

    return s;
}

// ========================================================================
// GERAÇÃO DOS VIZINHOS (JANELAS DESLIZANTES - NSD)
// ========================================================================

// Gera todos os pares de índices de início de janelas que NÃO se sobrepõem
std::vector<std::pair<int, int>> GenerateWindowPairs(int num_patterns, int omega) {
    std::vector<std::pair<int, int>> pairs;
    // A primeira janela começa no índice 'i'
    for (int i = 0; i <= num_patterns - omega; ++i) {
        // A segunda janela começa no índice 'j', pulando 'omega' casas para evitar sobreposição
        for (int j = i + omega; j <= num_patterns - omega; ++j) {
            pairs.push_back({i, j});
        }
    }
    return pairs;
}

// Troca os padrões de duas janelas (de tamanho omega) na sequência
Solution ApplyWindowSwap(const Solution& s, int idx1, int idx2, int omega) {
    Solution s_linha = s;
    for (int k = 0; k < omega; ++k) {
        std::swap(s_linha.sequencia_de_padroes[idx1 + k], s_linha.sequencia_de_padroes[idx2 + k]);
    }
    return s_linha;
}

// ========================================================================
// ALGORITHM 3: NSD (Nested Steepest Descent)
// ========================================================================
Solution NSD(Solution initial_solution, const Instance& inst) {
    Solution s = initial_solution;
    bool improvement;
    
    // Parâmetros do NSD (conforme o artigo)
    int omega = 2; // Tamanho da janela deslizante
    int nsd_iteration = 1;
    
    std::random_device rd;
    std::mt19937 rng(rd());

    // --- PAINEL INICIAL DE DEBUG ---
    std::cout << "\n============================================" << std::endl;
    std::cout << "=== INICIANDO OTIMIZACAO NSD ===" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "-> Clientes (Pilhas): " << inst.num_customers << std::endl;
    std::cout << "-> Padroes (Estagios): " << inst.num_products << std::endl;
    std::cout << "-> Tamanho da Janela (omega): " << omega << std::endl;
    std::cout << "-> Custo (Z_MOSP) Inicial: " << s.cost << std::endl;
    std::cout << "============================================\n" << std::endl;

    do {
        std::cout << "\n[Iteracao NSD: " << nsd_iteration++ << "] Custo Atual: " << s.cost << std::endl;
        improvement = false;
        
        // 1. Gera todas as combinações de 2 janelas que não compartilham padrões
        std::vector<std::pair<int, int>> window_pairs = GenerateWindowPairs(s.sequencia_de_padroes.size(), omega);
        std::cout << "  -> Explorando Vizinhanca 2-swap (" << window_pairs.size() << " pares de janelas disponiveis)" << std::endl;
        
        // Randomiza a ordem (mantendo a estratégia First-Improvement)
        std::shuffle(window_pairs.begin(), window_pairs.end(), rng);

        int eval_count = 0;
        for (const auto& wp : window_pairs) {
            eval_count++;
            
            if (eval_count % 500 == 0) {
                std::cout << "    Testando vizinho " << eval_count << " de " << window_pairs.size() << "...\r";
                std::cout.flush();
            }

            // Aplica a troca de janelas
            Solution s_linha = ApplyWindowSwap(s, wp.first, wp.second, omega);
            s_linha.cost = EvaluateCost(s_linha, inst);
            
            // Aplica a busca local cirúrgica do gargalo
            s_linha = ReduceC1s(s_linha, inst);
            
            if (s_linha.cost < s.cost) {
                std::cout << "                                                                 \r"; 
                std::cout << "    *** FIRST-IMPROVEMENT ACEITO! Custo Global caiu de " << s.cost << " para " << s_linha.cost << " ***" << std::endl;
                s = s_linha;
                improvement = true;
                break; // Achou melhora, atualiza a solução base e recomeça as janelas
            }
        }
        
        if (!improvement && window_pairs.size() >= 500) {
             std::cout << "                                                                 \r";
        }
        
        if (!improvement) {
            std::cout << "  -> Sem melhoria. NSD finalizado." << std::endl;
        }
        
    } while (improvement);

    std::cout << "\n=== Fim do NSD (Otimo Local Alcancado) ===" << std::endl;
    return s;
}

// ========================================================================
// MAIN
// ========================================================================
int main() {
    // 1. Lê a instância via entrada padrão (stdin)
    Instance inst = ReadInstance();

    // 2. Solução Inicial trivial (ordem 0, 1, 2... N-1)
    Solution initial_solution;
    initial_solution.sequencia_de_padroes.resize(inst.num_products);
    std::iota(initial_solution.sequencia_de_padroes.begin(), initial_solution.sequencia_de_padroes.end(), 0);
    initial_solution.cost = EvaluateCost(initial_solution, inst);

    std::cout << "Custo inicial trivial: " << initial_solution.cost << std::endl;

    // 3. Executa o NVND
    Solution best_solution = NSD(initial_solution, inst);

    // 4. Exibe os resultados
    std::cout << "Melhor custo encontrado: " << best_solution.cost << std::endl;
    std::cout << "Sequencia final: ";
    for (int p : best_solution.sequencia_de_padroes) {
        std::cout << p + 1 << " "; // +1 apenas para alinhar com o formato humano (1 a P)
    }
    std::cout << std::endl;

    return 0;
}