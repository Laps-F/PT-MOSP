import os
import re
import pandas as pd
from collections import defaultdict
from statistics import mean, stdev
import gspread
from gspread_dataframe import set_with_dataframe
from google.oauth2.service_account import Credentials

# --- Settings ---
RESULTS_FOLDER = "../Results2/Challenge/BRKGA"
GOOGLE_SHEET_NAME = "Results_BRKGA_Challenge"  # Changed sheet name
FOLDER_ID = "1uUbZAu3BT5D4LrwBGA_r5Cq88KaBwwah"
CREDENTIALS_FILE = "../shaped-buttress-383520-602982384f84.json"

# Regex patterns remain the same
REGEX_TIPO_1_SPSHAW = re.compile(r'([a-zA-Z]+)(\d+)?\((\d+)\)_res\.txt')
REGEX_TIPO_2_RANDOM = re.compile(r'([a-zA-Z_]+)-(\d+)-(\d+)-(\d+)-(\d+)\((\d+)\)_res\.txt')
# --- MODIFICAÇÃO 1 ---
REGEX_TIPO_3_SCOOP = re.compile(r'scoop-(.+)\((\d+)\)_res\.txt')
REGEX_TIPO_4_FAGGIOLI = re.compile(r'(p\d{4})n(\d+)\((\d+)\)_res\.txt')

def load_data(results_folder):
    """
    Loads all result .txt files from the specified folder, parses them,
    and calculates statistics.
    """
    all_runs_data, avg_data, best_data = [], [], []
    grouped_instances = defaultdict(list)
    subsets_by_size = defaultdict(list)
    
    print(f"Loading data from: {results_folder}")

    for filename in os.listdir(results_folder):
        if not filename.endswith(".txt"):
            continue

        filepath = os.path.join(results_folder, filename)

        with open(filepath, "r") as f:
            lines = [line.strip() for line in f if line.strip()]

        if len(lines) < 6:
            print(f"Warning: file {filename} appears incomplete (expected >= 6 lines)")
            continue

        dimensions_line = lines[0]
        time_ms = float(lines[1])
        solution_value = float(lines[2])
        exchanges = int(lines[3])
        max_pieces_per_pattern = int(lines[4])
        solution_sequence = ' '.join(lines[5:])

        # --- Multi-format parsing logic ---
        size = ""
        subset_name = ""
        instance_name = ""
        execution_id = ""
        match_found = False

        match = REGEX_TIPO_1_SPSHAW.match(filename)
        if match:
            match_found = True
            subset_raw, instance_raw, exec_raw = match.groups()
            size = dimensions_line.replace(' ', 'x')
            subset_name = subset_raw
            instance_name = instance_raw
            execution_id = exec_raw

        if not match_found:
            match = REGEX_TIPO_2_RANDOM.match(filename)
            if match:
                match_found = True
                type_raw, lin, col, subset_num, inst_num, exec_num = match.groups()
                size = f"{lin}x{col}"
                subset_name = subset_num
                instance_name = inst_num
                execution_id = exec_num

        if not match_found:
            match = REGEX_TIPO_4_FAGGIOLI.match(filename)
            if match:
                match_found = True
                instance_raw, subset_raw, exec_raw = match.groups()
                size = dimensions_line.replace(' ', 'x')
                subset_name = instance_raw
                instance_name = f"n{subset_raw}"
                execution_id = exec_raw
        
        if not match_found:
            match = REGEX_TIPO_3_SCOOP.match(filename)
            if match:
                match_found = True
                
                # --- MODIFICAÇÃO 2 ---
                # O regex agora retorna 2 grupos: (subset_raw, exec_raw)
                subset_raw, exec_raw = match.groups()
                
                size = dimensions_line.replace(' ', 'x')
                subset_name = subset_raw
                instance_name = "N/A"  # Definimos um placeholder, já que não há instância
                execution_id = exec_raw

        if not match_found:
            print(f"WARNING: File {filename} did not match any regex pattern.")
            size = dimensions_line.replace(' ', 'x')
            subset_name = "Unknown"
            instance_name = "Unknown"
            execution_id = "Unknown"
        
        execution_key = (size, subset_name, instance_name)

        run_entry = {
            "filename": filename,
            "size": size,
            "subset": subset_name,
            "instance": instance_name,
            "execution": execution_id,
            "time (ms)": time_ms,
            "solution value": solution_value,
            "number of exchange attempts": exchanges,
            "number of max pieces per pattern": max_pieces_per_pattern,
            "solution": solution_sequence,
        }

        all_runs_data.append(run_entry)
        grouped_instances[execution_key].append(run_entry)

    # --- Calculate Averages and Best ---
    
    for (size, subset, instance), entries in grouped_instances.items():
        sol_values = [e["solution value"] for e in entries]
        
        mean_entry = {
            "size": size,
            "subset": subset,
            "instance": instance,
            "avg time (ms)": mean(e["time (ms)"] for e in entries),
            "avg solution value": mean(sol_values),
            "stdev solution value": stdev(sol_values) if len(sol_values) > 1 else 0.0,
            "avg exchanges": mean(e["number of exchange attempts"] for e in entries),
            "avg max pieces pattern": mean(e["number of max pieces per pattern"] for e in entries),
        }

        avg_data.append(mean_entry)

        is_faggioli = "Faggioli_Bentivoglio" in results_folder

        if is_faggioli:
            subset_key = subset
        else:
            subset_key = (size, subset)

        subsets_by_size[subset_key].append(mean_entry)

    for (size, subset, instance), entries in grouped_instances.items():
        best_entry = min(entries, key=lambda e: e["solution value"])
        
        min_entry = {
            "size": size,
            "subset": subset,
            "instance": instance,
            "time (ms)": best_entry["time (ms)"],
            "best solution value": best_entry["solution value"],
            "exchanges": best_entry["number of exchange attempts"],
            "avg max pieces pattern": mean(e["number of max pieces per pattern"] for e in entries),
        }

        best_data.append(min_entry)

    return all_runs_data, avg_data, best_data, subsets_by_size

