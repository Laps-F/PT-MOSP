#include "mospDecoder.h"
#include <algorithm>
#include <numeric>
#include <limits>
#include <iostream>
#include <atomic>
#include <chrono>

MospDecoder::MospDecoder(int m, int n, const std::vector<std::vector<bool>>& matrix, std::chrono::high_resolution_clock::time_point start_time, double time_limit)
    : m(m), n(n), matrix(matrix), start_time(start_time), time_limit(time_limit) {
    // PRÉ-PROCESSAMENTO: Lista de itens por padrão
    items_in_pattern.resize(m);
    for (int p = 0; p < m; ++p) {
        for (int i = 0; i < n; ++i) {
            if (matrix[i][p]) {
                items_in_pattern[p].push_back(i);
            }
        }
    }
}

MospDecoder::~MospDecoder() {}

// --- MÉTODOS AUXILIARES ---

int MospDecoder::calculate_mosp(const std::vector<int>& partial_sequence) const {
    if (partial_sequence.empty()) return 0;

    int seq_size = partial_sequence.size();
    std::vector<int> first_occ(n, -1);
    std::vector<int> last_occ(n, -1);

    for (int k = 0; k < seq_size; ++k) {
        int pattern = partial_sequence[k];
        for (int i = 0; i < n; ++i) {
            if (matrix[i][pattern]) {
                if (first_occ[i] == -1) first_occ[i] = k;
                last_occ[i] = k;
            }
        }
    }

    std::vector<int> events(seq_size + 2, 0);
    for (int i = 0; i < n; ++i) {
        if (first_occ[i] != -1) {
            events[first_occ[i]] += 1;
            events[last_occ[i] + 1] -= 1;
        }
    }

    int max_mosp = 0;
    int open_stacks = 0;
    for (int k = 0; k < seq_size; ++k) {
        open_stacks += events[k];
        if (open_stacks > max_mosp) max_mosp = open_stacks;
    }

    return max_mosp;
}

std::vector<int> MospDecoder::get_open_stacks_per_pattern(const std::vector<int>& sequence) const {
    int seq_size = sequence.size();
    std::vector<int> open_stacks_list(seq_size, 0);

    if (seq_size == 0) return open_stacks_list;

    std::vector<int> first_occ(n, -1);
    std::vector<int> last_occ(n, -1);

    for (int k = 0; k < seq_size; ++k) {
        int pattern = sequence[k];
        for (int i = 0; i < n; ++i) {
            if (matrix[i][pattern]) {
                if (first_occ[i] == -1) first_occ[i] = k;
                last_occ[i] = k;
            }
        }
    }

    std::vector<int> events(seq_size + 2, 0);
    for (int i = 0; i < n; ++i) {
        if (first_occ[i] != -1) {
            events[first_occ[i]] += 1;
            events[last_occ[i] + 1] -= 1;
        }
    }

    int current = 0;
    for (int k = 0; k < seq_size; ++k) {
        current += events[k];
        open_stacks_list[k] = current;
    }

    return open_stacks_list;
}

int MospDecoder::calculate_mosp_with_insert(const std::vector<int>& partial_solution,
                                           const std::vector<int>& first_occ,
                                           const std::vector<int>& last_occ,
                                           int pattern,
                                           size_t insert_pos,
                                           std::vector<int>& events) const {
    int seq_size = static_cast<int>(partial_solution.size()) + 1;
    
    // Zera o buffer reutilizável (muito rápido, sem alocação)
    std::fill(events.begin(), events.begin() + seq_size + 2, 0);

    // Itens já presentes na solução parcial
    for (int item = 0; item < n; ++item) {
        if (first_occ[item] == -1) continue;

        int f = first_occ[item] >= static_cast<int>(insert_pos) ? first_occ[item] + 1 : first_occ[item];
        int l = last_occ[item] >= static_cast<int>(insert_pos) ? last_occ[item] + 1 : last_occ[item];
        events[f] += 1;
        events[l + 1] -= 1;
    }

    // Itens do novo padrão (usando pré-computação para evitar loop sobre n)
    for (int item : items_in_pattern[pattern]) {
        int f;
        int l;
        if (first_occ[item] == -1) {
            f = l = static_cast<int>(insert_pos);
        } else {
            int old_f = first_occ[item] >= static_cast<int>(insert_pos) ? first_occ[item] + 1 : first_occ[item];
            int old_l = last_occ[item] >= static_cast<int>(insert_pos) ? last_occ[item] + 1 : last_occ[item];
            events[old_f] -= 1;
            events[old_l + 1] += 1;
            f = std::min(old_f, static_cast<int>(insert_pos));
            l = std::max(old_l, static_cast<int>(insert_pos));
        }

        events[f] += 1;
        events[l + 1] -= 1;
    }

    int max_mosp = 0;
    int open_stacks = 0;
    for (int k = 0; k < seq_size; ++k) {
        open_stacks += events[k];
        if (open_stacks > max_mosp) max_mosp = open_stacks;
    }
    return max_mosp;
}


