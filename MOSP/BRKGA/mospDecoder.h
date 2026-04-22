#ifndef MOSP_DECODER_H
#define MOSP_DECODER_H

#include <vector>

class MospDecoder {
private:
    int m; // Número de padrões
    int n; // Número de itens
    std::vector<std::vector<bool>> matrix; 

    // Lista de itens contidos em cada padrão para acesso O(1)
    std::vector<std::vector<int>> items_in_pattern;

    // Métodos auxiliares para calcular o fitness e a busca local
    int calculate_mosp(const std::vector<int>& partial_sequence) const;
    std::vector<int> get_open_stacks_per_pattern(const std::vector<int>& sequence) const;
    int calculate_mosp_with_insert(const std::vector<int>& partial_solution,
                                   const std::vector<int>& first_occ,
                                   const std::vector<int>& last_occ,
                                   int pattern,
                                   size_t insert_pos,
                                   std::vector<int>& events) const;

public:
    MospDecoder(int num_patterns, int num_items, const std::vector<std::vector<bool>>& init_matrix);
    ~MospDecoder();

    double decode(const std::vector<double>& chromosome, std::vector<int>* solution = nullptr) const;
};

#endif