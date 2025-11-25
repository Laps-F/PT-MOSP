import sys
import os
from pathlib import Path
from typing import TextIO
import re 

# --- Regex (copiado de converter_solucao.py) ---
REGEX_TIPO_1_SPSHAW = re.compile(r'([a-zA-Z]+)(\d+)?\((\d+)\)_res\.txt')
REGEX_TIPO_2_RANDOM = re.compile(r'([a-zA-Z_]+)-(\d+)-(\d+)-(\d+)-(\d+)\((\d+)\)_res\.txt')
REGEX_TIPO_3_SCOOP = re.compile(r'scoop-(.+)_(.+)\((\d+)\)_res\.txt')
REGEX_TIPO_4_FAGGIOLI = re.compile(r'(p\d{4})n(\d+)\((\d+)\)_res\.txt')

def get_instance_basename(result_filename: str) -> str | None:
    """
    Com base no nome do arquivo de resultado, descobre o NOME BASE
    do arquivo de instância original.
    Retorna None se o padrão não for reconhecido.
    """
    
    # Tentativa 1: "SP1(0)_res.txt" -> "SP1.txt"
    match = REGEX_TIPO_1_SPSHAW.match(result_filename)
    if match:
        conjunto, inst_num, _ = match.groups()
        
        inst_num_str = inst_num if inst_num else ""
        instance_basename = f"{conjunto}{inst_num_str}.txt"

        return instance_basename

    # Tentativa 2: "Random-125-125-8-4(11-1)_res.txt" -> "Random-125-125-8-4.txt"
    match = REGEX_TIPO_2_RANDOM.match(result_filename)
    if match:
        tipo, lin, col, conj_num, inst_num, _ = match.groups()
        instance_basename = f"{tipo}-{lin}-{col}-{conj_num}-{inst_num}.txt"
        return instance_basename

    # --- ADICIONADO: TENTATIVA 3 (SCOOP) ---
    # Ex: "scoop-aa_bb(0)_res.txt" -> "scoop-aa_bb.txt"
    match = REGEX_TIPO_3_SCOOP.match(result_filename)
    if match:
        # Regex: r'scoop-(.+)_(.+)\((\d+)\)_res\.txt'
        # Grupos: (prefixo, sufixo, num)
        prefixo, sufixo, _ = match.groups()
        instance_basename = f"scoop-{prefixo}_{sufixo}.txt"
        return instance_basename

    # --- ADICIONADO: TENTATIVA 4 (FAGGIOLI) ---
    # Ex: "p4020n1(3)_res.txt" -> "p4020n1.txt"
    match = REGEX_TIPO_4_FAGGIOLI.match(result_filename)
    if match:
        # Regex: r'(p\d{4})n(\d+)\((\d+)\)_res\.txt'
        # Grupos: (base_name, inst_num, run_num)
        base_name, inst_num, _ = match.groups()
        instance_basename = f"{base_name}n{inst_num}.txt"
        return instance_basename

    # Se nenhum padrão bater
    return None

# --- Fim das adições ---


