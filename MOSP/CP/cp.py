import sys
import os
import csv
import ortools
from ortools.sat.python import cp_model

def read_instance(filename):
    """
    Lê a matriz, assumindo que as dimensões estão corretas para o MOSP.
    """
    try:
        with open(filename, 'r') as file:
            tokens = file.read().split()
            
        if not tokens:
            print("Erro: O ficheiro está vazio.")
            sys.exit(1)
            
        num_items = int(tokens[0])
        num_patterns = int(tokens[1])
        
        matrix_A = []
        idx = 2
        
        for i in range(num_items):
            row = []
            for j in range(num_patterns):
                row.append(int(tokens[idx]))
                idx += 1
            matrix_A.append(row)
            
        return num_items, num_patterns, matrix_A

    except Exception as e:
        print(f"Erro ao processar a instância: {e}")
        sys.exit(1)

def save_log_csv(log_data, filename="resultados_mosp.csv"):
    """
    Salva os resultados em um arquivo CSV. Cria o cabeçalho se o arquivo não existir.
    """
    file_exists = os.path.isfile(filename)
    
    headers = [
        "Instancia", "Modelo_Formulacao", "Solver", "Versao_Solver",
        "Tempo_Limite_Seg", "Tempo_Execucao_Seg", "Status", 
        "Solucao_Corrente_Obj", "Best_Bound", "Gap_Percentual"
    ]
    
    with open(filename, mode='a', newline='', encoding='utf-8') as f:
        writer = csv.DictWriter(f, fieldnames=headers)
        if not file_exists:
            writer.writeheader()
        writer.writerow(log_data)