# def calculate_subset_statistics(subsets_by_size, best_data, results_folder):
#     """
#     Calculates summary statistics for each (subset) group.
#     """
#     subset_stats = []

#     # Loop agora usa uma chave de 1 elemento: (subset_name,)
#     # Esta linha agora funciona, pois a chave é uma tupla
#     # for (size, subset_name), entries in subsets_by_size.items():
#     for key, entries in subsets_by_size.items():
    
#         if isinstance(key, tuple):
#             size, subset_name = key
#         else:
#             # Caso Faggioli → chave é só o subset
#             subset_name = key
#             # Definir size como "various" ou pegar o primeiro
#             all_sizes = set(d["size"] for d in entries)
#             size = list(all_sizes)[0] if len(all_sizes) == 1 else "Vários"

#         if "Challenge" in results_folder and subset_name != "Shaw":
#              # Criando grupos subset+instância
#             groups = {}
#             for d in entries:
#                 sub = d["subset"]
#                 inst = d["instance"]
#                 gkey = (sub, inst)
#                 if gkey not in groups:
#                     groups[gkey] = []
#                 groups[gkey].append(d)

#             # Agora calcular estatísticas para cada grupo subset+instância
#             for (sub, inst), gentries in groups.items():

#                 times = [x["avg time (ms)"] for x in gentries]
#                 values = [x["avg solution value"] for x in gentries]
#                 instance_stdevs = [x.get("stdev solution value", 0.0) for x in gentries]

#                 avg_instance_stdev = mean(instance_stdevs) if instance_stdevs else 0.0

#                 # Filtrar best_data pelo subset+instância
#                 candidates = [
#                     d for d in best_data
#                     if d["subset"] == sub and d["instance"] == inst
#                 ]

