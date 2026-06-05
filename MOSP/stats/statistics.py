import pandas as pd
from scipy.stats import shapiro, ttest_rel, wilcoxon
from statsmodels.stats.multitest import multipletests

# 1. Carrega os dados gerados pelo seu script anterior
df = pd.read_csv("results_for_statistics_multialgoritmo.csv")

# 2. Definição do algoritmo base e os concorrentes exatamente como nas colunas
algoritmo_controle = "PT_MOSP"
concorrentes = ["BRKGA", "SA", "SND"]

resultados = []

def analisar_par(nome_grupo, grupo_dados, controle, concorrente):
    n_instancias = len(grupo_dados)
    
    # Extrai as séries de dados removendo eventuais valores nulos (NaN)
    # Isso evita erros se um algoritmo não rodou alguma instância específica
    dados_validos = grupo_dados[[controle, concorrente]].dropna()
    n_validos = len(dados_validos)
    
    if n_validos == 0:
        return None # Pula se não houver dados para comparar
        
    val_controle = dados_validos[controle]
    val_concorrente = dados_validos[concorrente]
    
    # Contagem W-T-L (Minimização: PT_MOSP < Concorrente é Vitória)
    wins = (val_controle < val_concorrente).sum()
    ties = (val_controle == val_concorrente).sum()
    losses = (val_controle > val_concorrente).sum()
    
    diferencas = val_controle - val_concorrente
    
    # Se todas as instâncias empatarem, o teste estatístico falha por variância zero
    if ties == n_validos:
        return {
            "Concorrente": concorrente, "Grupo": nome_grupo, 
            "Instâncias": n_validos, "Wins": wins, "Ties": ties, "Losses": losses,
            "Teste": "N/A (Empate total)", "p_valor_bruto": 1.0
        }

    # Teste de normalidade de Shapiro-Wilk nas diferenças
    # (Só roda se houver pelo menos 3 dados diferentes para o teste não quebrar)
    if len(set(diferencas)) > 2:
        stat_shapiro, p_shapiro = shapiro(diferencas)
    else:
        p_shapiro = 0.0 # Força o não-paramétrico se houver pouca variação
    
    # Escolha do teste pareado
    if p_shapiro > 0.05:
        stat, p_valor = ttest_rel(val_controle, val_concorrente)
        teste = "t-Student"
    else:
        try:
            # zero_method='wilcox' descarta as instâncias empatadas durante o cálculo do p-valor
            stat, p_valor = wilcoxon(val_controle, val_concorrente, zero_method='wilcox')
        except ValueError:
            p_valor = 1.0 
        teste = "Wilcoxon"

    return {
        "Concorrente": concorrente, 
        "Grupo": nome_grupo, 
        "Instâncias": n_validos, 
        "Wins": wins, "Ties": ties, "Losses": losses,
        "Teste": teste, 
        "p_valor_bruto": p_valor
    }

print("Executando testes estatísticos...\n")

for concorrente in concorrentes:
    # A. Análise Global (todas as linhas da tabela juntas)
    res_global = analisar_par("Global", df, algoritmo_controle, concorrente)
    if res_global: resultados.append(res_global)
    
    # B. Análise Agrupada por Dimensão (tamanho)
    for tamanho, grupo in df.groupby("tamanho"):
        res_grupo = analisar_par(tamanho, grupo, algoritmo_controle, concorrente)
        if res_grupo: resultados.append(res_grupo)

# Converte para DataFrame para organizar
df_resultados = pd.DataFrame(resultados)

# C. Aplica a correção de Holm-Bonferroni em todos os p-valores calculados de uma vez
p_valores_brutos = df_resultados["p_valor_bruto"].values
reject, pvals_corrected, _, _ = multipletests(p_valores_brutos, alpha=0.05, method='holm')

# Adiciona o p-valor corrigido na tabela final
df_resultados["p_valor_ajustado"] = pvals_corrected

# Ordena a tabela visualmente: Primeiro pelo Concorrente, depois pela Ordem Alfabética/Numérica do Grupo
df_resultados = df_resultados.sort_values(by=["Concorrente", "Grupo"], ascending=[True, False])

# Seleciona as colunas finais para exibição
colunas_exibicao = ["Concorrente", "Grupo", "Instâncias", "Wins", "Ties", "Losses", "p_valor_ajustado", "Teste"]
tabela_final = df_resultados[colunas_exibicao]

print("=== Tabela de Resultados Estatísticos: PT-MOSP vs Literatura ===")
print(tabela_final.to_string(index=False))

# Exportação para facilitar colocar no artigo/texto:
tabela_final.to_csv("tabela_estatistica_final_artigo.csv", index=False)

# Exportação direta para código LaTeX (ideal para o formato do seu documento)
# print("\n=== Código LaTeX da Tabela ===")
# print(tabela_final.to_latex(index=False, float_format="%.2e"))