class SolutionChecker:
    """
    Esta classe encapsula o estado (dados da instância e solução) 
    que era global no seu código C++.
    """
    def __init__(self):
        self.numberPatterns: int = 0
        self.numberPieces: int = 0
        self.patternPieces: list[list[int]] = []
        self.ordem: list[int] = []

    def ler_matriz(self, nome_arquivo: str) -> bool:
        """
        Lê o arquivo de instância (matriz de padrões x peças).
        Equivalente a sua função lerMatriz.
        """
        try:
            # Reseta os dados da instância (como no C++)
            self.patternPieces = []
            self.ordem = [] 

            with open(nome_arquivo, 'r') as f:
                # Lê a primeira linha: numberPatterns numberPieces
                line = f.readline().split()
                if not line:
                    print(f"Erro: Arquivo de entrada '{nome_arquivo}' está vazio ou mal formatado.", file=sys.stderr)
                    return False
                
                # self.numberPatterns = int(line[0])
                # self.numberPieces = int(line[1])
                self.numberPieces = int(line[0])
                self.numberPatterns = int(line[1])

                self.patternPieces = [[] for _ in range(self.numberPatterns)]

                # Lê o restante da matriz de uma vez
                matrix_values = []
                for line in f:
                    matrix_values.extend([int(v) for v in line.split()])

                # Popula patternPieces (lendo coluna por coluna - Formato 0)
                val_idx = 0
                for i in range(self.numberPieces):
                    for j in range(self.numberPatterns):
                        if val_idx < len(matrix_values) and matrix_values[val_idx] == 1:
                            self.patternPieces[j].append(i)
                        val_idx += 1
                        
                if val_idx != self.numberPatterns * self.numberPieces:
                    print(f"Aviso: O tamanho da matriz em '{nome_arquivo}' não bate com as dimensões {self.numberPatterns}x{self.numberPieces}.", file=sys.stderr)

            return True
        except FileNotFoundError:
            print(f"Erro ao abrir arquivo de entrada: {nome_arquivo}", file=sys.stderr)
            return False
        except Exception as e:
            print(f"Erro ao processar entrada {nome_arquivo}: {e}", file=sys.stderr)
            return False
    
    # def ler_matriz(self, nome_arquivo: str) -> bool:
    #     """
    #     Lê o arquivo de instância (matriz de padrões x peças).
    #     Equivalente a sua função lerMatriz.
    #     """
    #     try:
    #         # Reseta os dados da instância (como no C++)
    #         self.patternPieces = []
    #         self.ordem = [] 

    #         with open(nome_arquivo, 'r') as f:
    #             # Lê a primeira linha: numberPatterns numberPieces
    #             line = f.readline().split()
    #             if not line:
    #                 print(f"Erro: Arquivo de entrada '{nome_arquivo}' está vazio ou mal formatado.", file=sys.stderr)
    #                 return False

    #             self.numberPatterns = int(line[0])
    #             self.numberPieces = int(line[1])

    #             self.patternPieces = [[] for _ in range(self.numberPatterns)]

    #             # Lê o restante da matriz de uma vez
    #             matrix_values = []
    #             for line in f:
    #                 matrix_values.extend([int(v) for v in line.split()])

    #         # --- CORREÇÃO APLICADA AQUI ---
    #             # Popula patternPieces (lendo LINHA por LINHA - Formato Padrões x Peças)
    #             val_idx = 0
    #             for j in range(self.numberPatterns):
    #                 for i in range(self.numberPieces):
    #                     if val_idx < len(matrix_values) and matrix_values[val_idx] == 1:
    #                         # Adiciona a peça 'i' à lista do padrão 'j'
    #                         self.patternPieces[j].append(i)
    #                     val_idx += 1
    #             if val_idx != self.numberPatterns * self.numberPieces:
    #                 print(f"Aviso: O tamanho da matriz em '{nome_arquivo}' não bate com as dimensões {self.numberPatterns}x{self.numberPieces}.", file=sys.stderr)

    #             print("\npatternPieces (padrão → peças):")
    #             limite = min(21, self.numberPatterns)
    #             for j in range(limite):
    #                 print(f"Padrão {j}: {self.patternPieces[j]}")
                    
    #         return True
    #     except FileNotFoundError:
    #         print(f"Erro ao abrir arquivo de entrada: {nome_arquivo}", file=sys.stderr)
    #         return False
    #     except Exception as e:
    #         print(f"Erro ao processar entrada {nome_arquivo}: {e}", file=sys.stderr)
    #         return False
       
    def ler_solucao(self, nome_arquivo: str) -> tuple[bool, int]:
        """
        Lê o arquivo de solução.
        Equivalente a sua função lerSolucao.
        """
        try:
            with open(nome_arquivo, 'r') as f:
                lines = f.readlines()
                # O formato esperado é:
                # L1: mm nn
                # L2: tempo
                # L3: valorSolu
                # L4: (ignoredLine1)
                # L5: (ignoredLine2)
                # L6: ordem (sequência de padrões)
                if len(lines) < 6:
                    print(f"Erro: Arquivo de solução '{nome_arquivo}' incompleto. Esperava 6+ linhas.", file=sys.stderr)
                    return False, 0

                # L1: mm, nn
                line1 = lines[0].split()
                # mm = int(line1[0])
                # nn = int(line1[1])
                nn = int(line1[0])
                mm = int(line1[1])

                if mm != self.numberPatterns or nn != self.numberPieces:
                    print(f"Erro: Dimensões da solução ({mm}x{nn}) em '{nome_arquivo}' não batem com a entrada ({self.numberPatterns}x{self.numberPieces}).", file=sys.stderr)
                    return False, 0
                
                # L3: valorSolu
                valor_solu = int(lines[2].strip())

                # L6: ordem
                self.ordem = [int(v) for v in lines[5].split()]

                if len(self.ordem) != self.numberPatterns:
                    print(f"Erro: A ordem da solução ({len(self.ordem)}) em '{nome_arquivo}' não bate com o num. de padrões ({self.numberPatterns}).", file=sys.stderr)
                    return False, 0
            
            return True, valor_solu
        except FileNotFoundError:
            print(f"Erro ao abrir arquivo de solução: {nome_arquivo}", file=sys.stderr)
            return False, 0
        except Exception as e:
            print(f"Erro ao processar solução {nome_arquivo}: {e}", file=sys.stderr)
            return False, 0

    def calcular_max_pilhas_abertas(self) -> int:
        """
        Calcula o valor da solução (máximo de pilhas abertas).
        Equivalente a sua função calcularMaxPilhasAbertas.
        """
        # (peça -> último padrão na 'ordem' que a utiliza)
        ultima_ocorrencia = [-1] * self.numberPieces
        # 0 = pilha fechada, 1 = pilha aberta
        ativa = [0] * self.numberPieces 

        # 1. Pré-calcula a última ocorrência de cada peça na sequência
        for i in range(self.numberPatterns):
            padrao_id = self.ordem[i]
            for peca in self.patternPieces[padrao_id]:
                # Armazena o ID do padrão (assim como no C++)
                ultima_ocorrencia[peca] = padrao_id

        pilhas_abertas = 0
        max_pilhas = 0

        # 2. Simula o processo
        for i in range(self.numberPatterns):
            padrao_id = self.ordem[i]

            # Abrir novas pilhas
            for peca in self.patternPieces[padrao_id]:
                if ativa[peca] == 0:
                    ativa[peca] = 1
                    pilhas_abertas += 1
            
            # Registra o máximo *antes* de fechar
            max_pilhas = max(max_pilhas, pilhas_abertas)

            # Fechar pilhas
            for peca in self.patternPieces[padrao_id]:
                if ativa[peca] == 1 and ultima_ocorrencia[peca] == padrao_id:
                    ativa[peca] = 0
                    pilhas_abertas -= 1

        return max_pilhas

