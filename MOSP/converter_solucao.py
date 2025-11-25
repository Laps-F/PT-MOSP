import os
import re
import sys
from pathlib import Path  # Importar Path

# --- IMPORTANTE: CONFIGURAÇÕES OBRIGATÓRIAS ---
# Carvalho_Soma (READ_FORM = 0, Leitura peça x padrao - Verificar ler_matriz 1)
# Challenge (READ_FORM = 1, Leitura padrao x peça - Verificar ler_matriz 2)
# Chu_Stuckey (READ_FORM = 1, Leitura padrao x peça - Verificar ler_matriz 2)
# Faggioli_Bentivoglio (READ_FORM = 0, Leitura peça x padrão - Verificar ler_matriz 1)
# Frinhani (READ_FORM = ???, Leitura ?? x ?? - Verificar ler_matriz ?? )
# SCOOP (READ_FORM = 0, Leitura peça x padrão - Verificar ler_matriz 1)

PASTA_INSTANCIAS = "../Instances"
PASTA_SOLUCOES = "Results/SCOOP"
READ_FORM = 0

# --- Fim das Configurações ---


# Regex para os arquivos de resultado
# Padrão 1: "SP1(0)_res.txt" ou "Shaw23(2)_res.txt"
REGEX_TIPO_1_SPSHAW = re.compile(r'([a-zA-Z]+)(\d+)?\((\d+)\)_res\.txt')
# Padrão 2: "Random-125-125-8-4(11-1)_res.txt"
REGEX_TIPO_2_RANDOM = re.compile(r'([a-zA-Z_]+)-(\d+)-(\d+)-(\d+)-(\d+)\((\d+)\)_res\.txt')
REGEX_TIPO_3_SCOOP = re.compile(r'scoop-(.+)_(.+)\((\d+)\)_res\.txt')
REGEX_TIPO_4_FAGGIOLI = re.compile(r'(p\d{4})n(\d+)\((\d+)\)_res\.txt')

def get_instance_basename(result_filename):
    """
    Com base no nome do arquivo de resultado, descobre o NOME BASE
    do arquivo de instância original.
    """
    
    # Tentativa 1: "SP1(0)_res.txt" -> "SP1.txt"
    match = REGEX_TIPO_1_SPSHAW.match(result_filename)
    if match:
        conjunto, inst_num, _ = match.groups()

        inst_num_str = inst_num if inst_num else ""
        instance_basename = f"{conjunto}{inst_num_str}.txt"

        is_challenge_file = True
        return instance_basename, is_challenge_file

    # Tentativa 2: "Random-125-125-8-4(11-1)_res.txt" -> "Random-125-125-8-4.txt"
    match = REGEX_TIPO_2_RANDOM.match(result_filename)
    if match:
        tipo, lin, col, conj_num, inst_num, _ = match.groups()
        instance_basename = f"{tipo}-{lin}-{col}-{conj_num}-{inst_num}.txt"
        is_challenge_file = False
        return instance_basename, is_challenge_file

    match = REGEX_TIPO_3_SCOOP.match(result_filename)
    if match:
        # Regex: r'scoop-(.+)_(.+)\((\d+)\)_res\.txt'
        # Grupos: (prefixo, sufixo, num)
        prefixo, sufixo, _ = match.groups()
        instance_basename = f"scoop-{prefixo}_{sufixo}.txt"
        is_challenge_file = False
        return instance_basename, is_challenge_file

    # --- ADICIONADO: TENTATIVA 4 (FAGGIOLI) ---
    # Ex: "p4020n1(3)_res.txt" -> "p4020n1.txt"
    match = REGEX_TIPO_4_FAGGIOLI.match(result_filename)
    if match:
        # Regex: r'(p\d{4})n(\d+)\((\d+)\)_res\.txt'
        # Grupos: (base_name, inst_num, run_num)
        base_name, inst_num, _ = match.groups()
        instance_basename = f"{base_name}n{inst_num}.txt"
        is_challenge_file = False
        return instance_basename, is_challenge_file


    # Se nenhum padrão bater
    return None, None

