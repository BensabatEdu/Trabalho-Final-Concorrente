/*
 * gerador_testes.c
 * Compilar: 
 * gcc gerador_testes.c -o gerador_testes -O3 -Wall
 * * Uso:
 * ./gerador_testes <tipo> <largura> <altura> <arquivo_saida.pgm>
 * * Tipos:
 * preto   - Imagem totalmente preta (valor 0)
 * branco  - Imagem totalmente branca (valor 255)
 * xadrez  - Padrão de xadrez (blocos de 8x8)
 * metade   - Metade superior branca, metade inferior em padrão de xadrez
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void escrever_cabecalho_pgm(FILE *f, int w, int h) {
    fprintf(f, "P5\n");                
    fprintf(f, "%d %d\n", w, h);     
    fprintf(f, "255\n");              
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <tipo> <largura> <altura> <arquivo_saida.pgm>\n", argv[0]);
        fprintf(stderr, "Tipos: preto, branco, xadrez\n");
        return 1;
    }

    const char *tipo = argv[1];
    int w = atoi(argv[2]);
    int h = atoi(argv[3]);
    const char *nome_arquivo = argv[4];

    if (w <= 0 || h <= 0) {
        fprintf(stderr, "Erro: Largura e altura devem ser positivas.\n");
        return 1;
    }

    FILE *f = fopen(nome_arquivo, "wb");
    if (f == NULL) {
        perror("Erro ao abrir arquivo de saída");
        return 1;
    }

    escrever_cabecalho_pgm(f, w, h);

    size_t total_pixels = (size_t)w * h;
    unsigned char *data = malloc(total_pixels);
    if (data == NULL) {
        fprintf(stderr, "Erro ao alocar memória para pixels\n");
        fclose(f);
        return 1;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            size_t index = (size_t)y * w + x;
            if (strcmp(tipo, "preto") == 0) {
                data[index] = 0;
            } else if (strcmp(tipo, "branco") == 0) {
                data[index] = 255;
            } else if (strcmp(tipo, "xadrez") == 0) {
                int quad_x = (x / 8) % 2;
                int quad_y = (y / 8) % 2;
                data[index] = (quad_x ^ quad_y) ? 255 : 0; // 0 = preto, 255 = branco
            }

            else if (strcmp(tipo, "metade") == 0) {
            if (y < h / 2) {
                data[index] = 255;
            } else {
                int quad_x = (x / 8) % 2;
                int quad_y = (y / 8) % 2;
                data[index] = (quad_x ^ quad_y) ? 255 : 0;
            }
            }
            else {
                fprintf(stderr, "Erro: Tipo '%s' desconhecido.\n", tipo);
                free(data);
                fclose(f);
                return 1;
            }
        }
    }

    size_t escritos = fwrite(data, 1, total_pixels, f);
    if (escritos != total_pixels) {
        perror("Erro ao escrever dados de pixel");
        free(data);
        fclose(f);
        return 1;
    }

    free(data);
    fclose(f);
    printf("Imagem '%s' (%s, %dx%d) gerada com sucesso.\n", nome_arquivo, tipo, w, h);
    return 0;
}