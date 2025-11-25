#include "MOSP.h"
#include "MCNH/HNCM.h"

using namespace std;

MOSP::MOSP(std::string filename, int readForm, int neighborStrategy, int inSequenceForm){
    neighborStrt = neighborStrategy;
	fn = filename;
    sequenceForm = inSequenceForm;
    std::ifstream file(filename);
	std::string line; 
	
    if(!file.is_open()){
        std::cout << "Could not open file! \n";
        return;
    }	

    file >> numberPatterns >> numberPieces;
    // file >> numberPieces >> numberPatterns;


    patternPieces.resize(numberPatterns);
    stackSizeEvaluation.resize(numberPieces, 0);
    maxNumberPiecesPerPatern = 0;
    constructSolutionTime = 0;

    if(readForm){ //leitura arquivos do rafael
        for (int j = 0; j < numberPatterns; j++) {
            for (int i = 0; i < numberPieces; i++) {
                int value;
                file >> value;
                if (value == 1) {
                    patternPieces[j].push_back(i); // armazena o índice da peça (j) associada ao padrão (i)
                    stackSizeEvaluation[i]++; //armazenam a quantidade de vezes que cada peça aparece nos padrões
                }
            }
        }
    } else { // leitura dos outros arquivos
        for (int i = 0; i < numberPieces; i++) {
            for (int j = 0; j < numberPatterns; j++) {
                int value;
                file >> value;
                if (value == 1) {
                    patternPieces[j].push_back(i); // armazena o índice da peça (j) associada ao padrão (i)
                    stackSizeEvaluation[i]++; //armazenam a quantidade de vezes que cada peça aparece nos padrões
                }
            }
        }
    }

    for (int j = 0; j < numberPatterns; j++)
        if(patternPieces[j].size() > maxNumberPiecesPerPatern){
            maxNumberPiecesPerPatern = patternPieces[j].size();
        }

    patternBitsets.resize(numberPatterns);

    if(sequenceForm){
        for (int i = 0; i < numberPatterns; i++) {
            for (int p : patternPieces[i]) {
                patternBitsets[i].set(p);
            }
        }

        patternsPorPeca.resize(numberPieces);
        for (int pattern = 0; pattern < numberPatterns; ++pattern) {
            for (int piece = 0; piece < numberPieces; ++piece) {
                if (patternBitsets[pattern].test(piece)) {
                    patternsPorPeca[piece].push_back(pattern);
                }
            }
        }
    }

    file.close();
}

MOSP::~MOSP(){	
}

solMOSP MOSP::construction() {
    int idx = constructionCallIndex.fetch_add(1);

    solMOSP ss;

    if(idx == 0 || idx == 2){
         int* solution = nullptr;
        int resultHNCM, solutionSize = 0;	
        double timeHNCM;

        char* filename = new char[fn.size() + 1];
        strcpy(filename, fn.c_str());

        if(sequenceForm){
            mainMethod(&resultHNCM, &timeHNCM, filename, &solution, &solutionSize, 1); //peças
        } else {
            mainMethod(&resultHNCM, &timeHNCM, filename, &solution, &solutionSize, 0); //padroes
        }

        // std::cout << "Debug: mainMethod retornou solutionSize = " << solutionSize << std::endl;
        // for(int i = 0; i < solutionSize; i++) {
        //     std::cout<<solution[i]<<", ";
        // }std::cout<<"."<<std::endl;

        constructSolutionTime += timeHNCM;

        for(int i = 0; i < solutionSize; i++) {
            ss.sol.push_back(solution[i]);
        }

        delete[] solution;
    } else {
        std::random_device rnd_device;
        std::mt19937 mersenne_engine {rnd_device()};

        if(sequenceForm){
            for (int i = 0; i < numberPieces; i++) {
                ss.sol.push_back(i);
            }
        } else {
            for (int i = 0; i < numberPatterns; i++) {
                ss.sol.push_back(i);
            }
        }

        std::shuffle(begin(ss.sol), end(ss.sol), mersenne_engine);
    }

    ss.evalSol = evaluate(ss);
    ss.maxNumberPiecesPerPatern = maxNumberPiecesPerPatern;
	ss.Nup = false;
	ss.Ndown = false;

	return ss;
}

solMOSP MOSP::neighbor(solMOSP sol){
    switch (neighborStrt) {
        case 0:
            return neighborSwap(sol);
        case 1:
            return neighbor2Opt(sol);
        case 2:
            return neighborInsertion(sol);
        default:
            return sol;  // Retorna a mesma solução se tipo inválido
    }
}

