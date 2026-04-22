#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <limits>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include "./BRKGA/BRKGA.h"
#include "./BRKGA/MTRand.h"
#include "./BRKGA/mospDecoder.h"
#include "results.h"

using namespace std;

// Função para ler a matriz a partir do arquivo
bool readInstance(const std::string& filename, int& num_patterns, int& num_items, std::vector<std::vector<bool>>& matrix) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo: " << filename << std::endl;
        return false;
    }

    file >> num_items >> num_patterns;

    matrix.assign(num_items, std::vector<bool>(num_patterns, false));

    int val;
    for (int i = 0; i < num_items; ++i) {
        for (int j = 0; j < num_patterns; ++j) {
            file >> val;
            matrix[i][j] = (val == 1);
        }
    }

    file.close();
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: ./mainBRKGA <caminho_para_instancia> [--INST <instance>] [--OUTDIR <outdir>]" << std::endl;
        return 1;
    }

    int exec = 0;
    std::string outDir = "resultados";
    unsigned max_generations = 100;
    unsigned print_every = 1;
    unsigned MAX_THREADS = thread::hardware_concurrency();
    bool verbose = false;

    std::vector<std::string> arguments(argv + 1, argv + argc);

    // Instance file name
    std::string fn = arguments[0];

    // Read arguments
    for (unsigned int i = 1; i < arguments.size(); i += 2) {
        if (arguments[i] == "--INST")
            exec = std::stoi(arguments[i + 1]);
        else if (arguments[i] == "--OUTDIR")
            outDir = arguments[i + 1];
        else if (arguments[i] == "--GENS")
            max_generations = std::stoul(arguments[i + 1]);
        else if (arguments[i] == "--PRINT_EVERY")
            print_every = std::stoul(arguments[i + 1]);
        else if (arguments[i] == "--THREAD_USED")
            MAX_THREADS = std::stoul(arguments[i + 1]);
        else if (arguments[i] == "--VERBOSE")
            verbose = (std::stoi(arguments[i + 1]) != 0);
    }

    // Create BRKGA subdirectory
    std::filesystem::create_directories(outDir + "/BRKGA");
    outDir += "/BRKGA";

    int m, n;
    std::vector<std::vector<bool>> matriz;

    if (!readInstance(fn, m, n, matriz)) {
        return 1;
    }

    MospDecoder decoder(m, n, matriz);

    // Parâmetros do BRKGA
    unsigned p = 10 * m;      // tamanho da população

    // Aplica a regra rigorosa da Tabela 5: p_e = min(0.25 * p, 50)
    double absolute_pe = std::min(0.25 * p, 50.0);
    double absolute_pm = 0.20 * p;

    // A brkgaAPI EXIGE frações, então convertemos o valor absoluto de volta para percentual:
    double pe_fraction = absolute_pe / p;
    double pm_fraction = absolute_pm / p;

    double rhoe = 0.70;       // probabilidade de herdar da elite
    unsigned K = 1;           // número de populações

    MTRand rng(12345 + (exec * 9999));

    if (verbose) {
        std::cout << "BRKGA START: instance='" << fn << "' m=" << m << " n=" << n
                  << " pop=" << p << " pe=" << absolute_pe << " pm=" << absolute_pm
                  << " gens=" << max_generations << " threads=" << MAX_THREADS << std::endl;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Passamos as frações calculadas (pe_fraction e pm_fraction) para a API
    BRKGA<MospDecoder, MTRand> algorithm(m, p, pe_fraction, pm_fraction, rhoe, decoder, rng, K, MAX_THREADS);

    // Processo evolutivo
    for (unsigned gen = 0; gen < max_generations; ++gen) {
        auto gen_start = std::chrono::high_resolution_clock::now();
        algorithm.evolve();

        if (verbose && ((gen + 1) % print_every == 0 || gen + 1 == max_generations)) {
            double current_best = algorithm.getBestFitness();
            auto gen_end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> gen_elapsed = gen_end - gen_start;
            // std::cout << "GEN " << (gen + 1) << "/" << max_generations
            //           << " | best=" << current_best
            //           << " | time=" << gen_elapsed.count() << "s"
            //           << std::endl;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

    // Extrai o resultado
    std::vector<int> best_sol;
    double best_fitness = decoder.decode(algorithm.getBestChromosome(), &best_sol);

    SolResult res;
    res.evalSol = static_cast<int>(best_fitness);
    res.maxNumberPiecesPerPatern = 0; // Não calculado, talvez chamar novamente
    res.sol = best_sol;

    int trocas = 0; // BRKGA não tem trocas como PT

    saveResults(fn, res, static_cast<int>(elapsed.count()), trocas, outDir, exec, 1);

    return 0;
}