#                 if candidates:
#                     avg_best_value = mean(d["best solution value"] for d in candidates)
#                     avg_best_time = mean(d["time (ms)"] for d in candidates)
#                 else:
#                     avg_best_value = None
#                     avg_best_time = None

#                 subset_stats.append({
#                     "size": size,
#                     "subset": sub,
#                     "instance": inst,     # ← NOVO CAMPO
#                     "n_instances": len(gentries),
#                     "avg time (ms)": mean(times),
#                     "avg solution value": mean(values),
#                     "avg time (best solution)": avg_best_time,
#                     "avg value (best solution)": avg_best_value,
#                     "avg stdev (solution value)": avg_instance_stdev,
#                 })

#             continue  # importantíssimo para não cair no caso normal

#         times = [d["avg time (ms)"] for d in entries]
#         values = [d["avg solution value"] for d in entries]
        
#         instance_stdevs = [d.get("stdev solution value", 0.0) for d in entries]
#         avg_instance_stdev = mean(instance_stdevs) if instance_stdevs else 0.0

#         # Filtra 'candidates' apenas pelo 'subset_name'
#         # candidates = [
#         #     d for d in best_data
#         #     if d["subset"] == subset_name
#         # ]

#         if "instance" in entries[0]:
#             instance_name = entries[4]["instance"]
#             candidates = [
#                 d for d in best_data
#                 if d["subset"] == subset_name and d["instance"] == instance_name
#             ]
#             if subset_name == "10": # and instance == "5":
#                 print(instance_name)
#                 # print(f"Candidatos para subset={subset_name}, instance={instance_name}:")
#                 # for d in candidates:
#                 #     print(d)
#         else:
#             candidates = [
#                 d for d in best_data
#                 if d["subset"] == subset_name
#             ]
#         if candidates:
#             avg_best_value = mean(d["best solution value"] for d in candidates)
#             avg_best_time = mean(d["time (ms)"] for d in candidates)
#         else:
#             avg_best_value = None
#             avg_best_time = None


#         # Determina o que exibir na coluna 'size'
#         all_sizes = set(d["size"] for d in entries)
#         # size_display = "Vários" if len(all_sizes) > 1 else (list(all_sizes)[0] if all_sizes else "N/A")
#         size_display = size

#         subset_stats.append({
#             "size": size_display,
#             "subset": subset_name,
#             "n_instances": len(entries),
#             "avg time (ms)": mean(times),
#             "avg solution value": mean(values),
#             "avg time (best solution)": avg_best_time,
#             "avg value (best solution)": avg_best_value,
#             "avg stdev (solution value)": avg_instance_stdev,
#         })

#     return subset_stats

