# PT-MOSP — Parallel Tempering for the Minimization of Open Stacks Problem

This repository contains the source code, benchmark instances, raw results and analysis
scripts used in the study *Optimizing Cutting Sequences through Parallel Tempering: The
Minimization of Open Stacks Problem*.

It includes the proposed PT-MOSP solver, the reimplemented baselines used in the direct
comparisons (BRKGA, SA and NSD), the exact and constraint-programming models used for
scalability screening (ILP, CP and the branch-and-bound of Chu and Stuckey), the instance
sets, the parameter tuning files and the scripts that generate the tables and figures of
the paper.

## Citation

Santos Filho, M.L.A.P. and Carvalho, M.A.M. Optimizing Cutting Sequences through Parallel
Tempering: The Minimization of Open Stacks Problem. *International Transactions in
Operational Research* (in press).

## License

The code and the experimental data in this repository are released under the
Creative Commons Attribution-NonCommercial 4.0 International license (CC BY-NC 4.0).
See the `LICENSE` file. Instance sets created by other authors remain under the terms of
their original publications; cite the corresponding dataset authors when reusing them.

## Repository layout

```
MOSP/
  MOSP.cpp, MOSP.h        MOSP model: instance reading, decoding, evaluation, neighborhoods
  mainMOSP.cpp            PT-MOSP entry point
  Makefile                build and batch-execution targets
  results.h               output formatting for the runs
  MCNH/                   HNCM construction heuristic (HNCM.h) and list helper (LinkedList.h)
  BRKGA/                  biased random-key genetic algorithm baseline
  SA/                     simulated annealing baseline
  SND/                    nested steepest descent baseline
  CHU_STUCKEY/            branch-and-bound of Chu and Stuckey
  ILP/ilp.py              integer linear programming model (Gurobi)
  CP/cp.py                constraint programming model (OR-Tools CP-SAT)
  verify/                 solution checker for the sequences produced by the solvers
  Results-PT/, Results2/  raw per-run outputs, grouped by instance set
  Results-tables/         aggregated spreadsheets and conversion scripts
  Results-graphs/         graph.py and the generated figures
  stats/                  statistical analysis scripts and the CSVs they produce
Instances/                benchmark instances, one folder per source
PTAPI-main/               parallel tempering framework (headers, examples, sample instances)
tuning/                   irace setup and log of the calibration reported in the paper
```

## Instances

The classical benchmark (720 instances) is split by source:

| Folder | Instances | Source |
| --- | --- | --- |
| `Instances/Carvalho_Soma` | 150 | Carvalho and Soma |
| `Instances/Challenge` | 46 | Constraint Modelling Challenge 2005 |
| `Instances/Chu_Stuckey` | 200 | Chu and Stuckey |
| `Instances/Faggioli_Bentivoglio` | 300 | Faggioli and Bentivoglio |
| `Instances/SCOOP` | 24 | SCOOP project |

The large-scale benchmark of Frinhani, Carvalho and Soma (610 instances, from 400x400 to
1000x1000) is in `Instances/Frinhani`. The folders `Frinhani_All`, `Frinhani_BRKGA` and
`FrinhaniTesla_*` hold the same instances split into subfolders, used only to run batches
in parallel on several machines.

Folders with a `Dataset_Description` file describe the format and the origin of that set.

## Requirements

- Linux or macOS
- g++ with C++20 support (the reported experiments used g++ 11.4.0)
- make
- pthreads, and OpenMP for the BRKGA baseline
- Python 3, for the ILP and CP models and for the analysis scripts
- Gurobi (ILP model) and OR-Tools (CP model)
- irace, only to reproduce the parameter tuning

## Build

```
cd MOSP
make
```

`make` builds all four executables: `mainMOSP` (PT-MOSP), `mainBRKGA`, `mainSND` and
`mainSA`. `make clean` removes them.

## Running

Each target runs every instance five times and writes the outputs under
`Results2/<source>/`, in a subfolder named after the method.

Single instance:

```
cd MOSP
make run-one FILE="../Instances/Frinhani/<instance>.txt"
```

One instance set:

```
make run-folder FOLDER=Frinhani
```

All instance sets:

```
make run-all
```

The baselines use the same pattern with a method suffix: `run-one-brkga`,
`run-folder-brkga`, `run-all-brkga`, and likewise for `-snd` and `-sa`.

The solver can also be called directly:

```
./mainMOSP <instance file> [options] --INST <run index> --OUTDIR <output folder>
```

## Parameters

| Flag | Meaning |
| --- | --- |
| `--TEMP_INIT` | lowest temperature |
| `--TEMP_FIM` | highest temperature |
| `--N_REPLICAS` | number of replicas |
| `--MCL` | Markov chain length between exchange attempts |
| `--PTL` | number of exchange proposals (stopping criterion) |
| `--TEMP_DIST` | temperature distribution scheme |
| `--MOV_TYPE` | neighborhood operator |
| `--TYPE_UPDATE` | temperature update scheme |
| `--TEMP_UPDATE` | update frequency, given as a divisor of `PTL` |
| `--THREAD_USED` | number of threads |
| `--SEQUENCE` | solution representation |
| `--INST` | run index, used to seed and to name the output file |
| `--OUTDIR` | output folder |

The configuration used in the paper was selected by irace and is the default `PARAMS` line
of the Makefile:

```
--TEMP_INIT 0.01 --TEMP_FIM 10 --N_REPLICAS 11 --MCL 600 --PTL 2000 \
--TEMP_DIST 2 --MOV_TYPE 2 --TYPE_UPDATE 1 --TEMP_UPDATE 3
```

Add `--THREAD_USED 11` to match the number of threads used in the reported experiments.

## Exact and constraint-based models

The ILP model (`MOSP/ILP/ilp.py`) and the CP model (`MOSP/CP/cp.py`) are run directly with
Python and write their bounds to `resultados_mosp.csv` in their own folders. The
branch-and-bound of Chu and Stuckey is in `MOSP/CHU_STUCKEY` and follows the same output
convention. In the paper these three methods are used for scalability screening on the
large-scale benchmark, not as primary competitors.

## Tables, figures and statistics

- `MOSP/Results-tables/gerar_csv.py` turns the raw per-run outputs into CSV files.
- `MOSP/Results-tables/` holds the aggregated spreadsheets: `Final_Results.xlsx`, the
  PT-MOSP results per instance set under `PT/`, the classical-benchmark reference values
  under `BenchmarksClassicos/`, and the baseline results
  (`Results_BRKGA_Frinhani.xlsx`, `Results_SA_Frinhani.xlsx`, `Results_SND_Frinhani.xlsx`).
- `MOSP/stats/statistics.py`, `statistics_bks.py` and `statistics_piecerank.py` compute the
  comparisons reported in the paper and write the `tabela_estatistica_*.csv` files in the
  same folder.
- `MOSP/Results-graphs/graph.py` generates the figures, saved as PNG next to the script.

## Solution verification

`MOSP/verify/` contains a checker that rebuilds the open-stack profile of a reported
sequence and confirms its objective value. Use `verificador.cpp` (compiled) or
`verificador.py`, and `convet_solution_to_patterns.py` to convert item sequences into
pattern sequences before checking.

## Parameter tuning

`tuning/` holds the irace setup used in the calibration: `parameters.txt` with the ranges,
`scenario.txt`, `instances-list.txt` with the training instances and `target-runner`.
`out_article.txt` is the log of the run reported in the paper. Before running irace, edit
the `EXE` variable in `target-runner` to point to your local `mainMOSP` binary.

## Authors

- Mauro Lúcio A. P. Santos Filho (corresponding author), <mauro.paulino@aluno.ufop.edu.br>
- Marco Antonio M. Carvalho, <mamc@ufop.edu.br>

Postgraduate Program in Computer Science, Federal University of Ouro Preto,
Ouro Preto, Brazil.
