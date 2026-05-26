#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <bitset>

using namespace std;
namespace fs = std::filesystem;

int numberPatterns, numberPieces;
vector<vector<int>> patternPieces;
vector<int> ordem;
vector<bitset<1000>> patternBitsets;

// Lê a entrada no formato invertido: peças x padrões
bool lerMatriz(const string& nomeArquivo) {
    ifstream file(nomeArquivo);
    if (!file) {
        cerr << "Erro ao abrir arquivo de entrada: " << nomeArquivo << endl;
        return false;
    }
    
    patternPieces.clear();
    ordem.clear();

    file >> numberPatterns >> numberPieces;

    patternPieces.resize(numberPatterns);

    for (int i = 0; i < numberPieces; i++) {
    for (int j = 0; j < numberPatterns; j++) {
        int value;
            file >> value;
            if (value == 1) {
                patternPieces[j].push_back(i); // armazena o índice da peça (j) associada ao padrão (i)
            }
        }
    }

    patternBitsets.resize(numberPatterns);

    // Pré-processa os padrões para bitsets
    for (int i = 0; i < numberPatterns; i++) {
        for (int p : patternPieces[i]) {
            patternBitsets[i].set(p);
        }
    }

    file.close();
    return true;
}

// Lê o arquivo de solução
bool lerSolucao(const string& nomeArquivo, int& valorSolu) {
    ifstream fin(nomeArquivo);
    if (!fin) {
        cerr << "Erro ao abrir arquivo de solução: " << nomeArquivo << endl;
        return false;
    }

    int mm, nn;
    double tempo;
    fin >> mm >> nn;

    if (mm != numberPatterns || nn != numberPieces) {
        cerr << "Erro: dimensões da solução não batem com a entrada." << endl;
        return false;
    }

    fin >> tempo;
    fin >> valorSolu;

    fin.ignore(numeric_limits<streamsize>::max(), '\n');

    string ignoredLine1, ignoredLine2;
    getline(fin, ignoredLine1);
    getline(fin, ignoredLine2);


    ordem.resize(numberPatterns);
    for (int i = 0; i < numberPatterns; ++i) {
        if (!(fin >> ordem[i])) {
            cerr << "Erro ao ler a ordem dos padrões." << endl;
            return false;
        }
    }

    fin.close();
    return true;
}

// Calcula o número máximo de pilhas abertas seguindo a ordem da solução
int calcularMaxPilhasAbertas() {
    bitset<1000> piecesRead;
    vector<bool> patternUsed(numberPatterns, false);
    vector<int> patternSequence;

    for (int piece : ordem) {
        piecesRead.set(piece);

        for (int pattern = 0; pattern < numberPatterns; pattern++) {
            if (patternUsed[pattern]) continue;

            if ((patternBitsets[pattern] & piecesRead) == patternBitsets[pattern]) {
                patternSequence.push_back(pattern);
                patternUsed[pattern] = true;
            }
        }
    }

    vector<int> ultimaOcorrencia(numberPieces, -1);
    vector<int> ativa(numberPieces, 0); // 1 se a peça está ativa (pilha aberta)

    for (int i = 0; i < numberPatterns; ++i) {
        int padrao = patternSequence[i];
        for (int peca : patternPieces[padrao]) { 
            ultimaOcorrencia[peca] = padrao;
        }
    }

    int pilhasAbertas = 0;
    int maxPilhas = 0;

    for (int i = 0; i < numberPatterns; ++i) {
        int padrao = patternSequence[i];

        // Abrir novas pilhas
        for (int peca : patternPieces[padrao]) {
            if (ativa[peca] == 0) {
                ativa[peca] = 1;
                pilhasAbertas++;
            }
        }

        maxPilhas = max(maxPilhas, pilhasAbertas);

        // Fechar pilhas cuja última ocorrência é agora
        for (int peca : patternPieces[padrao]) {
            if (ativa[peca] == 1 && ultimaOcorrencia[peca] == padrao) {
                ativa[peca] = 0;
                pilhasAbertas--;
            }
        }

    }

    return maxPilhas;
}

void verificarArquivo(const string& caminhoEntrada, const string& caminhoSolucao, ofstream& outValidos, ofstream& outInvalidos, bool flag) {
    string nomeArquivo = fs::path(caminhoEntrada).filename().string();
    if (flag)
        cout << "Verificando: " << nomeArquivo << endl;

    if (!lerMatriz(caminhoEntrada)) {
        cerr << "❌ Erro ao ler entrada.\n" << endl;
        return;
    }

    int valorSolu;
    if (!lerSolucao(caminhoSolucao, valorSolu)) {
        cerr << "❌ Erro ao ler solução.\n" << endl;
        return;
    }

    int valorCalculado = calcularMaxPilhasAbertas();
    
    if (flag){
        cout << "  Valor declarado na solução: " << valorSolu << endl;
        cout << "  Valor calculado pelo verificador: " << valorCalculado << endl;
    }
    if (valorCalculado == valorSolu) {
        if (flag)
            cout << "  ✅ Solução VÁLIDA!" << endl;
        outValidos << nomeArquivo << endl;
    } else {
        if (flag)
            cout << "  ❌ Solução INVÁLIDA!" << endl;
        outInvalidos << nomeArquivo << " (esperado: " << valorSolu << ", calculado: " << valorCalculado << ")" << endl;
    }
}

int main(int argc, char* argv[]) {
    ofstream outValidos("validos.txt");
    ofstream outInvalidos("invalidos.txt");

    if (!outValidos || !outInvalidos) {
        cerr << "Erro ao criar arquivos de saída!" << endl;
        return 1;
    }
// resultados/Random-400-400-2-1_res.txt
// ../Frinhani/Instances/Random-400-400-2-1.txt
    if (argc == 3) {
        string entrada = argv[1];
        string solucao = argv[2];
        verificarArquivo(entrada, solucao, outValidos, outInvalidos, true);
    } else {
        string pastaEntrada = "../../Instances/Frinhani";
        string pastaSolucao = "../Results/Frinhani";

        
        for (const auto& entrada : fs::directory_iterator(pastaEntrada)) {
            if (!entrada.is_regular_file()) continue;

            string nomeBase = entrada.path().stem().string();
            string caminhoEntrada = entrada.path().string();
            string caminhoSolucao = pastaSolucao + "/" + nomeBase + "_res.txt";
            
            verificarArquivo(caminhoEntrada, caminhoSolucao, outValidos, outInvalidos, false);
        }
    }

    outValidos.close();
    outInvalidos.close();
    return 0;
}