def calculate_subset_statistics(subsets_by_size, best_data, results_folder):
    """
    Calculates summary statistics for each (subset) group.
    Fixes: 
      - Matching specific instances correctly (including Size check).
      - Removing hardcoded indices.
    """
    subset_stats = []

    # Itera sobre os grupos criados no load_data
    for key, entries in subsets_by_size.items():
        
        # --- 1. Identificar Size e Subset ---
        if isinstance(key, tuple):
            current_size, current_subset_name = key
        else:
            # Caso Faggioli ou agrupamento simples
            current_subset_name = key
            all_sizes = set(d["size"] for d in entries)
            current_size = list(all_sizes)[0] if len(all_sizes) == 1 else "Vários"

        # --- 2. Calcular Médias Gerais do Grupo ---
        # entries contém os dados MÉDIOS de cada instância (ex: média das execuções da instância 1)
        avg_time_global = mean(d["avg time (ms)"] for d in entries)
        avg_sol_global = mean(d["avg solution value"] for d in entries)
        
        instance_stdevs = [d.get("stdev solution value", 0.0) for d in entries]
        avg_stdev_global = mean(instance_stdevs) if instance_stdevs else 0.0

        # --- 3. Calcular "Average of Best Solutions" ---
        # Precisamos encontrar o 'best value' correspondente para CADA instância dentro deste grupo
        # e tirar a média desses melhores valores.
        
        best_values_accumulator = []
        best_times_accumulator = []

        for entry in entries:
            e_inst = entry["instance"]
            e_subset = entry["subset"]
            e_size = entry["size"]

            # Procura a entrada correspondente em best_data
            # A chave é: Mesmo Size + Mesmo Subset + Mesma Instância
            match = next((b for b in best_data 
                          if b["size"] == e_size 
                          and b["subset"] == e_subset 
                          and b["instance"] == e_inst), None)
            
            if match:
                best_values_accumulator.append(match["best solution value"])
                best_times_accumulator.append(match["time (ms)"])
        
        # Calcula as médias dos melhores valores encontrados
        if best_values_accumulator:
            final_avg_best_val = mean(best_values_accumulator)
            final_avg_best_time = mean(best_times_accumulator)
        else:
            final_avg_best_val = None
            final_avg_best_time = None

        # --- 4. Montar o Dicionário de Estatísticas ---
        stats_entry = {
            "size": current_size,
            "subset": current_subset_name,
            "n_instances": len(entries),
            "avg time (ms)": avg_time_global,
            "avg solution value": avg_sol_global,
            "avg time (best solution)": final_avg_best_time,
            "avg value (best solution)": final_avg_best_val,
            "avg stdev (solution value)": avg_stdev_global,
        }
        
        # Se for Challenge (exceto Shaw), adicionamos a coluna instance para detalhamento, se necessário
        # (Mantendo sua lógica original de separar grupos se for Challenge)
        if "Challenge" in results_folder and current_subset_name != "Shaw":
             # Se quiser manter a lógica de separar por instância no output final, 
             # o loop ideal seria fora. Mas para manter compatibilidade com a estrutura:
             stats_entry["instance"] = entries[0]["instance"] if len(entries) == 1 else "Agrupado"

        subset_stats.append(stats_entry)

    return subset_stats

