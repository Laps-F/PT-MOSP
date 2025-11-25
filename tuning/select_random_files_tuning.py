import os
import shutil
import random
from collections import defaultdict

# Caminho da pasta onde estão os arquivos
origem = '../Frinhani/Instances'
# Caminho da pasta onde os arquivos selecionados serão salvos
destino = '../Instances'

# Criar a pasta de destino, se não existir
os.makedirs(destino, exist_ok=True)

# Listar todos os arquivos na pasta
arquivos = [f for f in os.listdir(origem)]

# Agrupar arquivos por conjunto
grupos = defaultdict(list)

for arquivo in arquivos:
    partes = arquivo.split('-')
    # Exemplo: ['Random', '400', '400', '2', '4_res.txt']
    conjunto = '-'.join(partes[1:4])  # '400-400-2'
    grupos[conjunto].append(arquivo)

# Selecionar um arquivo aleatório de cada grupo e copiar
for conjunto, lista_arquivos in grupos.items():
    selecionado = random.choice(lista_arquivos)
    origem_arquivo = os.path.join(origem, selecionado)
    destino_arquivo = os.path.join(destino, selecionado)
    shutil.copy2(origem_arquivo, destino_arquivo)
    print(f'Selecionado e copiado: {selecionado}')

print('Seleção e cópia concluídas!')