def load_instance_data(instance_filepath, is_challenge_file, read_form):
    """
    Lê o arquivo de instância e recria as estruturas de dados 
    'patterns_por_peca' e 'pattern_bitsets' do C++.
    """
    try:
        with open(instance_filepath, "r") as f:
            
            # Lê o número de padrões e peças
            line = f.readline().split()
            # numberPatterns = int(line[0])
            # numberPieces = int(line[1])
            numberPieces = int(line[0])
            numberPatterns = int(line[1])
            
            # Lê todos os valores da matriz de uma vez
            matrix_values = []
            for line in f:
                matrix_values.extend([int(v) for v in line.split()])
            
            # Inicializa as estruturas de dados
            pattern_pieces = [[] for _ in range(numberPatterns)]
            
            val_idx = 0
            if read_form == 1:
                # Formato "Rafael" (linha por linha, j < numberPatterns primeiro)
                for j in range(numberPatterns):
                    for i in range(numberPieces):
                        if val_idx < len(matrix_values) and matrix_values[val_idx] == 1:
                            pattern_pieces[j].append(i)
                        val_idx += 1
            else:
                # Formato "Outros" (coluna por coluna, i < numberPieces primeiro)
                for i in range(numberPieces):
                    for j in range(numberPatterns):
                        if val_idx < len(matrix_values) and matrix_values[val_idx] == 1:
                            pattern_pieces[j].append(i)
                        val_idx += 1

            # Constrói as estruturas de mapeamento
            pattern_bitsets = [set(pieces) for pieces in pattern_pieces]
            patterns_por_peca = [[] for _ in range(numberPieces)]
            
            for pattern_index, pieces_in_this_pattern in enumerate(pattern_bitsets):
                for piece_index in pieces_in_this_pattern:
                    patterns_por_peca[piece_index].append(pattern_index)

            return patterns_por_peca, pattern_bitsets

    except FileNotFoundError:
        # Este erro é crítico e deve sempre ser mostrado
        print(f"Erro: Arquivo de instância '{instance_filepath}' não encontrado.", file=sys.stderr)
        print(f"Verifique se a constante PASTA_INSTANCIAS está correta.", file=sys.stderr)
        return None, None
    except Exception as e:
        print(f"Erro ao processar o arquivo de instância '{instance_filepath}': {e}", file=sys.stderr)
        return None, None

def convert_to_pattern_sequence(piece_sequence, patterns_por_peca, pattern_bitsets):
    """
    Replica a lógica da função MOSP::evaluate em Python para converter
    a sequência de peças na sequência de padrões.
    """
    pieces_read = set()
    patterns_used = set()
    pattern_sequence = []
    
    for piece in piece_sequence:
        pieces_read.add(piece)
        candidate_patterns = patterns_por_peca[piece]
        
        for pattern in candidate_patterns:
            if pattern not in patterns_used:
                pieces_needed_by_pattern = pattern_bitsets[pattern]
                
                if pieces_needed_by_pattern.issubset(pieces_read):
                    pattern_sequence.append(pattern)
                    patterns_used.add(pattern)

    return pattern_sequence

def load_result_file_data(result_filepath):
    """
    Lê o arquivo de resultado, extrai a sequência de peças (linha 6),
    e retorna TODAS as linhas originais (com quebras de linha).
    """
    try:
        with open(result_filepath, "r") as f:
            all_lines = f.readlines() 
        
        if len(all_lines) < 6:
            # Erro crítico, mostrar sempre
            print(f"Erro: Arquivo de resultado '{result_filepath}' está incompleto (esperava 6+ linhas).", file=sys.stderr)
            return None, None

        piece_sequence_str = all_lines[5].strip() 
        
        if not piece_sequence_str:
            print(f"Aviso: Linha da solução de peças (Linha 6) em '{result_filepath}' está vazia.", file=sys.stderr)
            piece_sequence = []
        else:
            piece_sequence = [int(p) for p in piece_sequence_str.split()]
        
        return all_lines, piece_sequence

    except FileNotFoundError:
        print(f"Erro: Arquivo de resultado '{result_filepath}' não encontrado.", file=sys.stderr)
        return None, None
    except Exception as e:
        print(f"Erro ao ler a sequência de peças: {e}", file=sys.stderr)
        return None, None

# --- NOVA FUNÇÃO DE PROCESSAMENTO ---