def get_pageRank_statistics():
    """
    Returns a hardcoded DataFrame with PageRank statistics.
    """
    statistics = [
        {"subset": "subset 2", "size": "400x400", "avg time (ms)": 0.03, "avg solution value": 82.60},
        {"subset": "subset 4", "size": "400x400", "avg time (ms)": 0.04, "avg solution value": 177.60},
        {"subset": "subset 6", "size": "400x400", "avg time (ms)": 0.05, "avg solution value": 254.80},
        {"subset": "subset 8", "size": "400x400", "avg time (ms)": 0.06, "avg solution value": 302.80},
        {"subset": "subset 10", "size": "400x400", "avg time (ms)": 0.07, "avg solution value": 331.40},
        {"subset": "subset 14", "size": "400x400", "avg time (ms)": 0.09, "avg solution value": 361.70},
        {"subset": "subset 18", "size": "400x400", "avg time (ms)": 0.10, "avg solution value": 375.60},
        {"subset": "subset 20", "size": "400x400", "avg time (ms)": 0.10, "avg solution value": 380.40},
        {"subset": "subset 24", "size": "400x400", "avg time (ms)": 0.11, "avg solution value": 386.70},
        {"subset": "subset 28", "size": "400x400", "avg time (ms)": 0.13, "avg solution value": 390.10},
        {"subset": "subset 30", "size": "400x400", "avg time (ms)": 0.14, "avg solution value": 391.70},
        {"subset": "subset 34", "size": "400x400", "avg time (ms)": 0.16, "avg solution value": 393.90},
        {"subset": "subset 2", "size": "600x600", "avg time (ms)": 0.05, "avg solution value": 125.40},
        {"subset": "subset 4", "size": "600x600", "avg time (ms)": 0.09, "avg solution value": 266.40},
        {"subset": "subset 6", "size": "600x600", "avg time (ms)": 0.12, "avg solution value": 379.20},
        {"subset": "subset 8", "size": "600x600", "avg time (ms)": 0.14, "avg solution value": 452.60},
        {"subset": "subset 10", "size": "600x600", "avg time (ms)": 0.15, "avg solution value": 494.50},
        {"subset": "subset 14", "size": "600x600", "avg time (ms)": 0.17, "avg solution value": 540.10},
        {"subset": "subset 18", "size": "600x600", "avg time (ms)": 0.19, "avg solution value": 562.00},
        {"subset": "subset 20", "size": "600x600", "avg time (ms)": 0.20, "avg solution value": 569.90},
        {"subset": "subset 24", "size": "600x600", "avg time (ms)": 0.23, "avg solution value": 577.30},
        {"subset": "subset 28", "size": "600x600", "avg time (ms)": 0.26, "avg solution value": 584.20},
        {"subset": "subset 30", "size": "600x600", "avg time (ms)": 0.27, "avg solution value": 586.60},
        {"subset": "subset 34", "size": "600x600", "avg time (ms)": 0.30, "avg solution value": 590.00},
        {"subset": "subset 38", "size": "600x600", "avg time (ms)": 0.34, "avg solution value": 591.60},
        {"subset": "subset 40", "size": "600x600", "avg time (ms)": 0.36, "avg solution value": 592.90},
        {"subset": "subset 2", "size": "800x800", "avg time (ms)": 0.09, "avg solution value": 161.50},
        {"subset": "subset 4", "size": "800x800", "avg time (ms)": 0.15, "avg solution value": 354.70},
        {"subset": "subset 6", "size": "800x8800", "avg time (ms)": 0.20, "avg solution value": 504.20},
        {"subset": "subset 8", "size": "800x800", "avg time (ms)": 0.22, "avg solution value": 596.60},
        {"subset": "subset 10", "size": "800x800", "avg time (ms)": 0.25, "avg solution value": 658.50},
        {"subset": "subset 14", "size": "800x800", "avg time (ms)": 0.28, "avg solution value": 720.50},
        {"subset": "subset 18", "size": "800x800", "avg time (ms)": 0.32, "avg solution value": 749.80},
        {"subset": "subset 20", "size": "800x800", "avg time (ms)": 0.34, "avg solution value": 758.40},
        {"subset": "subset 24", "size": "800x800", "avg time (ms)": 0.37, "avg solution value": 770.60},
        {"subset": "subset 28", "size": "800x800", "avg time (ms)": 0.42, "avg solution value": 778.00},
        {"subset": "subset 30", "size": "800x800", "avg time (ms)": 0.44, "avg solution value": 781.20},
        {"subset": "subset 34", "size": "800x800", "avg time (ms)": 0.49, "avg solution value": 785.30},
        {"subset": "subset 38", "size": "800x800", "avg time (ms)": 0.53, "avg solution value": 788.90},
        {"subset": "subset 40", "size": "800x800", "avg time (ms)": 0.57, "avg solution value": 789.80},
        {"subset": "subset 44", "size": "800x800", "avg time (ms)": 0.62, "avg solution value": 791.90},
        {"subset": "subset 48", "size": "800x800", "avg time (ms)": 0.69, "avg solution value": 793.20},
        {"subset": "subset 50", "size": "800x800", "avg time (ms)": 0.74, "avg solution value": 793.80},
        {"subset": "subset 2", "size": "1000x1000", "avg time (ms)": 0.13, "avg solution value": 200.80},
        {"subset": "subset 4", "size": "1000x1000", "avg time (ms)": 0.22, "avg solution value": 439.30},
        {"subset": "subset 6", "size": "1000x1000", "avg time (ms)": 0.29, "avg solution value": 630.90},
        {"subset": "subset 8", "size": "1000x1000", "avg time (ms)": 0.34, "avg solution value": 748.60},
        {"subset": "subset 10", "size": "1000x1000", "avg time (ms)": 0.37, "avg solution value": 821.20},
        {"subset": "subset 14", "size": "1000x1000", "avg time (ms)": 0.42, "avg solution value": 899.10},
        {"subset": "subset 18", "size": "1000x1000", "avg time (ms)": 0.48, "avg solution value": 935.10},
        {"subset": "subset 20", "size": "1000x1000", "avg time (ms)": 0.50, "avg solution value": 945.90},
        {"subset": "subset 24", "size": "1000x1000", "avg time (ms)": 0.55, "avg solution value": 962.90},
        {"subset": "subset 28", "size": "1000x1000", "avg time (ms)": 0.62, "avg solution value": 972.10},
        {"subset": "subset 30", "size": "1000x1000", "avg time (ms)": 0.65, "avg solution value": 975.30},
        {"subset": "subset 34", "size": "1000x1000", "avg time (ms)": 0.71, "avg solution value": 981.60},
        {"subset": "subset 38", "size": "1000x1000", "avg time (ms)": 0.77, "avg solution value": 985.00},
        {"subset": "subset 40", "size": "1000x1000", "avg time (ms)": 0.80, "avg solution value": 986.70},
        {"subset": "subset 44", "size": "1000x1000", "avg time (ms)": 0.88, "avg solution value": 989.20},
        {"subset": "subset 48", "size": "1000x1000", "avg time (ms)": 0.98, "avg solution value": 991.40},
        {"subset": "subset 50", "size": "1000x1000", "avg time (ms)": 1.04, "avg solution value": 991.60},
        {"subset": "subset 54", "size": "1000x1000", "avg time (ms)": 1.14, "avg solution value": 992.90}
    ]
    
    return pd.DataFrame(statistics)

