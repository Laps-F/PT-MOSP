import os
import shutil
import random

# Configurações
pasta_raiz = 'Frinhani_All'  # Onde estão as 10 pastas atuais
num_novas_pastas = 39
extensao_instancia = '.txt'  # Ajuste para a extensão real dos seus arquivos (ex: .ins, .txt, .dat)

def reorganizar_instancias():
    todos_arquivos = []

    # 1. Coletar todos os caminhos de arquivos das pastas part_x
    for subdir in os.listdir(pasta_raiz):
        caminho_subdir = os.path.join(pasta_raiz, subdir)
        if os.path.isdir(caminho_subdir) and subdir.startswith('part_'):
            arquivos = [os.path.join(caminho_subdir, f) for f in os.listdir(caminho_subdir) 
                        if f.endswith(extensao_instancia)]
            todos_arquivos.extend(arquivos)

    # 2. Embaralhar a lista para garantir aleatoriedade
    random.shuffle(todos_arquivos)

    # 3. Criar as 39 novas pastas
    novas_pastas = []
    for i in range(1, num_novas_pastas + 1):
        nova_pasta = os.path.join(pasta_raiz, f'thread_{i}')
        os.makedirs(nova_pasta, exist_ok=True)
        novas_pastas.append(nova_pasta)

    # 4. Distribuir os arquivos usando o operador módulo para balanceamento
    for idx, caminho_antigo in enumerate(todos_arquivos):
        nome_arquivo = os.path.basename(caminho_antigo)
        destino = os.path.join(novas_pastas[idx % num_novas_pastas], nome_arquivo)
        
        # Usando copy2 para manter metadados, ou use move se quiser apagar das originais
        shutil.move(caminho_antigo, destino)
        print(f"Movido: {nome_arquivo} -> {os.path.basename(novas_pastas[idx % num_novas_pastas])}")

    print(f"\nConcluído! {len(todos_arquivos)} instâncias distribuídas em {num_novas_pastas} pastas.")

if __name__ == "__main__":
    reorganizar_instancias()