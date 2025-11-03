#!/bin/bash

BIN_DIR="bin"
IMG_DIR="testes"
ARQUIVO_CSV="resultados.csv"
REPETICOES=3
LISTA_THREADS="1 2 4 8 12 16"

LISTA_IMAGENS=(
    "$IMG_DIR/xadrez_512.pgm"      
    "$IMG_DIR/xadrez_1920.pgm"    
    "$IMG_DIR/xadrez_4096.pgm"    
    "$IMG_DIR/xadrez_8192.pgm"    
    "$IMG_DIR/metade_4096.pgm"    
)

LISTA_IMAGENS_CORRETUDE=(
    "$IMG_DIR/preto_512.pgm"
    "$IMG_DIR/branco_512.pgm"
    "$IMG_DIR/xadrez_512.pgm"
)

parse_output() {
    local output="$1"
    local tipo="$2" 
    
    local regex_io="Tempo de I/O \(leitura\): ([0-9.]+) ms"
    local regex_comp="Tempo de (Computação|total): ([0-9.]+) ms"
    local regex_cont="Contador final = ([0-9]+)"
    
    if [[ "$tipo" == "IO" && "$output" =~ $regex_io ]]; then
        echo "${BASH_REMATCH[1]}"
    elif [[ "$tipo" == "COMP" && "$output" =~ $regex_comp ]]; then
        echo "${BASH_REMATCH[2]}"
    elif [[ "$tipo" == "CONTADOR" && "$output" =~ $regex_cont ]]; then
        echo "${BASH_REMATCH[1]}"
    else
        echo "ERRO"
    fi
}

echo "Versao,Imagem,N_Threads,Repeticao,Tempo_IO_ms,Tempo_Comp_ms,Contador_Final" > $ARQUIVO_CSV
echo "Iniciando testes... Isso pode demorar vários minutos."

echo "--- Iniciando Testes de Corretude (com 4 threads) ---"
for img in "${LISTA_IMAGENS_CORRETUDE[@]}"; do
    echo "Testando corretude em $img..."
    ./$BIN_DIR/sequencial "$img" >> /dev/null 
    ./$BIN_DIR/sequencial "$img"
    ./$BIN_DIR/horizontal "$img" 4
    ./$BIN_DIR/vertical "$img" 4
    ./$BIN_DIR/tiling_static "$img" 4
    ./$BIN_DIR/tiling_dynamic "$img" 4
done
echo "--- Testes de Corretude Concluídos ---"
echo "Verifique manualmente se os 'Contador final' acima são idênticos."


echo "--- Iniciando Testes de Desempenho (Repetições=$REPETICOES) ---"

for img in "${LISTA_IMAGENS[@]}"; do
    echo "Testando: sequencial | Imagem: $img"
    for r in $(seq 1 $REPETICOES); do
        output=$(./$BIN_DIR/sequencial "$img")
        
        tempo_io=$(parse_output "$output" "IO")
        tempo_comp=$(parse_output "$output" "COMP")
        contador=$(parse_output "$output" "CONTADOR")
        
        echo "sequencial,$img,1,$r,$tempo_io,$tempo_comp,$contador" >> $ARQUIVO_CSV
    done
done

for versao in horizontal vertical tiling_static tiling_dynamic; do
    for img in "${LISTA_IMAGENS[@]}"; do
        for t in $LISTA_THREADS; do
            echo "Testando: $versao | Imagem: $img | Threads: $t"
            for r in $(seq 1 $REPETICOES); do
                CMD="./$BIN_DIR/$versao $img $t"
                if [[ "$versao" == "tiling_static" || "$versao" == "tiling_dynamic" ]]; then
                    CMD+=" 64 64" 
                fi
                
                output=$($CMD)
                
                tempo_io=$(parse_output "$output" "IO")
                tempo_comp=$(parse_output "$output" "COMP")
                contador=$(parse_output "$output" "CONTADOR")
                
                echo "$versao,$img,$t,$r,$tempo_io,$tempo_comp,$contador" >> $ARQUIVO_CSV
            done
        done
    done
done

echo "--- Testes Concluídos! Resultados salvos em $ARQUIVO_CSV ---"