def export_to_google_sheets(df_dict, sheet_name, cred_path):
    """
    Exports a dictionary of DataFrames to a Google Sheet, with each
    DataFrame as its own tab.
    """
    print("Authorizing with Google Sheets...")
    credentials = Credentials.from_service_account_file(
        cred_path,
        scopes=[
            "https://www.googleapis.com/auth/spreadsheets",
            "https://www.googleapis.com/auth/drive"
        ]
    )
    client = gspread.authorize(credentials)

    try:
        spreadsheet = client.open(sheet_name, folder_id=FOLDER_ID)
    except gspread.SpreadsheetNotFound:
        print(f"Creating new spreadsheet: {sheet_name}")
        spreadsheet = client.create(sheet_name, folder_id=FOLDER_ID)
        spreadsheet.share(credentials.service_account_email, perm_type='user', role='writer')

    print("Uploading data...")
    for tab_name, dataframe in df_dict.items():
        try:
            worksheet = spreadsheet.worksheet(tab_name)
            print(f"Clearing old tab: {tab_name}")
            worksheet.clear()
        except gspread.exceptions.WorksheetNotFound:
            print(f"Creating new tab: {tab_name}")
            worksheet = spreadsheet.add_worksheet(title=tab_name, rows="1000", cols="20")
        
        set_with_dataframe(worksheet, dataframe.fillna(""))
        print(f"Successfully uploaded: {tab_name}")

