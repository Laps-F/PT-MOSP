import pandas as pd
import re
import os

def processar_todos_benchmarks():
    # 1. Dicionário de Configuração: Agora com o Challenge incluso!
    config_benchmarks = {
        "Carvalho_Soma": {
            "pt": "../Results-tables/PT/Results_CarvalhoSoma.xlsx",
            "bks": "../Results-tables/BenchmarksClassicos/LARGER AND HARDER RANDOM.xlsx",
            "regra": "padrao_random",
            "bks_sheet": 0
        },
        "Chu_Stuckey": {
            "pt": "../Results-tables/PT/Results_ChuStuckey.xlsx",
            "bks": "../Results-tables/BenchmarksClassicos/CHU-STUCKEY HARDER.xlsx",
            "regra": "padrao_random",
            "bks_sheet": 0
        },
        "Faggioli": {
            "pt": "../Results-tables/PT/Results_FaggioliBentivoglio.xlsx",
            "bks": "../Results-tables/BenchmarksClassicos/FAGGIOLI BENTIVOGLIO.xlsx",
            "regra": "padrao_faggioli",
            "bks_sheet": 0
        },
        "SCOOP": {
            "pt": "../Results-tables/PT/Results_SCOOP.xlsx",
            "bks": "../Results-tables/BenchmarksClassicos/SCOOP.xlsx",
            "regra": "padrao_scoop",
            "bks_sheet": 0
        },
        "Challenge": {
            "pt": "../Results-tables/PT/Results_Challenge.xlsx",
            "bks": "../Results-tables/BenchmarksClassicos/CHALLENGE.xlsx",
            "regra": "padrao_challenge",
            "bks_sheet": "OPTIMAL SOLUTIONS SELECTED INST" # Lê a aba correta!
        }
    }

    todos_resultados = []

    for nome_bench, caminhos in config_benchmarks.items():
        print(f"\n--- Processando Benchmark: {nome_bench} ---")
        
        if not os.path.exists(caminhos["pt"]) or not os.path.exists(caminhos["bks"]):
            print(f"⚠️ Aviso: Arquivos não encontrados para {nome_bench}. Pulando...")
            continue

        # Tenta pegar a aba de Melhores Resultados primeiro
        try:
            df_pt = pd.read_excel(caminhos["pt"], sheet_name="Summary by Instance (Best)")
        except ValueError:
            df_pt = pd.read_excel(caminhos["pt"], sheet_name=0)
            
        df_bks = pd.read_excel(caminhos["bks"], sheet_name=caminhos["bks_sheet"])
        
        colunas_possiveis = [c for c in df_pt.columns if 'best solution' in c.lower() or 'solution value' in c.lower()]
        
        if not colunas_possiveis:
            print(f"❌ Erro: Não achei a coluna de resultado em {nome_bench}.")
            continue
            
        col_pt_result = colunas_possiveis[0]
        df_pt = df_pt.rename(columns={col_pt_result: "PT_MOSP"})
        
        # Garante que as colunas base do PT sejam tratadas corretamente (evita NaN e converte float pra int)
        if "size" in df_pt.columns:
            df_pt["size"] = df_pt["size"].astype(str).str.strip()
        if "subset" in df_pt.columns:
            df_pt["subset"] = df_pt["subset"].astype(str).str.strip()
        if "instance" in df_pt.columns:
            # Converte valores vazios para string vazia e tira as casas decimais (.0)
            df_pt["instance"] = df_pt["instance"].apply(lambda x: str(int(x)) if isinstance(x, float) and pd.notna(x) else ("" if pd.isna(x) or str(x).lower() == 'nan' else str(x).strip()))

        dados_bks_extraidos = []
        
        # 2. Aplica as Regras de Extração
        for index, row in df_bks.iterrows():
            nome_original = str(row["Instance"]).strip()
            val_bks = row["Solution"]
            
            if caminhos["regra"] == "padrao_random":
                match = re.match(r"[a-zA-Z_]+-(\d+)-(\d+)-(\d+)-(\d+)", nome_original)
                if match:
                    l, c, sub, inst = match.groups()
                    dados_bks_extraidos.append({"size": f"{l}x{c}", "subset": sub, "instance": inst, "BKS": val_bks})
            
            elif caminhos["regra"] == "padrao_faggioli":
                match = re.match(r"(p\d+)(n\d+)", nome_original)
                if match:
                    sub, inst = match.groups()
                    dados_bks_extraidos.append({"subset": sub, "instance": inst, "BKS": val_bks})
            
            elif caminhos["regra"] == "padrao_scoop":
                nome_limpo = re.sub(r'\.dat(\.txt)?$', '', nome_original).strip()
                dados_bks_extraidos.append({"subset": nome_limpo, "BKS": val_bks})
                
            elif caminhos["regra"] == "padrao_challenge":
                if "miller" in nome_original.lower():
                    # Miller não tem número de instância na sua tabela, fica vazio
                    dados_bks_extraidos.append({"subset": "Miller", "instance": "", "BKS": val_bks})
                else:
                    # Separa letras (GP, Shaw) dos números (1, 10, etc)
                    match = re.match(r"^([a-zA-Z]+)(\d+)$", nome_original)
                    if match:
                        sub, inst = match.groups()
                        dados_bks_extraidos.append({"subset": sub, "instance": inst, "BKS": val_bks})

        df_bks_proc = pd.DataFrame(dados_bks_extraidos)

        # 3. Faz o Merge dependendo das colunas disponíveis
        if caminhos["regra"] == "padrao_random":
            df_merge = pd.merge(df_pt, df_bks_proc, on=["size", "subset", "instance"], how="inner")
        elif caminhos["regra"] in ["padrao_faggioli", "padrao_challenge"]:
            df_merge = pd.merge(df_pt, df_bks_proc, on=["subset", "instance"], how="inner")
        elif caminhos["regra"] == "padrao_scoop":
            df_merge = pd.merge(df_pt, df_bks_proc, on=["subset"], how="inner")
            df_merge["instance"] = "N/A" 

        # 4. Padroniza a tabela final
        df_merge = df_merge.rename(columns={"size": "tamanho", "subset": "conjunto", "instance": "instancia"})
        df_merge["benchmark"] = nome_bench
        
        if "tamanho" not in df_merge.columns:
            df_merge["tamanho"] = "N/A"
            
        df_merge = df_merge[["benchmark", "tamanho", "conjunto", "instancia", "PT_MOSP", "BKS"]]
        
        todos_resultados.append(df_merge)
        print(f"✅ Pareadas {len(df_merge)} instâncias de {nome_bench}.")

    # 5. Salva o arquivo global
    if todos_resultados:
        df_final_global = pd.concat(todos_resultados, ignore_index=True)
        nome_saida = "resultados_pt_vs_bks_GLOBAL.csv"
        df_final_global.to_csv(nome_saida, index=False)
        print(f"\n🚀 SUCESSO ABSOLUTO! Arquivo consolidado criado: {nome_saida} com {len(df_final_global)} instâncias totais.")

if __name__ == "__main__":
    processar_todos_benchmarks()