def processar_e_converter_arquivo(result_filepath: str, flag_verbose: bool) -> bool:
    """
    Função principal que processa um único arquivo de resultado.
    Retorna True em sucesso, False em falha.
    """
    
    result_file_path = Path(result_filepath)
    result_filename = result_file_path.name

    # Pega o nome da pasta do arquivo de resultado (ex: "Challenge")
    # Usa .parent.name para pegar o nome da pasta imediatamente acima
    result_folder_name = result_file_path.parent.name

    # 1. Encontrar o nome base do arquivo de instância (ex: "SP1.txt")
    instance_basename, is_challenge = get_instance_basename(result_filename)
    if instance_basename is None:
        print(f"Erro: O nome '{result_filename}' não corresponde a nenhum padrão de regex conhecido.", file=sys.stderr)
        return False
        
    # 2. Construir o caminho completo do arquivo de instância
    # PASTA_INSTANCIAS (ex: "Instances")
    # + result_folder_name (ex: "Challenge")
    # + instance_basename (ex: "SP1.txt")
    # Resultado: "Instances/Challenge/SP1.txt"
    instance_filepath = os.path.join(PASTA_INSTANCIAS, result_folder_name, instance_basename)

    if flag_verbose:
        print(f"Arquivo de Resultado: {result_filepath}", file=sys.stderr)
        print(f"Arquivo de Instância: {instance_filepath}", file=sys.stderr)
        print(f"Usando Read Form: {READ_FORM}", file=sys.stderr)
        print("-" * 30, file=sys.stderr)

    # 3. Ler a sequência de peças E O CONTEÚDO ORIGINAL do arquivo de resultado
    all_lines, piece_sequence = load_result_file_data(result_filepath)
    if all_lines is None:
        # A função de load já imprimiu o erro
        return False

    # 4. Ler os dados da instância
    patterns_por_peca, pattern_bitsets = load_instance_data(instance_filepath, is_challenge, READ_FORM)
    if patterns_por_peca is None:
        # A função de load já imprimiu o erro
        return False

    # 5. Converter a sequência
    pattern_sequence = convert_to_pattern_sequence(piece_sequence, patterns_por_peca, pattern_bitsets)
    
    # 6. Formatar a nova sequência de padrões como string
    pattern_sequence_str = " ".join(map(str, pattern_sequence)) + "\n"
    
    # 7. Substituir a linha 6 (índice 5)
    all_lines[5] = pattern_sequence_str

    # print(pattern_sequence_str)

    # 8. Sobrescrever o arquivo de resultado original
    try:
        with open(result_filepath, "w") as f:
            f.writelines(all_lines)
        
        if flag_verbose:
            print(f"Arquivo '{result_filepath}' atualizado com sucesso.", file=sys.stderr)
        return True
        
    except Exception as e:
        print(f"Erro ao escrever de volta no arquivo '{result_filepath}': {e}", file=sys.stderr)
        return False


# --- FUNÇÃO MAIN ATUALIZADA ---

def main():
    
    # Modo 1: Checagem de arquivo único
    # python converter_solucao.py <caminho_para_o_arquivo_res.txt>
    if len(sys.argv) == 2:
        result_filepath = sys.argv[1]
        if not Path(result_filepath).exists():
            print(f"Erro: Arquivo de resultado não encontrado: {result_filepath}", file=sys.stderr)
            sys.exit(1)
            
        processar_e_converter_arquivo(result_filepath, True)

    # Modo 2: Modo em lote (sem argumentos)
    # python converter_solucao.py
    elif len(sys.argv) == 1:
        pasta_solucao = Path(PASTA_SOLUCOES)
        
        if not pasta_solucao.is_dir():
            print(f"Erro: Pasta de soluções '{pasta_solucao}' não encontrada.", file=sys.stderr)
            print(f"Verifique a constante PASTA_SOLUCOES.", file=sys.stderr)
            sys.exit(1)
            
        print(f"Iniciando conversão em lote...")
        print(f"Buscando arquivos '*_res.txt' em: {pasta_solucao.resolve()}")
        
        count_sucesso = 0
        count_falha = 0
        
        REGEX_RUN_NUMBER = re.compile(r'\((\d+)\)_res\.txt$')

        for result_path in pasta_solucao.rglob("*_res.txt"):

            filename = result_path.name
            
            m = REGEX_RUN_NUMBER.search(filename)
            if not m:
                continue  
            
            run_num = int(m.group(1))
           
            if run_num not in [0, 1, 2, 3, 4]:
                continue

            print(f"Processando: {result_path.relative_to(pasta_solucao)}")

            try:
                if processar_e_converter_arquivo(str(result_path), False):
                    count_sucesso += 1
                else:
                    count_falha += 1
            except Exception as e:
                print(f"Erro fatal ao processar {result_path}: {e}", file=sys.stderr)
                count_falha += 1
                
        print("\nConversão em lote concluída.")
        print(f"Sucesso: {count_sucesso} arquivos.")
        print(f"Falha: {count_falha} arquivos.")

    # Modo inválido
    else:
        print("Erro: Número incorreto de argumentos.", file=sys.stderr)
        print("Uso:", file=sys.stderr)
        print("  Modo 1 (Arquivo Único): python converter_solucao.py <caminho_para_o_arquivo_res.txt>", file=sys.stderr)
        print("  Modo 2 (Em Lote):      python converter_solucao.py", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()