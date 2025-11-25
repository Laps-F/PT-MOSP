# MOSP — Minimization of Open Stacks Problem (Research Repository)

This repository contains the source code, datasets, and scripts used in a scientific study on the Minimization of Open Stacks Problem (MOSP). It implements construction heuristics and local search neighborhoods, includes multiple benchmark datasets, and contains experimental/tuning scripts to reproduce results.

Links to important files and symbols
- Build & run: [MOSP/Makefile](MOSP/Makefile)
- Main executable/entry-point: [MOSP/mainMOSP.cpp](MOSP/mainMOSP.cpp)
- Core solver and neighborhoods: [MOSP/MOSP.cpp](MOSP/MOSP.cpp) — [`MOSP::construction`](MOSP/MOSP.cpp), [`MOSP::neighbor`](MOSP/MOSP.cpp), [`MOSP::neighborSwap`](MOSP/MOSP.cpp), [`MOSP::neighbor2Opt`](MOSP/MOSP.cpp), [`MOSP::neighborInsertion`](MOSP/MOSP.cpp), [`MOSP::evaluate`](MOSP/MOSP.cpp)
- Solver interface and types: [MOSP/MOSP.h](MOSP/MOSP.h)
- PTAPI integration: [PTAPI-main/README.md](PTAPI-main/README.md)
- Dataset descriptions:
  - [Instances/Frinhani/Dataset_Description - Frinhani, Carvalho & Soma.txt](Instances/Frinhani/Dataset_Description - Frinhani, Carvalho & Soma.txt)
  - [Instances/Chu_Stuckey/Dataset_Description - Chu_Stuckey.txt](Instances/Chu_Stuckey/Dataset_Description - Chu_Stuckey.txt)
  - [Instances/Challenge/Dataset_Description - Challenge.txt](Instances/Challenge/Dataset_Description - Challenge.txt)
  - Instances/Cavalho_Soma - Does not have a description file here.
  - Instances/SCOOP - Does not have a description file here.
- Tables:
  - [MOSP/Results-tables/Final_Results.xlsx](MOSP/Results-tables/Final_Results.xlsx)
  - [MOSP/Results-tables/Results_CarvalhoSoma.xlsx](MOSP/Results-tables/Results_CarvalhoSoma.xlsx)
  - [MOSP/Results-tables/Results_Challenge.xlsx](MOSP/Results-tables/Results_Challenge.xlsx)
  - [MOSP/Results-tables/Results_ChuStuckey.xlsx](MOSP/Results-tables/Results_ChuStuckey.xlsx)
  - [MOSP/Results-tables/Results_FaggioliBentivoglio.xlsx](MOSP/Results-tables/Results_FaggioliBentivoglio.xlsx)
  - [MOSP/Results-tables/Results_Frinhani.xlsx](MOSP/Results-tables/Results_Frinhani.xlsx)
  - [MOSP/Results-tables/Results_SCOOP.xlsx](MOSP/Results-tables/Results_SCOOP.xlsx)
- Graphs:
  - [MOSP/Results-graphs/graph_Carvalho_Soma.png](MOSP/Results-graphs/graph_Carvalho_Soma.png.xlsx)
  - [MOSP/Results-graphs/graph_Challenge.png](MOSP/Results-graphs/graph_Challenge.png.xlsx)
  - [MOSP/Results-graphs/graph_Chu_Stuckey.png](MOSP/Results-graphs/graph_Chu_Stuckey.png.xlsx)
  - [MOSP/Results-graphs/graph_Faggioli_Bentivoglio.png](MOSP/Results-graphs/graph_Faggioli_Bentivoglio.png.xlsx)
  - [MOSP/Results-graphs/graph_SCOOP.png](MOSP/Results-graphs/graph_SCOOP.png.xlsx)
- Tuning and experiment files: [tuning/configurations.txt](tuning/configurations.txt), [tuning/instances-list.txt](tuning/instances-list.txt), [tuning/out_article.txt](tuning/out_article.txt)

Repository layout
- MOSP/ — main solver sources, utilities, Makefile and scripts
  - [MOSP/Makefile](MOSP/Makefile) — build/run/test helper
  - [MOSP/MOSP.cpp](MOSP/MOSP.cpp) and [MOSP/MOSP.h](MOSP/MOSP.h)
  - [MOSP/MCNH/](MOSP/MCNH/) — helper functions and heuristics (e.g. [LinkedList.h](MOSP/MCNH/LinkedList.h))
