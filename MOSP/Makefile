# Nome do executável
TARGET = mainMOSP
TARGET_BRKGA = mainBRKGA
TARGET_VND = mainVND

# Pasta de código-fonte
SRC = mainMOSP.cpp
SRC_BRKGA = mainBRKGA.cpp BRKGA/mospDecoder.cpp BRKGA/Population.cpp
SRC_VND = VND/vnd.cpp

# Flags de compilação
CXX = g++ 
CXXFLAGS = ../PTAPI-main/include/*.h ./MOSP.cpp ./MCNH/*.h -std=c++20 -Wshadow -Wall -g -O3 -Wno-unused-result -lpthread -march=native
CXXFLAGS_BRKGA = -std=c++20 -fopenmp -Wshadow -Wall -g -O3 -Wno-unused-result -lpthread -march=native
CXXFLAGS_VND = -I. -std=c++20 -Wshadow -Wall -g -O3 -Wno-unused-result -lpthread -march=native

# Diretórios de instâncias
INST_DIR = ../Instances

# RESULT_DIR_A = resultados
RESULT_DIR = Results2
RESULT_DIR_A = resultados	
RESULT_DIR_B = resultados-challenge

# Parâmetros fixos
# PARAMS = --TEMP_INIT 0.01 --TEMP_FIM 10 --N_REPLICAS 5 --MCL 600 --TEMP_DIST 2 --MOV_TYPE 2 --TYPE_UPDATE 0 --THREAD_USED 5
PARAMS = --TEMP_INIT 0.01 --TEMP_FIM 10 --N_REPLICAS 11 --MCL 600 --PTL 2000 --TEMP_DIST 2 --MOV_TYPE 2 --TYPE_UPDATE 1 --TEMP_UPDATE 3

# Regra principal
all: $(TARGET) $(TARGET_BRKGA) $(TARGET_VND)

# Compilação
$(TARGET): $(SRC)
	$(CXX) $(SRC) $(CXXFLAGS) -o $(TARGET) 

$(TARGET_BRKGA): $(SRC_BRKGA)
	$(CXX) $(SRC_BRKGA) $(CXXFLAGS_BRKGA) -o $(TARGET_BRKGA) 

$(TARGET_VND): $(SRC_VND)
	$(CXX) $(SRC_VND) $(CXXFLAGS_VND) -o $(TARGET_VND) 

run-all: $(TARGET)
	@echo "==> Iniciando execução de todas as instâncias..."
	@find $(INST_DIR) -mindepth 1 -maxdepth 1 -type d | while read subdir; do \
		CREATOR_NAME=$$(basename "$$subdir"); \
		RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"; \
		echo "--> Processando instâncias de: $$CREATOR_NAME"; \
		@mkdir -p "$$RESULT_SUBDIR"; \
		@find "$$subdir" -name '*.txt' | while read file; do \
			echo "    ==> Executando $$file 5 vezes..."; \
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
	RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"; \
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

# Regras para BRKGA
run-all-brkga: $(TARGET_BRKGA)
	@echo "==> Iniciando execução de todas as instâncias com BRKGA..."
	@ { \
		find $(INST_DIR) -mindepth 1 -maxdepth 1 -type d ! -name "Frinhani"; \
		find $(INST_DIR) -mindepth 1 -maxdepth 1 -type d -name "Frinhani"; \
	} | while read subdir; do \
		if [ -z "$$subdir" ]; then continue; fi; \
		CREATOR_NAME=$$(basename "$$subdir"); \
		RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"; \
		echo "==> Processando instâncias de: $$CREATOR_NAME (em $$subdir) com BRKGA"; \
		mkdir -p "$$RESULT_SUBDIR"; \
		for i in $$(seq 0 4); do \
			echo "    ==> Iniciando Rodada Mestra $$i (executando todos os arquivos 1x)..."; \
			find "$$subdir" -name '*.txt' | while read file; do \
				echo "        --> Processando $$file (Rodada $$i)"; \
				./$(TARGET_BRKGA) "$$file" --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
			done; \
			echo "    ==> Rodada Mestra $$i concluída."; \
		done; \
		echo "==> Execução de $$CREATOR_NAME com BRKGA concluída."; \
	done
	@echo "==> Execução de TODAS as instâncias com BRKGA foi concluída."

run-folder-brkga: $(TARGET_BRKGA)
	@if [ -z "$(FOLDER)" ]; then \
		echo "Erro: Especifique o diretório do criador."; \
		exit 1; \
	fi; \
	echo "DEBUG: FOLDER='$(FOLDER)'"; \
	CREATOR_NAME="$(FOLDER)"; \
	INST_SUBDIR=../Instances/"$$CREATOR_NAME"; \
	RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"; \
	if [ ! -d "$$INST_SUBDIR" ]; then \
		echo "Erro: Diretório de instância não encontrado: [$$INST_SUBDIR]"; \
		exit 1; \
	fi; \
	echo "==> Processando instâncias de: $$CREATOR_NAME (em $$INST_SUBDIR) com BRKGA"; \
	mkdir -p "$$RESULT_SUBDIR"; \
	for i in $$(seq 0 4); do \
		echo "    ==> Iniciando Rodada Mestra $$i (executando todos os arquivos 1x)..."; \
		find "$$INST_SUBDIR" -name '*.txt' | sort -V | while read file; do \
			echo "        --> Processando $$file (Rodada $$i)"; \
			./$(TARGET_BRKGA) "$$file" --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
		done; \
		echo "    ==> Rodada Mestra $$i concluída."; \
	done; \
	echo "==> Execução de $$CREATOR_NAME com BRKGA concluída."

run-one-brkga: $(TARGET_BRKGA)
	@if [ -z "$(FILE)" ]; then \
		echo "Uso: make run-one-brkga FILE=caminho/completo/para/instancia.txt"; \
		exit 1; \
	fi
	subdir=$$(dirname "$(FILE)"); \
	CREATOR_NAME=$$(basename "$$subdir"); \
	RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"/temp; \
	mkdir -p "$$RESULT_SUBDIR"; \
	echo "==> Executando $(FILE) 5 vezes em modo de teste com BRKGA..."; \
	for i in $$(seq 0 4); do \
		echo "--> Rodada $$i..."; \
		./$(TARGET_BRKGA) "$(FILE)" --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
	done

# Regras para VND
run-all-vnd: $(TARGET_VND)
	@echo "==> Iniciando execução de todas as instâncias com VND..."
	@ { \
		find $(INST_DIR) -mindepth 1 -maxdepth 1 -type d ! -name "Frinhani"; \
		find $(INST_DIR) -mindepth 1 -maxdepth 1 -type d -name "Frinhani"; \
	} | while read subdir; do \
		if [ -z "$$subdir" ]; then continue; fi; \
		CREATOR_NAME=$$(basename "$$subdir"); \
		RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"; \
		echo "==> Processando instâncias de: $$CREATOR_NAME (em $$subdir) com VND"; \
		mkdir -p "$$RESULT_SUBDIR"; \
		for i in $$(seq 0 4); do \
			echo "    ==> Iniciando Rodada Mestra $$i (executando todos os arquivos 1x)..."; \
			find "$$subdir" -name '*.txt' | while read file; do \
				echo "        --> Processando $$file (Rodada $$i)"; \
				./$(TARGET_VND) "$$file" --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
			done; \
			echo "    ==> Rodada Mestra $$i concluída."; \
		done; \
		echo "==> Execução de $$CREATOR_NAME com VND concluída."; \
	done
	@echo "==> Execução de TODAS as instâncias com VND foi concluída."

run-folder-vnd: $(TARGET_VND)
	@if [ -z "$(FOLDER)" ]; then \
		echo "Erro: Especifique o diretório do criador."; \
		exit 1; \
	fi; \
	echo "DEBUG: FOLDER='$(FOLDER)'"; \
	CREATOR_NAME="$(FOLDER)"; \
	INST_SUBDIR=../Instances/"$$CREATOR_NAME"; \
	RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"; \
	if [ ! -d "$$INST_SUBDIR" ]; then \
		echo "Erro: Diretório de instância não encontrado: [$$INST_SUBDIR]"; \
		exit 1; \
	fi; \
	echo "==> Processando instâncias de: $$CREATOR_NAME (em $$INST_SUBDIR) com VND"; \
	mkdir -p "$$RESULT_SUBDIR"; \
	for i in $$(seq 0 4); do \
		echo "    ==> Iniciando Rodada Mestra $$i (executando todos os arquivos 1x)..."; \
		find "$$INST_SUBDIR" -name '*.txt' | sort -V | while read file; do \
			echo "        --> Processando $$file (Rodada $$i)"; \
			./$(TARGET_VND) "$$file" --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
		done; \
		echo "    ==> Rodada Mestra $$i concluída."; \
	done; \
	echo "==> Execução de $$CREATOR_NAME com VND concluída."

run-one-vnd: $(TARGET_VND)
	@if [ -z "$(FILE)" ]; then \
		echo "Uso: make run-one-vnd FILE=caminho/completo/para/instancia.txt"; \
		exit 1; \
	fi
	subdir=$$(dirname "$(FILE)"); \
	CREATOR_NAME=$$(basename "$$subdir"); \
	RESULT_SUBDIR="$(RESULT_DIR)/$$CREATOR_NAME"/temp; \
	mkdir -p "$$RESULT_SUBDIR"; \
	echo "==> Executando $(FILE) 5 vezes em modo de teste com VND..."; \
	for i in $$(seq 0 4); do \
		echo "--> Rodada $$i..."; \
		./$(TARGET_VND) "$(FILE)" --INST $$i --OUTDIR "$$RESULT_SUBDIR"; \
	done

# Limpa binários e resultados
clean:
	rm -f $(TARGET)
	rm -f $(TARGET_BRKGA)
	rm -f $(TARGET_VND)
	rm -f $(RESULT_DIR)/*.txt
	

.PHONY: all run-all run-folder run-one clean run-all-brkga run-folder-brkga run-one-brkga run-all-vnd run-folder-vnd run-one-vnd
