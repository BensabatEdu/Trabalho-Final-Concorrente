# Processamento de imagens com pthreads – 5 versões

## Compilar o Gerador de Testes
gcc gerador_testes.c -o gerador_testes -O3 -Wall

## Compilar as 5 versões do programa
gcc src/image_sequencial.c     -o bin/sequencial     -O3 -Wall -Wextra

gcc src/image_horizontal.c     -o bin/horizontal     -lpthread -O3 -Wall -Wextra

gcc src/image_vertical.c       -o bin/vertical       -lpthread -O3 -Wall -Wextra

gcc src/image_tiling_static.c  -o bin/tiling_static  -lpthread -O3 -Wall -Wextra

gcc src/image_tiling_dynamic.c -o bin/tiling_dynamic -lpthread -O3 -Wall -Wextra

## Gerar Imagens de Teste 
./gerador_testes xadrez 4096 4096 testes/xadrez_4096.pgm

### Execução Manual
./bin/sequencial testes/xadrez_4096.pgm

### Fatias (Horizontal / Vertical)
./bin/horizontal testes/xadrez_4096.pgm 8

./bin/vertical testes/xadrez_4096.pgm 8

### Tiling (Estático / Dinâmico)
./bin/tiling_static testes/xadrez_4096.pgm 8 64 64

./bin/tiling_dynamic testes/metade_4096.pgm 8 256 256

## Execução Automatizada
chmod +x run_testes.sh

./run_testes.sh