def solve_mosp_cp_strict(instance_name, num_items, num_patterns, matrix_A, time_limit=21600.0):
    model = cp_model.CpModel()

    max_open_stacks = model.NewIntVar(0, num_items, 'max_open_stacks')

    # Variáveis dos Padrões (A Sequência no Tempo)
    pattern_starts = [model.NewIntVar(0, num_patterns - 1, f'P_start_{j}') for j in range(num_patterns)]
    model.AddAllDifferent(pattern_starts)

    item_intervals = []
    demands = []

    # Variáveis dos Itens (As Pilhas e o Span)
    for i in range(num_items):
        producing_patterns_vars = [pattern_starts[j] for j in range(num_patterns) if matrix_A[i][j] == 1]
        
        if producing_patterns_vars:
            i_start = model.NewIntVar(0, num_patterns - 1, f'I_start_{i}')
            i_end_m1 = model.NewIntVar(0, num_patterns - 1, f'I_end_m1_{i}')
            i_end = model.NewIntVar(1, num_patterns, f'I_end_{i}')
            i_size = model.NewIntVar(1, num_patterns, f'I_size_{i}')

            model.AddMinEquality(i_start, producing_patterns_vars)
            model.AddMaxEquality(i_end_m1, producing_patterns_vars)
            
            model.Add(i_end == i_end_m1 + 1)
            model.Add(i_size == i_end - i_start)

            interval = model.NewIntervalVar(i_start, i_size, i_end, f'I_interval_{i}')
            item_intervals.append(interval)
            demands.append(1)

    # Restrição de Capacidade
    model.AddCumulative(item_intervals, demands, max_open_stacks)
    model.Minimize(max_open_stacks)

    # Execução e Configurações do Solver
    solver = cp_model.CpSolver()
    
    # --- CONFIGURAÇÕES CRÍTICAS DE BENCHMARK ---
    solver.parameters.max_time_in_seconds = time_limit 
    solver.parameters.log_search_progress = True
    solver.parameters.search_branching = cp_model.PORTFOLIO_SEARCH
    
    # GARANTIA DE 1 THREAD (Isolamento Computacional)
    solver.parameters.num_search_workers = 1

    solver_name = "OR-Tools (CP-SAT)"
    solver_version = ortools.__version__

    print(f"\nIniciando o solver {solver_name} v{solver_version}...")
    print(f"Threads permitidas: 1")
    print(f"Tempo limite configurado para: {time_limit} segundos ({(time_limit/3600):.2f}h)")
    
    # Inicializa variáveis para o bloco finally
    status_str = "INTERRUPTED_OR_CRASHED"
    wall_time = 0.0
    obj_val = None
    best_bound = None
    gap = None

    try:
        status = solver.Solve(model)
        
        status_str = solver.StatusName(status)
        wall_time = solver.WallTime()
        
        if status == cp_model.OPTIMAL or status == cp_model.FEASIBLE:
            obj_val = solver.ObjectiveValue()
            best_bound = solver.BestObjectiveBound()
            
            if obj_val > 0:
                gap = ((obj_val - best_bound) / obj_val) * 100.0
            else:
                gap = 0.0

            if status == cp_model.OPTIMAL:
                print(f"\n[SUCESSO] SOLUÇÃO ÓTIMA COMPROVADA!")
            else:
                print(f"\n[AVISO] Limite de tempo esgotado. Melhor solução viável encontrada.")
                
            print(f"Máximo de Pilhas Abertas (Custo C): {int(obj_val)}")
            print(f"Melhor Limite Inferior (Best Bound): {int(best_bound)}")
            print(f"Gap de Otimalidade: {gap:.2f}%")
            print(f"Tempo de Execução: {wall_time:.2f}s")
            
            starts = [(j, solver.Value(pattern_starts[j])) for j in range(num_patterns)]
            starts.sort(key=lambda x: x[1])
            sequence = [x[0] for x in starts]
            print("Sequência de Padrões: " + " ".join(map(str, sequence)))
            
        else:
            print("\nNenhuma solução viável encontrada dentro do tempo.")
            try:
                best_bound = solver.BestObjectiveBound()
            except Exception:
                pass

    except KeyboardInterrupt:
        print("\n[AVISO] Execução interrompida pelo usuário via teclado (Ctrl+C). Gravando dados parciais...")
        status_str = "USER_INTERRUPTED"
        try: wall_time = solver.WallTime() 
        except: pass
        
    except Exception as e:
        print(f"\n[ERRO] Ocorreu uma falha inesperada: {e}")
        status_str = f"CRASHED_ERROR"

    finally:
        # Gravação Garantida
        log_data = {
            "Instancia": os.path.basename(instance_name),
            "Modelo_Formulacao": "CP_Intervalos",
            "Solver": solver_name,
            "Versao_Solver": solver_version,
            "Tempo_Limite_Seg": time_limit,
            "Tempo_Execucao_Seg": round(wall_time, 2) if wall_time > 0 else "",
            "Status": status_str,
            "Solucao_Corrente_Obj": int(obj_val) if obj_val is not None else "",
            "Best_Bound": int(best_bound) if best_bound is not None else "",
            "Gap_Percentual": round(gap, 4) if gap is not None else ""
        }
        
        save_log_csv(log_data)
        print(f"--> Log gravado no arquivo 'resultados_mosp.csv' com status: {status_str}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Uso: python3 {sys.argv[0]} <caminho_para_ficheiro.txt>")
        print("Opção: Adicione '--transpose' para inverter Linhas/Colunas caso o Custo não baixe.")
        sys.exit(1)
        
    instancia_arquivo = sys.argv[1]
    
    num_items, num_patterns, matrix_A = read_instance(instancia_arquivo)
    
    if len(sys.argv) > 2 and sys.argv[2] == '--transpose':
        print("--> APLICANDO TRANSPOSIÇÃO DE MATRIZ <--")
        matrix_A_transposta = [[matrix_A[j][i] for j in range(num_items)] for i in range(num_patterns)]
        matrix_A = matrix_A_transposta
        num_items, num_patterns = num_patterns, num_items
        instancia_arquivo += "_transposta"
        
    print(f"\n--- Instância: {instancia_arquivo} ---")
    print(f"Dimensões Ativas -> Itens (Linhas): {num_items} | Padrões (Colunas): {num_patterns}")
    
    solve_mosp_cp_strict(instancia_arquivo, num_items, num_patterns, matrix_A)