def verificar_arquivo(checker: SolutionChecker, caminho_entrada: str, caminho_solucao: str, 
                      out_validos: TextIO, out_invalidos: TextIO, flag: bool):
    """
    Função principal de orquestração.
    Equivalente a sua função verificarArquivo.
    """
    nome_arquivo_instancia = Path(caminho_entrada).name
    nome_arquivo_solucao = Path(caminho_solucao).name
    
    if flag:
        print(f"Verificando: {nome_arquivo_solucao} (Instância: {nome_arquivo_instancia})")

    if not checker.ler_matriz(caminho_entrada):
        print(f"❌ Erro ao ler entrada: {nome_arquivo_instancia}\n", file=sys.stderr)
        out_invalidos.write(f"{nome_arquivo_solucao} (Erro ao ler instância {nome_arquivo_instancia})\n")
        return

    ok, valor_solu = checker.ler_solucao(caminho_solucao)
    if not ok:
        print(f"❌ Erro ao ler solução: {nome_arquivo_solucao}\n", file=sys.stderr)
        # Não escreve em invalidos.txt aqui, pois o erro já foi impresso
        return

    valor_calculado = checker.calcular_max_pilhas_abertas()

    if flag:
        print(f"  Valor declarado na solução: {valor_solu}")
        print(f"  Valor calculado pelo verificador: {valor_calculado}")

    if valor_calculado == valor_solu:
        if flag:
            print("  ✅ Solução VÁLIDA!")
        out_validos.write(f"{nome_arquivo_solucao}\n")
    else:
        if flag:
            print("  ❌ Solução INVÁLIDA!")
        out_invalidos.write(f"{nome_arquivo_solucao} (Instância: {nome_arquivo_instancia}, Esperado: {valor_solu}, Calculado: {valor_calculado})\n")
    
    if flag:
        print() # Adiciona linha em branco para legibilidade

