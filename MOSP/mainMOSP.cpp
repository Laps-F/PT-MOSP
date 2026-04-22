#include <cstdlib>
#include <filesystem>
#include "../PTAPI-main/include/ExecTime.h"
#include "./MOSP.h"
#include "../PTAPI-main/include/PT.h"
#include "results.h"

using namespace std;

int main(int argc, char* argv[])
{
	//varibles
    int exec = 0;
	float tempIni = 0.01;
	float tempfim = 10;
	int tempN = 11;
	int MCL = 600;
	int PTL = 2000;	
	int tempUp = 400;
	int tempD = 2;
	int uType = 3;
    int mType = 2;
    int read = 0;
    int sequence = 1;
	int thN = thread::hardware_concurrency();	
    string outDir = "resultados";

	// int thN = 11;
	vector<string> arguments(argv + 1, argv + argc);	
	
	// Instance file name
	string fn = arguments[0];
	
	// Read arguments
	for(unsigned int i=1; i<arguments.size(); i+=2)
	{
		            
        if(arguments[i]== "--TEMP_FIM")
            tempfim = stof(arguments[i+1]);
        else if(arguments[i]== "--TEMP_INIT")
            tempIni = stof(arguments[i+1]);
        else if(arguments[i]== "--N_REPLICAS")
            tempN = stoi(arguments[i+1]);
        else if(arguments[i]== "--MCL")
            MCL  = stoi(arguments[i+1]);
        else if(arguments[i]== "--PTL")
            PTL = stoi(arguments[i+1]);
        else if(arguments[i]== "--TEMP_DIST")
            tempD = stoi(arguments[i+1]);
        else if(arguments[i]== "--MOV_TYPE")
            mType = stoi(arguments[i+1]);
        else if(arguments[i]== "--TYPE_UPDATE")
            uType = stoi(arguments[i+1]);
        else if(arguments[i]== "--TEMP_UPDATE"){
            int proportion = stoi(arguments[i+1]);
            tempUp = PTL / proportion;
        }
        else if(arguments[i]== "--THREAD_USED")
            thN = stoi(arguments[i+1]);
        else if(arguments[i]== "--SEQUENCE")
            sequence = stoi(arguments[i+1]);
        else if(arguments[i]== "--INST")
            exec = stoi(arguments[i+1]);
        else if(arguments[i]== "--OUTDIR")
            outDir = arguments[i+1];
    }

    // Create PT subdirectory
    std::filesystem::create_directories(outDir + "/PT");
    outDir += "/PT";

    // if(outDir == "Results/Frinhani" || outDir == "Results/Frinhani/temp") read = 1;

    MOSP* prob = new MOSP(fn, read, mType, sequence);
    
    // Create and start PT 

    PT<solMOSP> algo(tempIni, tempfim, tempN, MCL, PTL, 0, tempD, uType, tempUp);

    ExecTime et;
    ResultPT<solMOSP> resultado = algo.start(thN, prob);
    solMOSP sol = resultado.best;             // pega a melhor solução
    int elapsed = et.getTimeMs();
    int trocas = resultado.numTrocas;   
    int construcTime = sol.construcTime;
    int execTime_ms = elapsed + construcTime;
    float execTime_s = execTime_ms / 1000;

    SolResult res;
    res.evalSol = sol.evalSol;
    res.maxNumberPiecesPerPatern = sol.maxNumberPiecesPerPatern;
    res.sol = sol.sol;
    res.construcTime = construcTime;

    saveResults(fn, res, execTime_ms, sol.ptl, outDir, exec, tempN);

    // cout<<construcTime<<endl;
    // cout<<"Trocas: "<<sol.ptl<<endl; 
    // float resultForIrace = sol.evalSol + execTime_s/10000;
    // cout<<resultForIrace<<endl;

	return 0;
}