- Instances/ — instance datasets (Chu & Stuckey, Frinhani et al., Challenge, etc.)
- PTAPI-main/ — problem template and interfaces
- tuning/ — scripts and configuration files for parameter tuning and experiment reproduction
- Results/ — runtime results produced by the runs (created by the Makefile)

Tables and Figures

- Where to find raw outputs
  - MOSP/Results/: raw per-run text outputs (grouped by dataset/source).
  - MOSP/Results-tables/ and MOSP/stats/: scripts/csvs for aggregated tables and summary statistics.
  - MOSP/Results-graphs/: plotting scripts and generated images (graph.py).

- Table types and scripts
  - Per-run raw results: one file per run; include runtime, objective, seed, instance name and parameters.
  - Aggregated tables: use MOSP/Results-tables/gerar_csv.py to convert per-run outputs into CSVs for statistical processing.
  - Statistics script: MOSP/stats/statistics.py reads aggregated CSVs and computes summary statistics for PieceRank x PT-MOSP.


Requirements and dependencies
- Linux / macOS environment with:
  - g++ supporting C++20 (or clang++ with C++20)
  - make
  - Python 3 (for tuning/utility scripts)
  - pthreads support (Makefile uses -lpthread) and OpenMP (via `#include <omp.h>`)
- Build flags and typical params in [MOSP/Makefile](MOSP/Makefile). Example default parameters in Makefile: `PARAMS = --TEMP_INIT 0.01 --TEMP_FIM 10 --N_REPLICAS 11 --MCL 600 --PTL 2000 --TEMP_DIST 2 --MOV_TYPE 2 --TYPE_UPDATE 1 --TEMP_UPDATE 3`.

Quick start: build and run
1. Build
- cd MOSP
- make

2. Run a single instance (example)
- make run-one FILE="../Instances/Frinhani/Random-m-n-pbp-i.txt"
  - The Makefile will call the built executable and save output under `Results/<creator>/`.
  - You can pass other runtime parameters using `PARAMS` in the Makefile or CLI flags.

3. Run all instances for a folder (parallel experiments)
- cd MOSP
- make run-folder FOLDER=Frinhani

4. Run all instances across the whole Instances directory:
- cd MOSP
- make run-all

Notes on parameterization & experimentation
- The Makefile in [MOSP/Makefile](MOSP/Makefile) exposes `PARAMS`, `INST_DIR`, `RESULT_DIR`, and targets `run-one`, `run-folder`, `run-all` to automate experiments and store outputs under `Results/`.
- Check and edit `PARAMS` in [MOSP/Makefile](MOSP/Makefile) to change algorithm parameters (temperature, replicas, movement types, etc.). The program accepts CLI flags for these parameters.

Implementation overview
- The `MOSP` class implements constructors, evaluators, and three neighborhood operators:
  - Swap: [`MOSP::neighborSwap`](MOSP/MOSP.cpp)
  - 2-opt: [`MOSP::neighbor2Opt`](MOSP/MOSP.cpp)
  - Random insertion: [`MOSP::neighborInsertion`](MOSP/MOSP.cpp)
- Initialization uses either:
  - Construction heuristic via HNCM (`MOSP::construction` calls `mainMethod` in `MCNH/HNCM.h`), or
  - Random shuffling by sequence/pattern form (see [`MOSP::construction`](MOSP/MOSP.cpp) and [`MOSP::MOSP`](MOSP/MOSP.cpp)).  
- The `LinkedList` helper in `MOSP/MCNH/LinkedList.h` provides methods to manage pattern lists (add, count, finalize, transfer, and order insertion via `insereOrdenado`).

Reproducing results & tuning
- Use `tuning/` scripts and the Makefile targets to reproduce experiments and collect statistics.
- CSV results and temporary files are organized under `MOSP/Results/`, `MOSP/planilhas-resultados/`, `MOSP/Resultados-Testes/`, and `MOSP/stats/`.

Datasets and citation
- Included datasets and descriptions can be found under [Instances/](Instances/).
- Cite the corresponding dataset authors when using or publishing benchmarking results (see dataset description files linked above).

Authors
- Mauro Lúcio A. P. Santos Filho (Corresponding author) — mauro.paulino@aluno.ufop.edu.br  
- Marco Antonio M. Carvalho — mamc@ufop.edu.br

Affiliation: Federal University of Ouro Preto (UFOP), Department of Computing, Ouro Preto, Brazil