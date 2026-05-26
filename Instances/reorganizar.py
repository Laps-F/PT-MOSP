import os
import shutil
import random

# Configurações
pasta_origem = 'Frinhani_All'        # A pasta atual que ficará INTACTA
pasta_nova_raiz = 'Frinhani_BRKGA'   # A nova pasta principal que o script vai criar
num_novas_pastas = 3
extensao_instancia = '.txt'          

def reorganizar_instancias():
    todos_arquivos = []

    # 1. Coletar todos os caminhos de arquivos das subpastas na pasta de origem
    for subdir in os.listdir(pasta_origem):
        caminho_subdir = os.path.join(pasta_origem, subdir)
        
        if os.path.isdir(caminho_subdir):
            arquivos = [os.path.join(caminho_subdir, f) for f in os.listdir(caminho_subdir) 
                        if f.endswith(extensao_instancia)]
            todos_arquivos.extend(arquivos)

    # 2. Embaralhar a lista para garantir aleatoriedade
    random.shuffle(todos_arquivos)

    # 3. Criar a nova estrutura de pastas (ex: Frinhani_BRKGA/Pasta1, Frinhani_BRKGA/Pasta2...)
    novas_pastas = []
    for i in range(1, num_novas_pastas + 1):
        nova_pasta = os.path.join(pasta_nova_raiz, f'Pasta{i}')
        # Cria a pasta raiz e as subpastas, caso não existam
        os.makedirs(nova_pasta, exist_ok=True) 
        novas_pastas.append(nova_pasta)

    # 4. Distribuir os arquivos copiando-os para as novas pastas (balanceamento com módulo)
    # Lembrando que a contagem (idx) começa do zero, então o resto da divisão por 3 será sempre 0, 1 ou 2
    for idx, caminho_antigo in enumerate(todos_arquivos):
        nome_arquivo = os.path.basename(caminho_antigo)
        
        # Define em qual das 3 novas pastas o arquivo vai cair
        indice_destino = idx % num_novas_pastas
        destino = os.path.join(novas_pastas[indice_destino], nome_arquivo)
        
        # Usando copy2 para COPIAR o arquivo, mantendo o original intacto
        shutil.copy2(caminho_antigo, destino)
        print(f"Copiado: {nome_arquivo} -> {destino}")

    print(f"\nConcluído! {len(todos_arquivos)} instâncias foram copiadas para a nova estrutura em '{pasta_nova_raiz}'.")

if __name__ == "__main__":
    reorganizar_instancias()