def sort_dataframe(dataframe, sort_by_size_first=True):
    """
    Sorts a DataFrame based on hierarchical rules.
    
    :param dataframe: O DataFrame a ser ordenado.
    :param sort_by_size_first: Se True, ordena por 'size' primeiro.
                               Se False, ordena por 'subset' primeiro.
    """
    
    drop_cols = []
    sort_cols = []

    # --- 1. Criar colunas auxiliares de ordenação ---

    if "size" in dataframe.columns:
        # Extrai apenas os que batem com o padrão NNNxNNN
        size_df = dataframe["size"].str.extract(r"(\d+)x(\d+)").fillna(0).astype(int)
        dataframe["n_rows"] = size_df[0]
        dataframe["n_cols"] = size_df[1]
        drop_cols.extend(["n_rows", "n_cols"])

    if "subset" in dataframe.columns:
        try:
            if dataframe["subset"].notna().all() and dataframe["subset"].astype(str).str.startswith("subset ").all():
                dataframe["subset_num"] = dataframe["subset"].str.extract(r"(\d+)").astype(int)
                sort_cols.append("subset_num")
                drop_cols.append("subset_num")
            else:
                numeric_subsets = pd.to_numeric(dataframe["subset"], errors='coerce')
                if not numeric_subsets.isnull().all():
                    dataframe["subset_num"] = numeric_subsets
                    sort_cols.append("subset_num")
                    drop_cols.append("subset_num")
                else:
                    sort_cols.append("subset")
        except (ValueError, TypeError):
            sort_cols.append("subset")

    if "instance" in dataframe.columns:
        try:
            numeric_instances = dataframe["instance"].astype(str).str.extract(r"(\d+)").astype(float)
            
            if not numeric_instances.isnull().all().all():
                 dataframe["instance_num"] = numeric_instances
                 sort_cols.append("instance_num")
                 drop_cols.append("instance_num")
            else:
                sort_cols.append("instance")
        except (ValueError, TypeError, AttributeError):
            sort_cols.append("instance")

    if "execution" in dataframe.columns:
        try:
            numeric_executions = pd.to_numeric(dataframe["execution"], errors='coerce')
            if not numeric_executions.isnull().all():
                dataframe["execution_num"] = numeric_executions
                sort_cols.append("execution_num")
                drop_cols.append("execution_num")
            else:
                sort_cols.append("execution")
        except (ValueError, TypeError):
            sort_cols.append("execution")
            
    # --- 2. Construir a lista de ordenação final ---
    
    final_sort_order = []
    
    if sort_by_size_first:
        if "size" in dataframe.columns:
            final_sort_order.extend(["n_rows", "n_cols"])
        final_sort_order.extend(sort_cols)
    else:
        # Nova ordem: Subset -> Tamanho
        final_sort_order.extend(sort_cols)
        if "size" in dataframe.columns:
            final_sort_order.extend(["n_rows", "n_cols"])

    if final_sort_order:
        unique_sort_order = list(dict.fromkeys(final_sort_order))
        dataframe = dataframe.sort_values(unique_sort_order)

    if drop_cols:
        cols_to_drop = [col for col in drop_cols if col in dataframe.columns]
        dataframe = dataframe.drop(columns=cols_to_drop)

    return dataframe


def main():
    # 1. Load data and calculate stats
    all_runs, means, bests, subsets = load_data(RESULTS_FOLDER)
    subset_stats = calculate_subset_statistics(subsets, bests, RESULTS_FOLDER)
    
    # 2. Create DataFrames
    df_all = pd.DataFrame(all_runs)
    df_mean = pd.DataFrame(means)
    df_best = pd.DataFrame(bests)
    df_subset = pd.DataFrame(subset_stats)
    df_pageRank = get_pageRank_statistics()

    # 3. Sort ALL DataFrames
    print("Sorting DataFrames...")
    
    # Ordenação padrão (por tamanho primeiro)
    df_all = sort_dataframe(df_all, sort_by_size_first=True)
    df_mean = sort_dataframe(df_mean, sort_by_size_first=True)
    df_best = sort_dataframe(df_best, sort_by_size_first=True)
    
    # Nova ordenação (por subset primeiro)
    df_subset = sort_dataframe(df_subset, sort_by_size_first=True)
    df_pageRank = sort_dataframe(df_pageRank, sort_by_size_first=False)

    # 4. Define Sheet (Tab) Names in English
    sheets = {
        "All Results": df_all,
        "Summary by Instance (Mean)": df_mean,
        "Summary by Instance (Best)": df_best,
        "Summary by Subset": df_subset,
        # "Summary PageRank by Subset": df_pageRank,
    }

    # 5. Export
    export_to_google_sheets(sheets, GOOGLE_SHEET_NAME, CREDENTIALS_FILE)
    print(f"✅ Data successfully exported to Google Sheet: {GOOGLE_SHEET_NAME}")

# Standard Python entry point
if __name__ == "__main__":
    main()