// 2-opt neighbor moviment
solMOSP MOSP::neighbor2Opt(solMOSP sol){
	solMOSP s;
    s.sol = sol.sol; 
    s.maxNumberPiecesPerPatern = sol.maxNumberPiecesPerPatern;

    std::random_device rnd_device;
    std::mt19937 mersenne_engine {rnd_device()};
    if(sequenceForm){
        std::uniform_int_distribution<int> distPattern(0, numberPieces - 1);
    
        int piece1 = 0, piece2 = 0;
        
        do {
            piece1 = distPattern(mersenne_engine);
            piece2 = distPattern(mersenne_engine);
        } while (piece1 == piece2); 
        
        if(piece1 > piece2) std::swap(piece1, piece2);
    
        while(piece1 < piece2) {
            std::swap(s.sol[piece1], s.sol[piece2]);
            piece1++;
            piece2--;
        }
    }else {
        std::uniform_int_distribution<int> distPattern(0, numberPatterns - 1);
    
        int pattern1 = 0, pattern2 = 0;
        
        do {
            pattern1 = distPattern(mersenne_engine);
            pattern2 = distPattern(mersenne_engine);
        } while (pattern1 == pattern2); 
        
        if(pattern1 > pattern2) std::swap(pattern1, pattern2);
    
        while(pattern1 < pattern2) {
            std::swap(s.sol[pattern1], s.sol[pattern2]);
            pattern1++;
            pattern2--;
        }

    }

    s.Nup = sol.Nup;
    s.Ndown = sol.Ndown;

    return s;
}

// swap neighbor moviment
solMOSP MOSP::neighborSwap(solMOSP sol) {
    solMOSP s;
    s.sol = sol.sol;
    s.maxNumberPiecesPerPatern = sol.maxNumberPiecesPerPatern;

    std::random_device rnd_device;
    std::mt19937 mersenne_engine {rnd_device()};

    if(sequenceForm){
        std::uniform_int_distribution<int> distPattern(0, numberPieces - 1);
    
        int piece1 = 0, piece2 = 0;
    
        do {
            piece1 = distPattern(mersenne_engine);
            piece2 = distPattern(mersenne_engine);
        } while (piece1 == piece2);
    
        std::swap(s.sol[piece1], s.sol[piece2]);
    }else {
        std::uniform_int_distribution<int> distPattern(0, numberPatterns - 1);
    
        int pattern1 = 0, pattern2 = 0;
    
        do {
            pattern1 = distPattern(mersenne_engine);
            pattern2 = distPattern(mersenne_engine);
        } while (pattern1 == pattern2);
    
        std::swap(s.sol[pattern1], s.sol[pattern2]);
    }

    s.Nup = sol.Nup;
    s.Ndown = sol.Ndown;

    return s;
}

// random insertion neighbor movement
solMOSP MOSP::neighborInsertion(solMOSP sol) {
    solMOSP s;
    s.sol = sol.sol;
    s.maxNumberPiecesPerPatern = sol.maxNumberPiecesPerPatern;

    std::random_device rnd_device;
    std::mt19937 mersenne_engine {rnd_device()};
    
    if(sequenceForm){
        std::uniform_int_distribution<int> distPattern(0, numberPieces - 1);
    
        int pieceFrom = 0, pieceTo = 0;
    
        do {
            pieceFrom = distPattern(mersenne_engine);
            pieceTo = distPattern(mersenne_engine);
        } while (pieceFrom == pieceTo);
    
        int element = s.sol[pieceFrom];
        s.sol.erase(s.sol.begin() + pieceFrom);
    
        if (pieceTo > pieceFrom) {
            pieceTo--;  // ajuste porque o vetor ficou menor após remoção
        }
    
        s.sol.insert(s.sol.begin() + pieceTo, element);
    } else {
        std::uniform_int_distribution<int> distPattern(0, numberPatterns - 1);
    
        int patternFrom = 0, patternTo = 0;
    
        do {
            patternFrom = distPattern(mersenne_engine);
            patternTo = distPattern(mersenne_engine);
        } while (patternFrom == patternTo);
    
        int element = s.sol[patternFrom];
        s.sol.erase(s.sol.begin() + patternFrom);
    
        if (patternTo > patternFrom) {
            patternTo--;  // ajuste porque o vetor ficou menor após remoção
        }
    
        s.sol.insert(s.sol.begin() + patternTo, element);
    }

    s.Nup = sol.Nup;
    s.Ndown = sol.Ndown;

    return s;
}

double MOSP::evaluate(solMOSP& sol) {
    
    sol.construcTime = constructSolutionTime;
    
    std::vector<int> patternSequence;
    
    if (sequenceForm) {
        std::bitset<1000> piecesRead;
        std::bitset<1000> patternUsed;

        for (int piece : sol.sol) {
            piecesRead.set(piece);
            const auto& candidatos = patternsPorPeca[piece];
            auto notRead = ~piecesRead;
            

            for (int pattern : candidatos) {
                if (!patternUsed.test(pattern) &&
                    (patternBitsets[pattern] & notRead).none()) {
                    patternSequence.push_back(pattern);
                    patternUsed.set(pattern);
                }
            }
        }
    } else {
        patternSequence = sol.sol;
    }

    const int OPEN = 1, CLOSED = 0;

    std::vector<int> stack(numberPieces, CLOSED);
    std::vector<int> vet = stackSizeEvaluation;

    int openStacks = 0;
    int closedStacks = 0;
    int maxOpenStacks = -1;

    for (int index : patternSequence) {
        for (int peca : patternPieces[index]) {
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
