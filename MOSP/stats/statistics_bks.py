import pandas as pd
from scipy.stats import shapiro, ttest_rel, wilcoxon
from statsmodels.stats.multitest import multipletests

# 1. Carrega os dados globais pareados
df = pd.read_csv("resultados_pt_vs_bks_GLOBAL.csv")

controle = "PT_MOSP"
concorrente = "BKS"
resultados = []

def analisar_par(nome_grupo, grupo_dados):
    # Remove eventuais linhas vazias
    dados_validos = grupo_dados[[controle, concorrente]].dropna()
    n_validos = len(dados_validos)
    
    if n_validos == 0:
        return None
        
    val_pt = dados_validos[controle]
    val_bks = dados_validos[concorrente]
    
    # Contagem W-T-L (Minimização: PT_MOSP < BKS é Vitória/Novo Recorde)
    wins = (val_pt < val_bks).sum()
    ties = (val_pt == val_bks).sum()
    losses = (val_pt > val_bks).sum()
    
    diferencas = val_pt - val_bks
    
    # Se todas as instâncias empatarem (PT-MOSP achou o BKS em 100% dos casos)
    if ties == n_validos:
        return {
            "Benchmark": nome_grupo, 
            "Instâncias": n_validos, 
            "Wins": wins, "Ties": ties, "Losses": losses,
            "Teste": "N/A (100% Empates)", "p_valor_bruto": 1.0
        }

    # Teste de normalidade (só se houver variação suficiente)
    if len(set(diferencas)) > 2:
        stat_shapiro, p_shapiro = shapiro(diferencas)
    else:
        p_shapiro = 0.0 
    
    # Escolha do teste
    if p_shapiro > 0.05:
        stat, p_valor = ttest_rel(val_pt, val_bks)
        teste = "t-Student"
    else:
        try:
            # wilcox descarta empates no cálculo matemático para não enviesar o p-valor
            stat, p_valor = wilcoxon(val_pt, val_bks, zero_method='wilcox')
        except ValueError:
            p_valor = 1.0 
        teste = "Wilcoxon"

    return {
        "Benchmark": nome_grupo, 
        "Instâncias": n_validos, 
        "Wins": wins, "Ties": ties, "Losses": losses,
        "Teste": teste, 
        "p_valor_bruto": p_valor
    }

print("Executando testes estatísticos PT-MOSP vs BKS...\n")

# A. Análise Global (Todos os benchmarks juntos)
res_global = analisar_par("GLOBAL (Todos)", df)
if res_global: resultados.append(res_global)

# B. Análise Separada por Benchmark
for bench, grupo in df.groupby("benchmark"):
    res_grupo = analisar_par(bench, grupo)
    if res_grupo: resultados.append(res_grupo)

# Converte para DataFrame
df_resultados = pd.DataFrame(resultados)

# C. Aplica Correção de Holm
p_valores_brutos = df_resultados["p_valor_bruto"].values
reject, pvals_corrected, _, _ = multipletests(p_valores_brutos, alpha=0.05, method='holm')
df_resultados["p_valor_ajustado"] = pvals_corrected

# Ordena e seleciona as colunas finais
df_resultados = df_resultados.sort_values(by="Benchmark")
colunas_exibicao = ["Benchmark", "Instâncias", "Wins", "Ties", "Losses", "p_valor_ajustado", "Teste"]
tabela_final = df_resultados[colunas_exibicao]

print("=== Tabela de Resultados Estatísticos: PT-MOSP vs BKS ===")
print(tabela_final.to_string(index=False))

# Exporta para CSV
tabela_final.to_csv("tabela_estatistica_BKS_final.csv", index=False)

# Descomente a linha abaixo se já quiser o código LaTeX pronto para a dissertação
# print("\n=== Código LaTeX ===")
# print(tabela_final.to_latex(index=False, float_format="%.2e"))