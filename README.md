# Trabalho-Final-Concorrente

# Compile o gerador de testes
gcc gerador_testes.c -o gerador_testes -O3 -Wall

# Criar o diretório de binários
mkdir -p bin

# Compilar a Versão Sequencial (Baseline)
gcc src/image_sequencial.c -o bin/sequencial -O3 -Wall -Wextra

# Compilar a Versão Fatias Horizontais
gcc src/image_horizontal.c -o bin/horizontal -lpthread -O3 -Wall -Wextra

# Compilar a Versão Fatias Verticais
gcc src/image_vertical.c -o bin/vertical -lpthread -O3 -Wall -Wextra

# Compilar a Versão Tiling Estático
gcc src/image_tiling_static.c -o bin/tiling_static -lpthread -O3 -Wall -Wextra

# Compilar a Versão Tiling Dinâmico
gcc src/image_tiling_dynamic.c -o bin/tiling_dynamic -lpthread -O3 -Wall -Wextra

# Criar a pasta de testes
mkdir -p testes

# --- Testes de Corretude (Pequeno) ---
./gerador_testes preto 512 512 testes/preto_512.pgm
./gerador_testes branco 512 512 testes/branco_512.pgm
./gerador_testes xadrez 512 512 testes/xadrez_512.pgm

# --- Testes de Desempenho (Médio) ---
./gerador_testes xadrez 1920 1080 testes/xadrez_1920.pgm

# --- Testes de Desempenho (Grande - Carga Uniforme) ---
./gerador_testes xadrez 4096 4096 testes/xadrez_4096.pgm

# --- Testes de Desempenho (Gigante - Carga Uniforme) ---
./gerador_testes xadrez 8192 8192 testes/xadrez_8192.pgm

# --- Testes de Desempenho (Grande - Carga Não-Uniforme) ---
# (Metade branca, metade xadrez)
./gerador_testes metade 4096 4096 testes/metade_4096.pgm

# Dê permissão de execução ao script
chmod +x run_testes.sh

# Execute o script! (Isso pode demorar alguns minutos)
./run_testes.sh

# Uso (Sequencial): ./bin/sequencial <arquivo.pgm>
./bin/sequencial testes/xadrez_4096.pgm

# Uso (Fatias): ./bin/horizontal <arquivo.pgm> <n_threads>
./bin/horizontal testes/xadrez_4096.pgm 8

# Uso (Tiling): ./bin/tiling_static <arquivo.pgm> <n_threads> [bw] [bh]
# (bw e bh são opcionais, padrão 64x64)
./bin/tiling_static testes/metade_4096.pgm 8 256 256
