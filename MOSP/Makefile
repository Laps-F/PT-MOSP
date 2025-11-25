# Nome do executável
TARGET = mainMOSP

# Pasta de código-fonte
SRC = mainMOSP.cpp

# Flags de compilação
CXX = g++ 
CXXFLAGS = ../PTAPI-main/include/*.h ./MOSP.cpp ./MCNH/*.h -std=c++20 -Wshadow -Wall -g -O3 -Wno-unused-result -lpthread -march=native

# Diretórios de instâncias
INST_DIR = ../Instances

# RESULT_DIR_A = resultados
RESULT_DIR = Results
RESULT_DIR_A = resultados
RESULT_DIR_B = resultados-challenge

# Parâmetros fixos
# PARAMS = --TEMP_INIT 0.01 --TEMP_FIM 10 --N_REPLICAS 5 --MCL 600 --TEMP_DIST 2 --MOV_TYPE 2 --TYPE_UPDATE 0 --THREAD_USED 5
PARAMS = --TEMP_INIT 0.01 --TEMP_FIM 10 --N_REPLICAS 11 --MCL 600 --PTL 2000 --TEMP_DIST 2 --MOV_TYPE 2 --TYPE_UPDATE 1 --TEMP_UPDATE 3

# Regra principal
all: $(TARGET)

# Compilação
$(TARGET): $(SRC)
	$(CXX) $(SRC) $(CXXFLAGS) -o $(TARGET) 

run-all: $(TARGET)
	@echo "==> Iniciando execução de todas as instâncias..."
	@# Itera sobre cada subdiretório encontrado dentro de $(INST_DIR)
	@find $(INST_DIR) -mindepth 1 -maxdepth 1 -type d | while read subdir; do \
		CREATOR_NAME=$$(basename "$$subdir"); \
		RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"; \
		\
		echo "--> Processando instâncias de: $$CREATOR_NAME"; \
		\
		# Cria o diretório de resultados correspondente (ex: Results/Frinhani)
		@mkdir -p "$$RESULT_SUBDIR"; \
		\
		# Encontra e itera sobre cada arquivo .txt no subdiretório atual
		@find "$$subdir" -name '*.txt' | while read file; do \
			echo "    ==> Executando $$file 5 vezes..."; \
			\
			# Executa o algoritmo 5 vezes para cada instância
			@for i in $$(seq 0 4); do \
				echo "        --> Rodada $$i..."; \
				./$(TARGET) "$$file" $(PARAMS) --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
			done; \
		done; \
	done
	@echo "==> Execução de todas as instâncias foi concluída."

run-folder: $(TARGET)
	@if [ -z "$(FOLDER)" ]; then \
		echo "Erro: Especifique o diretório do criador."; \
		exit 1; \
	fi; \
	echo "DEBUG: FOLDER='$(FOLDER)'"; \
	CREATOR_NAME="$(FOLDER)"; \
	INST_SUBDIR=../Instances/"$$CREATOR_NAME"; \
	RESULT_SUBDIR=Results/"$$CREATOR_NAME"; \
	if [ ! -d "$$INST_SUBDIR" ]; then \
		echo "Erro: Diretório de instância não encontrado: [$$INST_SUBDIR]"; \
		exit 1; \
	fi; \
	echo "==> Processando instâncias de: $$CREATOR_NAME (em $$INST_SUBDIR)"; \
	mkdir -p "$$RESULT_SUBDIR"; \
	for i in $$(seq 0 4); do \
		echo "    ==> Iniciando Rodada Mestra $$i (executando todos os arquivos 1x)..."; \
		find "$$INST_SUBDIR" -name '*.txt' | while read file; do \
			echo "        --> Processando $$file (Rodada $$i)"; \
			./$(TARGET) "$$file" $(PARAMS) --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
		done; \
		echo "    ==> Rodada Mestra $$i concluída."; \
	done; \
	echo "==> Execução de $$CREATOR_NAME concluída."


run-one: $(TARGET)
	@if [ -z "$(FILE)" ]; then \
		echo "Uso: make run-one FILE=caminho/completo/para/instancia.txt"; \
		exit 1; \
	fi
	subdir=$$(dirname "$(FILE)"); \
	CREATOR_NAME=$$(basename "$$subdir"); \
	RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"/temp; \
	mkdir -p "$$RESULT_SUBDIR"; \
	echo "==> Executando $(FILE) 5 vezes em modo de teste..."; \
	for i in $$(seq 0 4); do \
		echo "--> Rodada $$i..."; \
		./$(TARGET) "$(FILE)" $(PARAMS) --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
	done

# Limpa binários e resultados
clean:
	rm -f $(TARGET)
	rm -f $(RESULT_DIR)/*.txt
	

.PHONY: all run-all run-folder run-one clean
