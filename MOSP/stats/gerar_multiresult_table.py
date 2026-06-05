import pandas as pd
import os
from functools import reduce

def gerar_tabela_estatistica_via_excel():
    # 1. Mapeamento dos arquivos Excel compilados
    # Usei os caminhos com base na estrutura que aparece no seu print do VS Code
    arquivos_excel = {
        "PT_MOSP": "../Results-tables/PT/Results_Frinhani.xlsx",
        "BRKGA": "../Results-tables/Results_BRKGA_Frinhani.xlsx",
        "SA": "../Results-tables/Results_SA_Frinhani.xlsx",
        "SND": "../Results-tables/Results_SND_Frinhani.xlsx"
    }

    # 2. Configurações de leitura
    # O nome da aba (Sheet) gerada pelo seu código original
    aba_alvo = "Summary by Instance (Best)"  # Mude para "Summary by Instance (Best)" se preferir testar os melhores
    coluna_metrica = "best solution value"    # Mude para "best solution value" se alterar a aba acima

    dataframes_algoritmos = []

    for nome_algoritmo, caminho_arquivo in arquivos_excel.items():
        print(f"Lendo dados de {nome_algoritmo} no arquivo: {caminho_arquivo}...")
        
        if not os.path.exists(caminho_arquivo):
            print(f"  -> AVISO: Arquivo não encontrado: {caminho_arquivo}")
            continue

        try:
            # Lê apenas a aba específica do Excel
            df = pd.read_excel(caminho_arquivo, sheet_name=aba_alvo)
            
            # Garante que não há espaços em branco extras nos nomes das colunas (boa prática)
            df.columns = df.columns.str.strip()
            
            # Filtra apenas as colunas de identificação e a métrica de interesse
            df_filtrado = df[["size", "subset", "instance", coluna_metrica]].copy()
            
            # Renomeia a coluna da métrica para o nome do algoritmo
            df_filtrado = df_filtrado.rename(columns={coluna_metrica: nome_algoritmo})
            
            # Converte as colunas de identificação para string para garantir um merge perfeito
            # (evita que '2' int não dê match com '2.0' float)
            df_filtrado["size"] = df_filtrado["size"].astype(str)
            df_filtrado["subset"] = df_filtrado["subset"].astype(str)
            df_filtrado["instance"] = df_filtrado["instance"].astype(str)
            
            dataframes_algoritmos.append(df_filtrado)
            print(f"  -> {len(df_filtrado)} instâncias carregadas com sucesso.")
            
        except Exception as e:
            print(f"  -> ERRO ao ler {caminho_arquivo}: {e}")

    if not dataframes_algoritmos:
        print("Nenhum dado pôde ser carregado. Verifique os caminhos dos arquivos.")
        return

    # 3. Mesclar (Merge) todos os algoritmos em uma única tabela
    print("\nMesclando os dados...")
    df_final = reduce(
        lambda left, right: pd.merge(left, right, on=['size', 'subset', 'instance'], how='outer'), 
        dataframes_algoritmos
    )

    # 4. Traduzir as colunas para o formato esperado pelo script de Wilcoxon
    df_final = df_final.rename(columns={
        "size": "tamanho",
        "subset": "conjunto",
        "instance": "instancia"
    })

    # (Opcional) Ordenar a tabela final de forma lógica antes de salvar
    try:
        df_final['tamanho_n'] = df_final['tamanho'].str.extract(r'(\d+)x').astype(float)
        df_final['conjunto_n'] = df_final['conjunto'].astype(float)
        df_final['instancia_n'] = df_final['instancia'].astype(float)
        df_final = df_final.sort_values(['tamanho_n', 'conjunto_n', 'instancia_n']).drop(columns=['tamanho_n', 'conjunto_n', 'instancia_n'])
    except:
        pass # Se falhar na conversão, ignora e mantém a ordem do merge

    # 5. Salvar o arquivo CSV final que será consumido pelas estatísticas
    nome_saida = "results_for_statistics_multialgoritmo.csv"
    df_final.to_csv(nome_saida, index=False)
    
    print(f"\n✅ Tabela consolidada gerada com sucesso: {nome_saida}")
    print(df_final.head())

if __name__ == "__main__":
    gerar_tabela_estatistica_via_excel()