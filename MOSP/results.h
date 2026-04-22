#ifndef RESULTS_H
#define RESULTS_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>

struct SolResult {
    double evalSol;
    int maxNumberPiecesPerPatern;
    std::vector<int> sol;
    int construcTime = 0; // Para compatibilidade
};

void saveResults(const std::string& fn, const SolResult& sol, int elapsed, int trocas, const std::string& dir, int instance, int replicas) {
    std::string basename = fn.substr(fn.find_last_of("/\\") + 1);
    std::string name_no_ext = basename.substr(0, basename.find_last_of('.'));

    std::vector<std::string> parts;
    std::stringstream ss(name_no_ext);
    std::string token;
    while (std::getline(ss, token, '-')) parts.push_back(token);

    std::string dimensao;
    std::ifstream file(fn);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << fn << std::endl;
        return;
    }

    std::string line;
    if (std::getline(file, line)) {
        dimensao = line;
    } else {
        dimensao = "?";
    }

    std::string out_name = dir + "/" + name_no_ext + "(" + std::to_string(instance) + ")_res.txt";

    std::ofstream ofs(out_name);
    if (!ofs.is_open()) {
        std::cerr << "Erro ao abrir arquivo de resultados: " << out_name << std::endl;
        return;
    }

    ofs << dimensao << '\n'
        << elapsed << '\n'
        << sol.evalSol << '\n'
        << trocas << '\n'
        << sol.maxNumberPiecesPerPatern << '\n';

    for (int i : sol.sol) {
        ofs << i << ' ';
    }
    ofs.close();
}

#endif // RESULTS_H