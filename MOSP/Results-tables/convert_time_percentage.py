import os
import glob
import re

# ================= CONFIGURAÇÕES =================
# 1. Coloque aqui o caminho da pasta onde estão os seus arquivos .txt
pasta_arquivos = "/home/mauro/TCC/MOSP/Results2/Frinhani/SND"

# 2. O fator de conversão proporcional (20 min em ms / tempo base anterior)
faktor = 1200000 / 1080169
# =================================================

# Busca todos os arquivos .txt na pasta configurada
caminho_busca = os.path.join(pasta_arquivos, "*.txt")
arquivos = glob.glob(caminho_busca)

print(f"Encontrados {len(arquivos)} arquivos .txt. Iniciando conversão...\n")

for arquivo in arquivos:
    try:
        # Abre o arquivo para ler as linhas existentes
        # 'utf-8' garante que caracteres como 'ç' e 'ã' não quebrem o script
        with open(arquivo, 'r', encoding='utf-8') as f:
            linhas = f.readlines()
        
        # Verifica se o arquivo tem pelo menos 2 linhas (onde fica o tempo)
        if len(linhas) > 1:
            linha_tempo = linhas[1].rstrip('\n') # Remove a quebra de linha temporariamente
            
            # Expressão regular: separa o número inicial do restante da linha
            match = re.match(r'^(\d+)(.*)', linha_tempo)
            
            if match:
                valor_original = int(match.group(1))
                resto_da_linha = match.group(2) # Guarda o " (tempo ms)" ou espaços se existirem
                
                # Calcula o novo valor proporcional e arredonda para inteiro
                valor_novo = int(round(valor_original * faktor))
                
                # Substitui apenas a segunda linha (índice 1), mantendo a estrutura original
                linhas[1] = f"{valor_novo}{resto_da_linha}\n"
                
                # Sobrescreve o arquivo original com os dados atualizados
                with open(arquivo, 'w', encoding='utf-8') as f:
                    f.writelines(linhas)
                    
                print(f"[Sucesso] {os.path.basename(arquivo)}: {valor_original} ms -> {valor_novo} ms")
            else:
                print(f"[Aviso] Segunda linha não contém um número válido em: {os.path.basename(arquivo)}")
        else:
            print(f"[Aviso] Arquivo muito curto ou vazio: {os.path.basename(arquivo)}")
            
    except Exception as e:
        print(f"[Erro] Falha ao processar {os.path.basename(arquivo)}: {e}")

print("\nProcesso concluído com sucesso!")