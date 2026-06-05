# -*- coding: utf-8 -*-
import sys
import os
import csv
import time
from mip import Model, xsum, minimize, BINARY, INTEGER, OptimizationStatus

def read_mosp_matrix(file_path):
    """Lê a matriz binária do arquivo."""
    try:
        with open(file_path, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        print("Erro: O arquivo '{0}' não foi encontrado.".format(file_path))
        sys.exit(1)
        
    matrix = []
    for line in lines:
        if not line.strip(): continue
        row = [int(val) for val in line.strip().split()]
        if len(row) == 2 and len(matrix) == 0: continue
        matrix.append(row)
            
    num_items = len(matrix)
    num_patterns = len(matrix[0])
    edges = set()
    
    for j in range(num_patterns):
        items_in_pattern = [i for i in range(num_items) if matrix[i][j] == 1]
        for idx1 in range(len(items_in_pattern)):
            for idx2 in range(idx1 + 1, len(items_in_pattern)):
                u, v = items_in_pattern[idx1], items_in_pattern[idx2]
                edges.add((min(u, v), max(u, v)))
    return num_items, list(edges)

def save_log_csv(log_data, filename="resultados_mosp.csv"):
    file_exists = os.path.isfile(filename)
    headers = ["Instancia", "Modelo_Formulacao", "Solver", "Versao_Solver",
               "Tempo_Limite_Seg", "Tempo_Execucao_Seg", "Status", 
               "Solucao_Corrente_Obj", "Best_Bound", "Gap_Percentual"]
    
    with open(filename, mode='a', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=headers)
        if not file_exists: writer.writeheader()
        writer.writerow(log_data)

def solve_mosp(file_path, num_items, edges, time_limit=21600.0):
    V = list(range(num_items))
    E = set((u, v) for u, v in edges)
    E.update((v, u) for u, v in edges)
        
    # Usando CBC por não requerer licença adicional
    m = Model(name="MOSP_Interval_Graph", solver_name="CBC")
    m.verbose = 1
    m.threads = 1
    
    status_str, wall_time, obj_val, best_bound, gap = "UNKNOWN", 0.0, None, None, None
    
    try:
        print("Iniciando montagem do modelo..."); sys.stdout.flush()
        
        # Variáveis
        K = m.add_var(name="K", var_type=INTEGER)
        x = {(i, j): m.add_var(var_type=BINARY) for i in V for j in V if i != j}
        y = {(i, j): m.add_var(var_type=BINARY) for i in V for j in V if i != j and (i, j) not in E}

        m.objective = minimize(K)

        # Restrições
        for i in V:
            for j in V:
                if i < j: m += x[i, j] + x[j, i] == 1
                
        for i in V:
            for j in V:
                for k in V:
                    if i != j and j != k and i != k: m += x[i, j] + x[j, k] + x[k, i] <= 2

        for (i, j) in y: m += y[i, j] <= x[i, j]

        for i in V:
            for j in V:
                if i != j and (i, j) not in E:
                    for k in V:
                        if k != i and k != j and (i, k) in E: m += y[i, j] <= x[k, j]

        for i in V:
            for j in V:
                if i != j and (i, j) not in E:
                    for k in V:
                        if k != i and k != j and (i, k) not in E: m += y[i, j] - y[i, k] <= x[k, j]

        for j in V:
            m += xsum(x[i, j] for i in V if i != j) - xsum(y[i, j] for i in V if i != j and (i, j) not in E) + 1 <= K

        print("Modelo montado. Iniciando otimização..."); sys.stdout.flush()
        solve_start = time.time()
        
        # Otimização
        status = m.optimize(max_seconds=time_limit)
        
        wall_time = time.time() - solve_start
        status_str = str(status).split('.')[-1]
        
        if status in [OptimizationStatus.OPTIMAL, OptimizationStatus.FEASIBLE]:
            obj_val, best_bound = m.objective_value, m.objective_bound
            gap = ((obj_val - best_bound) / obj_val) * 100.0 if obj_val > 0 else 0.0
            
    except MemoryError:
        status_str = "OUT_OF_MEMORY"
        wall_time = 0.0
    except Exception as e:
        status_str = "CRASHED_{0}".format(type(e).__name__)
        
    finally:
        log_data = {
            "Instancia": os.path.basename(file_path),
            "Modelo_Formulacao": "ILP_Grafos_Intervalo",
            "Solver": "MIP/CBC",
            "Versao_Solver": "N/A",
            "Tempo_Limite_Seg": time_limit,
            "Tempo_Execucao_Seg": round(wall_time, 2),
            "Status": status_str,
            "Solucao_Corrente_Obj": int(obj_val) if obj_val is not None else "",
            "Best_Bound": int(best_bound) if best_bound is not None else "",
            "Gap_Percentual": round(gap, 4) if gap is not None else ""
        }
        save_log_csv(log_data)
        print("\n--> Log ILP finalizado com status '{0}'".format(status_str))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python3 ilp.py <instancia> [timeout]")
        sys.exit(1)
    
    arquivo = sys.argv[1]
    tempo = float(sys.argv[2]) if len(sys.argv) > 2 else 21600.0
    n, arestas = read_mosp_matrix(arquivo)
    solve_mosp(arquivo, n, arestas, time_limit=tempo)