// --- DECODER PRINCIPAL ---
double MospDecoder::decode(const std::vector<double>& chromosome, std::vector<int>* solution) const {
    
    // VERIFICAÇÃO DE TEMPO:
    if (time_limit > 0.0) {
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = current_time - start_time;
        if (elapsed.count() >= time_limit) {
            // Se estourou o tempo, aborta o cálculo instantaneamente.
            // Retorna o "pior fitness possível" (assumindo que o problema é de MINIMIZAÇÃO).
            // O BRKGA vai ignorar esse cromossomo e a geração terminará quase na mesma hora.
            return std::numeric_limits<double>::max(); 
        }
    }

    // Buffer reutilizável para evitar alocações no loop interno
    std::vector<int> events_buffer(m + 2, 0);

    // 1. Decodificação (PIS - Pattern Insertion Sequence)
    std::vector<int> pis(m);
    std::iota(pis.begin(), pis.end(), 0);
    std::sort(pis.begin(), pis.end(), [&chromosome](int i1, int i2) {
        return chromosome[i1] < chromosome[i2];
    });

    // 2. Busca Local (Solution Builder)
    std::vector<int> partial_solution;
    partial_solution.reserve(m + 1);
    std::vector<int> first_occ(n, -1);
    std::vector<int> last_occ(n, -1);

    for (int pattern : pis) {
        int best_position = 0;
        int best_mosp = std::numeric_limits<int>::max();

        for (size_t i = 0; i <= partial_solution.size(); ++i) {
            int current_mosp = calculate_mosp_with_insert(partial_solution, first_occ, last_occ, pattern, i, events_buffer);
            if (current_mosp < best_mosp) {
                best_mosp = current_mosp;
                best_position = i;
            }
        }

        partial_solution.insert(partial_solution.begin() + best_position, pattern);

        // Atualização unificada dos first_occ e last_occ
        for (int item = 0; item < n; ++item) {
            if (matrix[item][pattern]) {
                // Item está no padrão: atualizar first_occ e last_occ
                int prev_first = first_occ[item];
                int prev_last = last_occ[item];
                int new_pos = static_cast<int>(best_position);

                if (prev_first >= new_pos && prev_first != -1) prev_first += 1;
                if (prev_last >= new_pos && prev_last != -1) prev_last += 1;

                if (first_occ[item] == -1) {
                    first_occ[item] = new_pos;
                    last_occ[item] = new_pos;
                } else {
                    first_occ[item] = std::min(prev_first, new_pos);
                    last_occ[item] = std::max(prev_last, new_pos);
                }
            } else {
                // Item não está no padrão: shift se necessário
                if (first_occ[item] >= static_cast<int>(best_position)) first_occ[item] += 1;
                if (last_occ[item] >= static_cast<int>(best_position)) last_occ[item] += 1;
            }
        }
    }

    // -------------------------------------------------------------
    // 3. FASE 3 DO ARTIGO: Chromosome Adjustment (Ajuste do Cromossomo)
    // -------------------------------------------------------------
    // Fazemos um cast no 'const' para podermos injetar o aprendizado 
    // da busca local de volta no DNA do BRKGA.
    std::vector<double>& modifiable_chromosome = const_cast<std::vector<double>&>(chromosome);
    
    // Pegamos os alelos (valores) originais gerados pelo BRKGA e os ordenamos
    std::vector<double> sorted_genes = modifiable_chromosome;
    std::sort(sorted_genes.begin(), sorted_genes.end());
    
    // Agora, garantimos que o cromossomo reflita exatamente a nova ordem gerada pela busca local.
    // O padrão que terminou na posição 'i' na partial_solution ganha o i-ésimo menor gene.
    for (size_t i = 0; i < partial_solution.size(); ++i) {
        int final_pattern = partial_solution[i];
        modifiable_chromosome[final_pattern] = sorted_genes[i];
    }
    // -------------------------------------------------------------

    // 4. Avaliação do Fitness (MMOSP)
    std::vector<int> mosp_k_list = get_open_stacks_per_pattern(partial_solution);
    int max_mosp = 0;
    double sum_mosp = 0.0;
    
    for (int val : mosp_k_list) {
        if (val > max_mosp) max_mosp = val;
        sum_mosp += val;
    }

    if (max_mosp == 0) return 0.0;

    // Fórmula MMOSP do artigo: MOSP + (soma(MOSP_k) / (m * MOSP))
    double mmosp = max_mosp + (sum_mosp / ((double)m * max_mosp));

    if (solution != nullptr) {
        *solution = partial_solution;
    }

    return mmosp; 
}