def main():
    """
    Ponto de entrada principal, lida com os argumentos e o modo em lote.
    Equivalente a sua função main.
    """
    try:
        # Abre os arquivos de saída
        with open("validos.txt", "w") as out_validos, \
             open("invalidos.txt", "w") as out_invalidos:
            
            checker = SolutionChecker()
            
            # Modo 1: Checagem de arquivo único
            # python verificador.py <arquivo_instancia> <arquivo_solucao>
            if len(sys.argv) == 3:
                entrada = sys.argv[1]
                solucao = sys.argv[2]
                if not Path(entrada).exists():
                    print(f"Erro: Arquivo de entrada não encontrado: {entrada}", file=sys.stderr)
                    return
                if not Path(solucao).exists():
                    print(f"Erro: Arquivo de solução não encontrado: {solucao}", file=sys.stderr)
                    return
                
                verificar_arquivo(checker, entrada, solucao, out_validos, out_invalidos, True)
            
            # --- MODO 2: LÓGICA DE LOTE ATUALIZADA ---
            elif len(sys.argv) == 1:
                # Mantém os mesmos caminhos que você definiu
                pasta_entrada = Path("../../Instances/SCOOP")
                pasta_solucao = Path("../Results/SCOOP")
                
                if not pasta_entrada.is_dir():
                    print(f"Erro: Pasta de instâncias não encontrada: {pasta_entrada}", file=sys.stderr)
                    return
                if not pasta_solucao.is_dir():
                    print(f"Erro: Pasta de soluções não encontrada: {pasta_solucao}", file=sys.stderr)
                    return
                
                print(f"Iniciando verificação em lote...")
                print(f"Instâncias em: {pasta_entrada.resolve()}")
                print(f"Soluções em:   {pasta_solucao.resolve()}")

                # Itera sobre a PASTA DE SOLUÇÕES
                for solucao_path in pasta_solucao.iterdir():
                    if not solucao_path.is_file():
                        continue
                    
                    # 1. Tenta extrair o nome da instância a partir do nome da solução
                    instance_basename = get_instance_basename(solucao_path.name)
                    
                    if instance_basename is None:
                        # Se não for um arquivo de solução válido (ex: "Dataset_Description.txt")
                        # ou um regex que não bate, avisa e pula.
                        if "_res.txt" in solucao_path.name:
                             print(f"Aviso: Nome de arquivo '{solucao_path.name}' parece de solução, mas não bateu o regex. Pulando.", file=sys.stderr)
                        continue
                    
                    # 2. Constrói o caminho da instância correspondente
                    caminho_entrada = pasta_entrada / instance_basename
                    caminho_solucao = str(solucao_path)

                    # 3. Verifica se a instância existe
                    if not caminho_entrada.exists():
                        print(f"Aviso: Solução '{solucao_path.name}' encontrada, mas instância '{caminho_entrada.name}' não existe em '{pasta_entrada}'. Pulando.", file=sys.stderr)
                        out_invalidos.write(f"{solucao_path.name} (Instância {caminho_entrada.name} não encontrada)\n")
                        continue
                    
                    # 4. Chama a verificação (com 'False' para flag verbosa)
                    verificar_arquivo(checker, str(caminho_entrada), caminho_solucao, out_validos, out_invalidos, False)
                
                print("Verificação em lote concluída.")
            
            # Modo inválido
            else:
                print("Erro: Número incorreto de argumentos.", file=sys.stderr)
                print("Uso:", file=sys.stderr)
                print("  Modo 1 (Arquivo Único): python verificador.py <arquivo_instancia> <arquivo_solucao>", file=sys.stderr)
                print("  Modo 2 (Em Lote):       python verificador.py", file=sys.stderr)
                sys.exit(1)


    except IOError as e:
        print(f"Erro ao criar arquivos de saída (validos.txt/invalidos.txt): {e}", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Ocorreu um erro inesperado: {e}", file=sys.stderr)
        sys.exit(1)

# Padrão Python para executar o main()
if __name__ == "__main__":
    main()